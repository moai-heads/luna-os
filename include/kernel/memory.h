#pragma once

#include <stdbool.h>
#include <stdint.h>

struct limine_memmap_response;

bool memory_init(const struct limine_memmap_response *response);
uint64_t memory_usable_bytes(void);
