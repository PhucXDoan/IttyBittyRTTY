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

static void
set_signal(enum Signal signal)
{
	//
	// In "Clear Timer on Compare Match" mode (CTC), Timer1's counter can be modulated by
	// OCR1A. That is, the counter goes from zero and up but will reset to zero after a
	// compare match to OCR1A, thus never becoming greater than OCR1A (but can be equal to it though).
	// @/pg 97/sec 15.7/(328P).
	// @/pg 109/tbl 15-5/(328P).
	//

	#define TIMER1_WAVEFORM_GENERATION_MODE 0b0100

	//
	// Configure the OC1A pin to toggle on each compare-match of OCR1A. @/pg 108/sec 15.11.1/(328P).
	//

	#define TIMER1_OC1A_MODE 0b01

	//
	// Configure the registers.
	//

	TCCR1A =
		(((TIMER1_OC1A_MODE                >> 1) & 1) << COM1A1) |
		(((TIMER1_OC1A_MODE                >> 0) & 1) << COM1A0) |
		(((TIMER1_WAVEFORM_GENERATION_MODE >> 1) & 1) << WGM11 ) |
		(((TIMER1_WAVEFORM_GENERATION_MODE >> 0) & 1) << WGM10 );

	TCCR1B =
		(((TIMER1_WAVEFORM_GENERATION_MODE >> 2) & 1) << WGM12) |
		(((SIGNAL_TABLE[signal].clksel     >> 2) & 1) << CS12 ) |
		(((SIGNAL_TABLE[signal].clksel     >> 1) & 1) << CS11 ) |
		(((SIGNAL_TABLE[signal].clksel     >> 0) & 1) << CS10 );

	OCR1A = SIGNAL_TABLE[signal].compare_value;

	//
	// Reset the timer's counter so the frequency transistion is done in a
	// consistent manner. If we don't do this, the counter might be larger
	// than the new compare value, to which the next compare-match will not
	// occur until after the overflow of the counter.
	//

	TCNT1 = 0;
}

extern noret void
main(void)
{
	//////////////////////////////////////////////////////////////// Initialization ////////////////////////////////////////////////////////////////

	sei(); // Enable interrupts.
	gpio_init();
	USART0_init();

	//////////////////////////////////////////////////////////////// Main ////////////////////////////////////////////////////////////////

	#if 1
		set_signal(Signal_mark); // TODO A way to resynchronize?
		_delay_ms(100.0);

		str message = str("Doing taxes suck!");
		for (;;)
		{
			// Data frames.
			for (u8 i = 0; i < message.len; i += 1)
			{
				// Start bit.
				set_signal(Signal_space);
				_delay_ms(BAUD_PERIOD_MS);

				// Data bits.
				for
				(
					i8 j = bitsof(message.data[i]) - 1;
					j >= 0;
					j -= 1
				)
				{
					if (message.data[i] & (1 << j))
					{
						set_signal(Signal_mark);
					}
					else
					{
						set_signal(Signal_space);
					}

					_delay_ms(BAUD_PERIOD_MS);
				}

				// Stop bit.
				set_signal(Signal_mark);
				_delay_ms(BAUD_PERIOD_MS);
			}
		}
	#else
		enum Signal curr_signal = Signal_none;
		for (;;)
		{
			char input = {0};
			while (!USART0_rx_char(&input));
			USART0_tx("%c", input);
			curr_signal = curr_signal == Signal_mark ? Signal_space : Signal_mark;
			set_signal(curr_signal);
		}
	#endif
}
