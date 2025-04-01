#include "gpio.meta"
/*
	with Meta.enter('static void\ngpio_init(void)'):
		for target, gpios in GPIOS:
			with Meta.enter(f'#if {target}'):

				#
				# Preprocessing each GPIO pin.
				#

				for gpio in gpios:
					match gpio.kind:

						#
						# Macros for controlling output pins. @/pg 59/sec 13.2.1/(328P).
						#

						case 'output':
							Meta.overload('GPIO_HIGH'  , [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} |=  (1 << PORT{gpio.port}{gpio.number})))')
							Meta.overload('GPIO_LOW'   , [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} &= ~(1 << PORT{gpio.port}{gpio.number})))')
							Meta.overload('GPIO_TOGGLE', [('NAME', gpio.name),        ], f'((void) (PORT{gpio.port} ^=  (1 << PORT{gpio.port}{gpio.number})))')
							Meta.overload('GPIO_SET'   , [('NAME', gpio.name), 'VALUE'], f'((void) ((VALUE) ? GPIO_HIGH({gpio.name}) : GPIO_LOW({gpio.name})))')

						#
						# Macros for controlling output pins. @/pg 60/sec 13.2.4/(328P).
						#

						case 'input':
							Meta.overload('GPIO_READ', [('NAME', gpio.name)], f'(!!(PIN{gpio.port} & (1 << PIN{gpio.port}{gpio.number})))')

						#
						# Ensure output-compare pin exists.
						#

						case 'output_compare':
							OUTPUT_COMPARE_PINS = {
								('D', 6) : 'OC0A',
								('D', 5) : 'OC0B',
								('B', 1) : 'OC1A',
								('B', 2) : 'OC1B',
								('B', 3) : 'OC2A',
								('D', 3) : 'OC2B',
							}
							assert (gpio.port, gpio.number) in OUTPUT_COMPARE_PINS, \
								f"GPIO {gpio.port}{gpio.number} ({gpio.name}) doesn't correspond to any timer's output-compare pin."

						case unknown:
							assert False, unknown

				#
				# Initialize each GPIO pin.
				#

				for gpio in gpios:
					match gpio.kind:

						case 'output' | 'output_compare': # @/pg 59/sec 13.2.1/(328P).
							Meta.line(f'''
								DDR{gpio.port} |= (1 << DD{gpio.port}{gpio.number});
							''')

						case 'input':
							Meta.line(f'''
								DDR{gpio.port} &= ~(1 << DD{gpio.port}{gpio.number});
							''')

						case unknown:
							assert False, unknown
*/
