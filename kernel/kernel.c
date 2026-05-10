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

static int shift_pressed = 0;

static const char scancode_map[128] = {
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    0, 'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};

static const char scancode_map_shift[128] = {
    0, 0, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    0, 'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' '
};

char read_key(void)
{
    uint8_t status;
    uint8_t scancode;

    do {
        status = inb(0x64);
    } while ((status & 0x01) == 0);

    scancode = inb(0x60);

    // Left shift press=0x2A release=0xAA
    // Right shift press=0x36 release=0xB6
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return 0;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        return 0;
    }

    if (scancode & 0x80) return 0;

    if (scancode < 128) {
        if (shift_pressed)
            return scancode_map_shift[scancode];
        else
            return scancode_map[scancode];
    }

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
