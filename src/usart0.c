static void
USART0_init(void)
{
	//
	// Determine the values to write to the in registers to get the desired baud rate.
	//

	#include "USART0_baud_configurer.meta"
	/*
		for u2x0, ubrr0 in (
			(0, F_OSC / (16 * USART0_BAUD) - 1), # Assuming we don't need the U2Xn bit. @/pg 146/tbl 19-1/(328P).
			(1, F_OSC / (8  * USART0_BAUD) - 1), # Assuming we       need the U2Xn bit. "
		):
			# Calculated value fits wthin UBBR0? @/pg 162/sec 19.10.5/(328P).
			if ubrr0.is_integer() and 0 <= ubrr0 < (1 << 12):
				Meta.define('USART0_U2X0_init' , u2x0      )
				Meta.define('USART0_UBBR0_init', int(ubrr0))
				break
		else:
			assert False, f'Desired USART0 baud rate of {USART0_BAUD} is not achievable.'
	*/

	//
	// If needed, double the transmission speed. @/pg 159/sec 19.10.2/(328P).
	//

	UCSR0A = (USART0_U2X0_init << U2X0);

	//
	// Set the divider to achieve the desired baud rate. @/pg 146/tbl 19-1/(328P).
	//

	UBRR0 = USART0_UBBR0_init;

	UCSR0C =
		(0 << UPM01 ) | (0 << UPM00 ) | // No parity bit. @/pg 161/tbl 19-5/(328P).
		(1 << UCSZ01) | (1 << UCSZ00);  // Data size of 8 bits. @/pg 162/tbl 19-7/(328P).

	//
	// Enable transmission and reception of data. @/pg 171/sec 20.8.3/(328P).
	//

	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
}

#define USART0_tx(...) STR_fmt_builder(_USART0_tx_callback, 0, __VA_ARGS__)
static enum StrFmtBuilderCallbackResult
_USART0_tx_callback(void* context, char* data, u16 len)
{
	for (u16 i = 0; i < len; i += 1)
	{
		// Wait for there to be space available to push data. @/pg 171/sec 20.8.2/(328P).
		while (!(UCSR0A & (1 << UDRE0)));

		// Push the data to be transmitted. @/pg 159/sec 19.10.1/(328P).
		UDR0 = data[i];
	}

	return StrFmtBuilderCallbackResult_continue;
}

static useret b8          // Data available?
USART0_rx_char(char* dst) // '\r' can be received in response to ENTER, and certain keys (e.g. arrows) will send multi-byte ANSI escape sequences.
{
	b8 available = false;

	// Data available to be popped? @/pg 159/sec 19.10.2/(328P).
	if (UCSR0A & (1 << RXC0))
	{
		// Some errors that could be handled, but we don't really need to.
		#if 0
			// We missed some data? @/pg 159/sec 19.10/(328P).
			if (UCSR0A & (1 << DOR0))
			{
				// Don't care.
			}

			// The data in the data register had a frame error? @/pg 159/sec 19.10.2/(328P).
			if (UCSR0A & (1 << FE0))
			{
				// Don't care.
			}

			// The data in the data register had a parity error? @/pg 159/sec 19.10.2/(328P).
			if (UCSR0A & (1 << UPE0))
			{
				// Don't care.
			}
		#endif

		// Pop the data that was received if desired. @/pg 159/sec 19.10.1/(328P).
		if (dst)
		{
			*dst = UDR0;
		}

		available = true;
	}

	return available;
}
