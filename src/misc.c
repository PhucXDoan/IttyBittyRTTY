#define nop() __asm__("nop")

static void
delay_nop(u32 count)
{
	for (u32 nop = 0; nop < count; nop += 1)
	{
		nop();
	}
}
