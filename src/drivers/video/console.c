#include <stdbool.h>
#include "kernel/console.h"
#include "drivers/serial.h"
#include "kernel/string.h"
#include "limine.h"
#include "font5x7.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define FONT_WIDTH 6
#define FONT_HEIGHT 9

struct console_state {
    volatile uint32_t *framebuffer;
    uint64_t width;
    uint64_t height;
    uint64_t pitch_pixels;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
    uint32_t columns;
    uint32_t rows;
    uint32_t column;
    uint32_t row;
    uint32_t foreground;
    uint32_t background;
    bool framebuffer_active;
};

static struct console_state state;
static volatile uint16_t *vga = (volatile uint16_t *)0xb8000;
static uint32_t vga_column;
static uint32_t vga_row;

static uint32_t mask_for(uint8_t size) {
    if (size == 0) {
        return 0;
    }
    if (size >= 32) {
        return 0xffffffffu;
    }
    return (1u << size) - 1u;
}

static uint32_t pack_rgb(uint32_t rgb) {
    uint32_t red = (rgb >> 16) & 0xffu;
    uint32_t green = (rgb >> 8) & 0xffu;
    uint32_t blue = rgb & 0xffu;
    red = (red * mask_for(state.red_mask_size) + 127u) / 255u;
    green = (green * mask_for(state.green_mask_size) + 127u) / 255u;
    blue = (blue * mask_for(state.blue_mask_size) + 127u) / 255u;
    return (red << state.red_mask_shift) |
           (green << state.green_mask_shift) |
           (blue << state.blue_mask_shift);
}

static const struct font_glyph *find_glyph(char character) {
    if (character >= 'a' && character <= 'z') {
        character = (char)(character - 'a' + 'A');
    }
    for (size_t i = 0; i < sizeof(font_glyphs) / sizeof(font_glyphs[0]); ++i) {
        if (font_glyphs[i].character == character) {
            return &font_glyphs[i];
        }
    }
    return &font_glyphs[1]; /* '?' */
}

static void fb_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!state.framebuffer_active || x >= state.width || y >= state.height) {
        return;
    }
    state.framebuffer[(uint64_t)y * state.pitch_pixels + x] = color;
}

static void draw_glyph(uint32_t x, uint32_t y, char character, uint32_t rgb) {
    const struct font_glyph *glyph = find_glyph(character);
    uint32_t foreground = pack_rgb(rgb);
    uint32_t background = pack_rgb(state.background);
    for (uint32_t gy = 0; gy < 7; ++gy) {
        for (uint32_t gx = 0; gx < 5; ++gx) {
            fb_pixel(x + gx, y + gy, (glyph->rows[gy] & (1u << (4u - gx))) ? foreground : background);
        }
    }
}

static void vga_clear(void) {
    for (uint32_t y = 0; y < VGA_HEIGHT; ++y) {
        for (uint32_t x = 0; x < VGA_WIDTH; ++x) {
            vga[y * VGA_WIDTH + x] = (uint16_t)(' ' | (0x17u << 8));
        }
    }
    vga_column = 0;
    vga_row = 0;
}

static void vga_putc(char character) {
    if (character == '\r') {
        vga_column = 0;
        return;
    }
    if (character == '\n') {
        vga_column = 0;
        ++vga_row;
    } else if (character >= 0x20 && character <= 0x7e) {
        vga[vga_row * VGA_WIDTH + vga_column] = (uint16_t)((uint8_t)character | (0x17u << 8));
        ++vga_column;
        if (vga_column >= VGA_WIDTH) {
            vga_column = 0;
            ++vga_row;
        }
    }
    if (vga_row >= VGA_HEIGHT) {
        vga_clear();
    }
}

void console_init(const struct limine_framebuffer *framebuffer, uint64_t hhdm_offset) {
    state.framebuffer_active = false;
    vga = (volatile uint16_t *)(uintptr_t)(hhdm_offset + 0xb8000u);
    state.foreground = 0xe6edf3;
    state.background = 0x0d1117;
    if (framebuffer != 0 && framebuffer->address != 0 && framebuffer->bpp == 32 &&
        framebuffer->width >= FONT_WIDTH && framebuffer->height >= FONT_HEIGHT) {
        state.framebuffer = (volatile uint32_t *)framebuffer->address;
        state.width = framebuffer->width;
        state.height = framebuffer->height;
        state.pitch_pixels = framebuffer->pitch / sizeof(uint32_t);
        state.red_mask_size = framebuffer->red_mask_size;
        state.red_mask_shift = framebuffer->red_mask_shift;
        state.green_mask_size = framebuffer->green_mask_size;
        state.green_mask_shift = framebuffer->green_mask_shift;
        state.blue_mask_size = framebuffer->blue_mask_size;
        state.blue_mask_shift = framebuffer->blue_mask_shift;
        state.columns = (uint32_t)(state.width / FONT_WIDTH);
        state.rows = (uint32_t)(state.height / FONT_HEIGHT);
        state.column = 0;
        state.row = 0;
        state.framebuffer_active = true;
        console_clear(state.background);
        return;
    }

    vga_clear();
}

void console_clear(uint32_t rgb) {
    state.background = rgb;
    if (state.framebuffer_active) {
        uint32_t color = pack_rgb(rgb);
        for (uint64_t y = 0; y < state.height; ++y) {
            for (uint64_t x = 0; x < state.width; ++x) {
                state.framebuffer[y * state.pitch_pixels + x] = color;
            }
        }
        state.column = 0;
        state.row = 0;
    } else {
        vga_clear();
    }
}

void console_set_color(uint32_t rgb) {
    state.foreground = rgb;
}

void console_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb) {
    if (!state.framebuffer_active) {
        return;
    }
    uint32_t color = pack_rgb(rgb);
    for (uint32_t dy = 0; dy < height && y + dy < state.height; ++dy) {
        for (uint32_t dx = 0; dx < width && x + dx < state.width; ++dx) {
            state.framebuffer[(uint64_t)(y + dy) * state.pitch_pixels + x + dx] = color;
        }
    }
}

void console_putc(char character) {
    serial_putc(character);
    if (!state.framebuffer_active) {
        vga_putc(character);
        return;
    }
    if (character == '\r') {
        state.column = 0;
        return;
    }
    if (character == '\n') {
        state.column = 0;
        ++state.row;
    } else if (character >= 0x20 && character <= 0x7e) {
        draw_glyph(state.column * FONT_WIDTH, state.row * FONT_HEIGHT, character, state.foreground);
        ++state.column;
        if (state.column >= state.columns) {
            state.column = 0;
            ++state.row;
        }
    }
    if (state.row >= state.rows) {
        console_clear(state.background);
    }
}

void console_write(const char *text) {
    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        console_putc(*text++);
    }
}

void console_write_at(uint32_t column, uint32_t row, const char *text, uint32_t rgb) {
    if (!state.framebuffer_active || text == 0 || column >= state.columns || row >= state.rows) {
        return;
    }
    uint32_t x = column;
    while (*text != '\0' && x < state.columns) {
        draw_glyph(x * FONT_WIDTH, row * FONT_HEIGHT, *text++, rgb);
        ++x;
    }
}
