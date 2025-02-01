#include <avr/io.h>

extern int
main(void)
{
	DDRB |= (1 << DDB5);

	for (;;)
	{
		PORTB |= (1 << PORTB5);
		for (long long i = 0; i < 100000; i += 1);
		PORTB &= ~(1 << PORTB5);
		for (long long i = 0; i < 100000; i += 1);
	}

	for(;;);
}
