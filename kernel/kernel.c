#include <stdint.h>
#include "font.h"

typedef struct {
    uint64_t framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
} BootInfo;

void draw_char(volatile uint32_t *fb, uint32_t pixels_per_scanline,
               int x, int y, char c, uint32_t color)
{
    const uint8_t *glyph = font[(uint8_t)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col)) {
                fb[(y + row) * pixels_per_scanline + (x + col)] = color;
            }
        }
    }
}

void kernel_main(BootInfo *info)
{
    volatile uint32_t *fb = (volatile uint32_t *)info->framebuffer;

    for (uint32_t i = 0; i < info->height * info->pixels_per_scanline; i++) {
        fb[i] = 0x000000;
    }

    const char *msg = "SimplrOS";
    for (int i = 0; msg[i]; i++) {
        draw_char(fb, info->pixels_per_scanline, i * 9, 10, msg[i], 0xFFFFFF);
    }

    while(1) {}
}
