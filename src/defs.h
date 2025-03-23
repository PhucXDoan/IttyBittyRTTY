//////////////////////////////////////////////////////////////// Configurations ////////////////////////////////////////////////////////////////

/* #meta GPIOS, SIGNALS, F_CLKIO
/*
	# TODO Look into ATmega328P's clock system.
	F_CLKIO = 16_000_000 - 43_500

	GPIOS = Meta.Obj(
		Transmitter = Meta.Table(
			('name'       , 'kind'          , 'port', 'number'),
			('builtin_led', 'output'        , 'B'   , 5       ),
			('trigger'    , 'output'        , 'D'   , 4       ),
			('transmitter', 'output_compare', 'B'   , 1       ),
		),
		Receiver = Meta.Table(
			('name'       , 'kind'          , 'port', 'number'),
			('builtin_led', 'output'        , 'B'   , 5       ),
		),
	)

	SIGNALS = {
		'none'  : 0,
		'mark'  : 2295,
		'space' : 2125,
	}
*/

//////////////////////////////////////////////////////////////// Primitives ////////////////////////////////////////////////////////////////

#define false                   0
#define true                    1
#define STRINGIFY_(X)           #X
#define STRINGIFY(X)            STRINGIFY_(X)
#define CONCAT_(X, Y)           X##Y
#define CONCAT(X, Y)            CONCAT_(X, Y)
#define sizeof(...)             ((signed) sizeof(__VA_ARGS__))
#define countof(...)            (sizeof(__VA_ARGS__) / sizeof((__VA_ARGS__)[0]))
#define bitsof(...)             (sizeof(__VA_ARGS__) * 8)
#define useret                  __attribute__((warn_unused_result))
#define noret                   __attribute__((noreturn))
#define sorry                   sorry_();
#define static_assert(...)      _Static_assert(__VA_ARGS__, STRINGIFY(__VA_ARGS__))

#include "primitives.meta"
/*
	for name, underlying, size in (
		('u8'  , 'unsigned char'     , 1),
		('u16' , 'unsigned short'    , 2),
		('u32' , 'unsigned long'     , 4),
		('u64' , 'unsigned long long', 8),
		('i8'  , 'signed char'       , 1),
		('i16' , 'signed short'      , 2),
		('i32' , 'signed long'       , 4),
		('i64' , 'signed long long'  , 8),
		('b8'  , 'signed char'       , 1),
		('b16' , 'signed short'      , 2),
		('b32' , 'signed long'       , 4),
		('b64' , 'signed long long'  , 8),
		('f32' , 'float'             , 4),
	):
		Meta.line(f'''
			typedef {underlying} {name};
			static_assert(sizeof({name}) == {size});
		''')
*/

// TODO Flash strings.
#define str(...) ((str) { ("" __VA_ARGS__), sizeof("" __VA_ARGS__) })
typedef struct
{
	char* data;
	u16   len;
} str;

//////////////////////////////////////////////////////////////// str.c ////////////////////////////////////////////////////////////////

enum StrFmtBuilderCallbackResult
{
	StrFmtBuilderCallbackResult_continue,
	StrFmtBuilderCallbackResult_break,
};

typedef useret enum StrFmtBuilderCallbackResult StrFmtBuilderCallback(void* context, char* data, u16 len);

enum StrShowIntStyle
{
	StrShowIntStyle_unsigned,
	StrShowIntStyle_signed,
	StrShowIntStyle_binary,
	StrShowIntStyle_hex_lower,
	StrShowIntStyle_hex_upper,
};

//////////////////////////////////////////////////////////////// Misc. ////////////////////////////////////////////////////////////////

#define BAUD_PERIOD (1.0 / 45.45 * 1000.0) // Period of baud rate for 45.45 b/s.

#include "timer_configurer.meta"
/*
	#
	# Create enumeration of signals that could be sent.
	#

	Meta.enums('Signal', None, SIGNALS.keys())

	#
	# Calculate look-up table for configuring the timer to output the desired frequency.
	#

	with Meta.enter('static struct { u8 clksel; u16 compare_value; } SIGNAL_TABLE[] =', '{', '};', indented=True):

		for signal, goal_freq in SIGNALS.items():

			best = None

			#
			# Getting 0Hz output is easy; just use a 0Hz clock source.
			#

			if goal_freq == 0:
				best = Meta.Obj(
					compare_value = 0,
					clksel        = 0,
					error         = 0,
				)

			#
			# Otherwise, we'll have to bruteforce some options.
			#

			else:

				for clksel, divider in { # @/pg 110/tbl 15-6/(328P).
					0b001 : 1,
					0b010 : 8,
					0b011 : 64,
					0b100 : 256,
					0b101 : 1024,
				}.items():

					#
					# Different clock sources results in different frequencies
					# that the counter will be incremented at. @/pg 110/tbl 15-6/(328P).
					#

					timer1_freq = F_CLKIO / divider

					#
					# Determine the closest compare value that'll output the desired frequency.
					# Note that division of two must be done since a compare-match only accounts
					# for half of a cycle.
					#

					compare_value = round(timer1_freq / goal_freq / 2 - 1)

					#
					# Determine the actual frequency that'd be generated and the error from it.
					#

					calculated_freq = timer1_freq / (compare_value + 1) / 2
					error           = abs(calculated_freq / goal_freq - 1)

					#
					# The compare value has a limited range of 16 bits, so even if the error is
					# the best so far, we wouldn't be able to actually use the value anyways.
					# @/pg 111/sec 15.11.5/(328P).
					#

					if 0 <= compare_value <= 2**16-1:

						# We found a better configuration that minimizes the error?
						if best is None or error < best.error:
							best = Meta.Obj(
								compare_value = compare_value,
								clksel        = clksel,
								error         = error,
							)

			#
			# Export the values to be used at run-time.
			#

			Meta.line(f'[Signal_{signal}] = {{ {best.clksel}, {best.compare_value} }}, // {goal_freq} Hz, {best.error * 100 :.4f}% error.')
*/
