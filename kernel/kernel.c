#include <stdint.h>
#include "font.h"

typedef struct {
    uint64_t framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pixels_per_scanline;
} BootInfo;

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static const char scancode_map[128] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    0, 'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};

char read_key(void)
{
    uint8_t status;
    uint8_t scancode;

    // Wait until keyboard buffer has data
    do {
        status = inb(0x64);
    } while ((status & 0x01) == 0);

    scancode = inb(0x60);

    // Ignore key releases (high bit set)
    if (scancode & 0x80) return 0;

    // Look up ASCII value
    if (scancode < 128)
        return scancode_map[scancode];

    return 0;
}

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

    // Fill screen black
    for (uint32_t i = 0; i < info->height * info->pixels_per_scanline; i++) {
        fb[i] = 0x000000;
    }

    // Draw header
    const char *msg = "SimplrOS";
    for (int i = 0; msg[i]; i++) {
        draw_char(fb, info->pixels_per_scanline, i * 9, 10, msg[i], 0xFFFFFF);
    }

    // Input loop
    int cursor_x = 0;
    int cursor_y = 30;

    while(1) {
        char c = read_key();
        if (c == '\n') {
            cursor_x = 0;
            cursor_y += 10;
        } else if (c == '\b') {
            if (cursor_x > 0) {
                cursor_x--;
                // Erase the character by drawing a black rectangle
                for (int row = 0; row < 8; row++) {
                    for (int col = 0; col < 8; col++) {
                        fb[(cursor_y + row) * info->pixels_per_scanline + (cursor_x * 9 + col)] = 0x000000;
                    }
                }
            }
        } else if (c != 0) {
            draw_char(fb, info->pixels_per_scanline,
                      cursor_x * 9, cursor_y, c, 0x00FF00);
            cursor_x++;
            if (cursor_x > 80) {
                cursor_x = 0;
                cursor_y += 10;
            }
        }
    }
}
