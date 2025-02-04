#define nop() __asm__("nop")

static void
delay_nop(u32 count)
{
	for (u32 nop = 0; nop < count; nop += 1)
	{
		nop();
	}
}

static noret void
sorry_(void)
{
	cli();
	for (;;)
	{
		for (u8 i = 0; i < 8; i += 1)
		{
			GPIO_TOGGLE(builtin_led);
			_delay_ms(25.0);
		}
		for (u8 i = 0; i < 16; i += 1)
		{
			GPIO_TOGGLE(builtin_led);
			_delay_ms(15.0);
		}
	}
}
