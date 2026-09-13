#pragma once

#include <stddef.h>

void *memset(void *destination, int value, size_t length);
void *memcpy(void *destination, const void *source, size_t length);
int memcmp(const void *left, const void *right, size_t length);
size_t strlen(const char *string);
