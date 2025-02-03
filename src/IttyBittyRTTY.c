#include <avr/io.h>
#include "defs.h"
#include "misc.c"
#include "gpio.c"

extern int
main(void)
{
	GPIO_init();

	for (;;)
	{
		GPIO_TOGGLE(buildin_led);
		delay_nop(1000000U);
	}

	for(;;);
}
