#pragma once

#include <stdint.h>

struct limine_framebuffer;

void console_init(const struct limine_framebuffer *framebuffer, uint64_t hhdm_offset);
void console_clear(uint32_t rgb);
void console_set_color(uint32_t rgb);
void console_write(const char *text);
void console_putc(char character);
void console_write_at(uint32_t column, uint32_t row, const char *text, uint32_t rgb);
void console_fill_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t rgb);
