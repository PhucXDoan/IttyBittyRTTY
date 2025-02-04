#include "GPIO.meta"
/*
	for gpio in GPIOS:
		# Macros to control output GPIO pins. @/pg 59/sec 13.2.1/(328P).
		if gpio.func == 'output':
			Meta.overload('GPIO_HIGH'  , [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} &= ~(1 << PORT{gpio.pin})))')
			Meta.overload('GPIO_LOW'   , [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} |= (1 << PORT{gpio.pin})))')
			Meta.overload('GPIO_TOGGLE', [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} ^= (1 << PORT{gpio.pin})))')
			Meta.overload('GPIO_SET'   , [('NAME', gpio.name), 'VALUE'], f'((void) ((VALUE) ? GPIO_HIGH({gpio.name}) : GPIO_LOW({gpio.name})))')

	with Meta.enter('static void\nGPIO_init(void)'):

		for gpio in GPIOS:

			# Make the GPIO be an output pin. @/pg 59/sec 13.2.1/(328P).
			if gpio.func == 'output' or gpio.func in OUTPUT_COMPARE_PINS:
				Meta.line(f'''
					DDR{gpio.port} |= (1 << DD{gpio.port}{gpio.number});
				''')
*/

/* #meta GPIO, OUTPUT_COMPARE_PINS
/*
	#
	# @/pg 3/fig 1-1/(328P).
	#

	OUTPUT_COMPARE_PINS = {
		'OC0A' : 'D6',
		'OC0B' : 'D5',
		'OC1A' : 'B1',
		'OC1B' : 'B2',
		'OC2A' : 'B3',
		'OC2B' : 'D3',
	}

	#
	# GPIO constructor.
	#

	def GPIO(name, pin, func):

		gpio = Meta.Obj(
			name   = name,
			pin    = pin,
			port   = pin[0],
			number = int(pin[1:]),
			func   = func,
		)

		match func:

			# Pin will be used as a digital output.
			case 'output':
				pass

			# Pin will be used as the waveform generator's output. @/pg 76/fig 14-3/(328P).
			case output_compare if output_compare in OUTPUT_COMPARE_PINS:
				assert OUTPUT_COMPARE_PINS[output_compare] == pin, \
					f'GPIO {pin} (`{name}`) is not associated with output compare {output_compare}' \
					f' (which is actually for pin {OUTPUT_COMPARE_PINS[output_compare]}).'

			# Don't know!
			case unknown:
				assert False, \
					f'GPIO {pin} (`{name}`) has unknown function: `{unknown}`.'

		return gpio
*/
