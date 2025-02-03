#include <avr/io.h>
#include "defs.h"
#include "misc.c"

extern int
main(void)
{
	DDRB |= (1 << DDB5);

	for (;;)
	{
		PORTB |= (1 << PORTB5);
		delay_nop(1000000U);
		PORTB &= ~(1 << PORTB5);
		delay_nop(1000000U);
	}

	for(;;);
}
