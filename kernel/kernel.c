#include <stdint.h>

void kernal_main(void)
{
	volatile uint32_t *vga = (volatile uint32_t *)0xB800;
	*vga = 0x0F530F4F;
}
