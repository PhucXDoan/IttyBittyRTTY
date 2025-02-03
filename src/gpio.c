#include "GPIO.meta"
/*
	for gpio in GPIOS:

		# Macros to control output GPIO pins. @/pg 59/sec 13.2.1/(328P).
		if gpio.type == 'output':
			Meta.overload('GPIO_HIGH'  , [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} &= ~(1 << PORT{gpio.pin})))')
			Meta.overload('GPIO_LOW'   , [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} |= (1 << PORT{gpio.pin})))')
			Meta.overload('GPIO_TOGGLE', [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} ^= (1 << PORT{gpio.pin})))')
			Meta.overload('GPIO_SET'   , [('NAME', gpio.name), 'VALUE'], f'((void) ((VALUE) ? GPIO_HIGH({gpio.name}) : GPIO_LOW({gpio.name})))')

	with Meta.enter('extern void\nGPIO_init(void)'):

		# Set GPIO's data direction (output/input). @/pg 59/sec 13.2.1/(328P).
		for gpio in GPIOS:
			if gpio.type == 'output':
				Meta.line(f'''
					DDR{gpio.port} |= (1 << DD{gpio.port}{gpio.number});
				''')
*/

/* #meta GPIO
/*
	def GPIO(name, pin, type):

		gpio = Meta.Obj(
			name   = name,
			pin    = pin,
			port   = pin[0],
			number = int(pin[1:]),
			type   = type,
		)

		match type:

			case 'output':
				pass

			# Unknown GPIO type.
			case unknown: assert False, unknown

		return gpio
*/
