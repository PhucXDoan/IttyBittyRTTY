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

extern noret void
main(void)
{
	GPIO_init();
	USART0_init();

	//////////////////////////////////////////////////////////////// Main ////////////////////////////////////////////////////////////////

	for (;;)
	{
		for (u16 i = 0; i < 256; i += 1)
		{
			USART0_tx("meow! %s=%b\n", "wow!", i);
		}

		GPIO_TOGGLE(builtin_led);
		char data = {0};
		while (!USART0_rx_char(&data));
	}
}
