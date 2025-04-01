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
	//////////////////////////////////////////////////////////////// Initialization ////////////////////////////////////////////////////////////////

	sei(); // Enable interrupts.
	gpio_init();
	USART0_init();

	// Make Timer0 count at a rate of F_CLKIO / 8 = ~16MHz / 8 = ~2 MHz. @/pg 87/tbl 14-9/(328P).
	// We use the overflow to know when a tick has passed, so 256 / (2 MHz) = 128 us.
	TCCR0B = (0 << CS02) | (1 << CS01) | (0 << CS00);
	#define MICROSECONDS_PER_TICK 128

	//////////////////////////////////////////////////////////////// Main ////////////////////////////////////////////////////////////////

	for (;;)
	{
		//
		// Roughly determine elapsed time.
		//

		u16 delta_us = {0};
		{
			// Timer0's counter overflowed? @/pg 88/sec 14.9.7/(328P).
			if (TIFR0 & (1 << TOV0))
			{
				TIFR0    = (1 << TOV0);           // Clear the flag.
				delta_us = MICROSECONDS_PER_TICK; // We know at least this amount of time has passed.
			}
			else
			{
				delta_us = 0; // No tick yet.
			}
		}

		//
		// Get signal with moving-median filter applied.
		//

		b8 signal = {0};
		b8 edge   = {0};
		{
			#define HYSTERESIS              (countof(ring_buffer) / 4)
			#define MICROSECONDS_PER_SAMPLE 128
			static u8  ring_buffer[32] = {0};
			static u8  ring_index      = 0;
			static i16 histogram[2]    = { countof(ring_buffer), 0 };
			static b8  prev_signal     = false;
			static u16 elapsed_us      = 0;

			elapsed_us += delta_us;

			if (elapsed_us >= MICROSECONDS_PER_SAMPLE)
			{
				elapsed_us -= MICROSECONDS_PER_SAMPLE;

				// Move the window update the histogram.
				histogram[ring_buffer[ring_index]] -= 1;
				ring_buffer[ring_index]             = GPIO_READ(signal);
				histogram[ring_buffer[ring_index]] += 1;
				ring_index                         += 1;
				ring_index                         %= countof(ring_buffer);

				// Determine the new signal.
				signal      = histogram[0] < histogram[1] + (prev_signal ? HYSTERESIS : -HYSTERESIS);
				edge        = signal != prev_signal;
				prev_signal = signal;
			}
			else // No update to the signal.
			{
				signal = prev_signal;
				edge   = false;
			}
		}

		//
		// Process the UART data frame.
		//

		enum DataStatus
		{
			DataStatus_none,
			DataStatus_start_bit_error,
			DataStatus_stop_bit_error,
			DataStatus_success,
		};

		enum DataStatus data_status = {0};
		u8              new_data    = 0;
		{
			static u16 elapsed_us = 0;
			static u8  baud_nth   = 0;
			static b8  midpoint   = false;
			static u8  data       = 0;

			elapsed_us += delta_us;

			// Need to find the start bit?
			if (!baud_nth)
			{
				// Falling edge found?
				if (edge && !signal)
				{
					baud_nth   = 1; // Begin to decode the data frame.
					midpoint   = false;
					elapsed_us = 0;
				}
			}

			// Have we began to decode baud symbols?
			if (baud_nth)
			{
				// Are we approximately in the midpoint of the baud symbol?
				if (!midpoint && elapsed_us >= (u16) BAUD_PERIOD_MS * 500)
				{
					midpoint = true;

					// Start bit?
					if (baud_nth == 1)
					{
						// Start bit signal is for some reason high?
						if (signal)
						{
							baud_nth    = 0; // Abort the data frame; might be noise.
							data_status = DataStatus_start_bit_error;
						}
					}
					// Stop bit?
					else if (baud_nth == 10)
					{
						// We can stop early so we'll be immediately ready for the next data frame.
						baud_nth = 0;

						if (signal)
						{
							data_status = DataStatus_success;
							new_data    = data;
						}
						else
						{
							data_status = DataStatus_stop_bit_error;
						}
					}
					// Push the data bit.
					else
					{
						data <<= 1;
						data |= !!signal;
					}
				}
				// We reach end of the baud symbol?
				else if (elapsed_us >= (u16) BAUD_PERIOD_MS * 1000)
				{
					// Repeat again for the next baud symbol.
					baud_nth   += 1;
					elapsed_us -= (u16) BAUD_PERIOD_MS * 1000;
					midpoint    = false;
				}
			}

			GPIO_SET(trigger, !!baud_nth);
		}

		//
		// Handle the data.
		//

		{
			static u8  heartbeat  = 0;
			static u32 elapsed_us = 0;

			elapsed_us += delta_us;

			switch (data_status)
			{
				case DataStatus_none:
				{
					if (elapsed_us >= 1000000)
					{
						elapsed_us  = 0;
						heartbeat  += 1;
						USART0_tx("%u : Nothing new.\n", heartbeat);
					}
				} break;

				case DataStatus_start_bit_error:
				case DataStatus_stop_bit_error:
				{
					heartbeat += 1;
					USART0_tx("%u : Frame error: %u.\n", heartbeat, data_status);
				} break;

				case DataStatus_success:
				{
					elapsed_us  = 0;
					heartbeat  += 1;

					if (32 <= new_data && new_data <= 126)
					{
						USART0_tx("%u : %u : '%c'\n", heartbeat, new_data, new_data);
					}
					else
					{
						USART0_tx("%u : %u\n", heartbeat, new_data);
					}
				} break;
			}
		}
	}
}
