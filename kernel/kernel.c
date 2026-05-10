#include <stdint.h>

typedef struct {
    uint64_t framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
} BootInfo;

void kernel_main(BootInfo *info)
{
    volatile uint32_t *fb = (volatile uint32_t *)info->framebuffer;

    // Fill screen black
    for (uint32_t i = 0; i < info->height * info->pixels_per_scanline; i++) {
        fb[i] = 0x000000;
    }

    // Draw a white rectangle in the top left
    for (uint32_t y = 10; y < 50; y++) {
        for (uint32_t x = 10; x < 300; x++) {
            fb[y * info->pixels_per_scanline + x] = 0xFFFFFF;
        }
    }

    while(1) {}
}
