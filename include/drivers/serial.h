#pragma once

#include <stdbool.h>

bool serial_init(void);
void serial_putc(char character);
void serial_write(const char *text);
