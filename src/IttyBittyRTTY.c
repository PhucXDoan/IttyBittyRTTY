#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdarg.h>
#include <string.h>
#include "defs.h"
#include "gpio.c"
#include "misc.c"
#include "str.c"
#include "usart0.c"

enum Signal
{
	Signal_none,
	Signal_mark,
	Signal_space,
};

static void
set_signal(enum Signal signal)
{
	// In "Clear Timer on Compare Match" mode (CTC), Timer1's counter can be modulated by
	// OCR1A. That is, the counter goes from zero and up but will reset to zero after a
	// compare match to OCR1A, thus never becoming greater than OCR1A (but can be equal to it though).
	// @/pg 97/sec 15.7/(328P).
	// @/pg 109/tbl 15-5/(328P).
	#define TIMER1_WAVEFORM_GENERATION_MODE 0b0100

	// Configure the OC1A pin to toggle on each compare-match of OCR1A. @/pg 108/sec 15.11.1/(328P).
	#define TIMER1_OC1A_MODE 0b01

	// Calculate the timing parameters to get the desired frequencies.
	#include "timer_configurer.meta"
	/*
		# TODO Look into ATmega328P's clock system.
		F_CLKIO = 16_000_000 - 43_500

		for frequency_name, desired_frequency in Meta.Obj(
			mark  = 2295,
			space = 2125,
		):

			best = None

			for clock_select_bits, divider in { # @/pg 110/tbl 15-6/(328P).
				0b001 : 1,
				0b010 : 8,
				0b011 : 64,
				0b100 : 256,
				0b101 : 1024,
			}.items():

				# Different clock sources results in different frequencies
				# that the counter will be incremented at. @/pg 110/tbl 15-6/(328P).
				timer1_frequency = F_CLKIO / divider

				# Determine the closest compare value that'll output the desired frequency.
				# Note that division of two must be done since a compare-match only accounts
				# for half of a cycle.
				compare_value = round(timer1_frequency / desired_frequency / 2 - 1)

				# Determine the actual frequency that'd be generated and the error from it.
				calculated_frequency = timer1_frequency / (compare_value + 1) / 2
				error                = abs(calculated_frequency / desired_frequency - 1)

				# The compare value has a limited range of 16 bits, so even if the error is
				# the best so far, we wouldn't be able to actually use the value anyways.
				# @/pg 111/sec 15.11.5/(328P).
				if 0 <= compare_value <= 2**16-1:
					if best is None or error < best.error:
						# We found a better configuration that minimizes the error.
						best = Meta.Obj(
							compare_value     = compare_value,
							clock_select_bits = clock_select_bits,
							error             = error,
						)

			# Export the values to be used at run-time.
			Meta.define(f'CS1_{frequency_name}'  , f'((u16) {best.clock_select_bits})')
			Meta.define(f'OCR1A_{frequency_name}', f'((u16) {best.compare_value    })')
	*/

	u8  clock_select_bits = {0};
	u16 compare_value     = {0};
	switch (signal)
	{
		case Signal_none:
		{
			clock_select_bits = 0;
			compare_value     = 0;
		} break;

		case Signal_mark:
		{
			clock_select_bits =   CS1_mark;
			compare_value     = OCR1A_mark;
		} break;

		case Signal_space:
		{
			clock_select_bits =   CS1_space;
			compare_value     = OCR1A_space;
		} break;
	}

	TCCR1A =
		(((TIMER1_OC1A_MODE                >> 1) & 1) << COM1A1) |
		(((TIMER1_OC1A_MODE                >> 0) & 1) << COM1A0) |
		(((TIMER1_WAVEFORM_GENERATION_MODE >> 1) & 1) << WGM11 ) |
		(((TIMER1_WAVEFORM_GENERATION_MODE >> 0) & 1) << WGM10 );

	TCCR1B =
		(((TIMER1_WAVEFORM_GENERATION_MODE >> 2) & 1) << WGM12) |
		(((clock_select_bits               >> 2) & 1) << CS12 ) |
		(((clock_select_bits               >> 1) & 1) << CS11 ) |
		(((clock_select_bits               >> 0) & 1) << CS10 );

	OCR1A = compare_value;

	// Reset the timer's counter so the frequency transistion is done in a
	// consistent manner. If we don't do this, the counter might be larger
	// than the new compare value, to which the next compare-match will not
	// occur until after the overflow of the counter.
	TCNT1 = 0;
}

extern noret void
main(void)
{
	GPIO_init();
	USART0_init();

	//////////////////////////////////////////////////////////////// Main ////////////////////////////////////////////////////////////////

	enum Signal current_signal = {0};
	for (;;)
	{
		if (USART0_rx_char(&(char) {0}))
		{
			GPIO_TOGGLE(builtin_led);
		}
		current_signal = current_signal == Signal_mark ? Signal_space : Signal_mark;
		set_signal(current_signal);
		GPIO_TOGGLE(trigger);
		_delay_ms(1.0 / 45.45 * 1000.0);
	}

	for(;;);
}
