#include <avr/io.h>
#include <util/delay.h>
#include "defs.h"
#include "misc.c"
#include "gpio.c"
#include "usart0.c"

extern noret void
main(void)
{
	GPIO_init();
	USART0_init();

	//////////////////////////////////////////////////////////////// Main ////////////////////////////////////////////////////////////////

	for (;;)
	{
		GPIO_TOGGLE(buildin_led);
		char data = {0};
		while (!USART0_rx_char(&data));
		USART0_tx_char(data);
	}
}
