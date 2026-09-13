#include "kernel/memory.h"
#include "limine.h"

static uint64_t usable_bytes;

bool memory_init(const struct limine_memmap_response *response) {
    usable_bytes = 0;
    if (response == 0 || response->entries == 0) {
        return false;
    }

    for (uint64_t i = 0; i < response->entry_count; ++i) {
        const struct limine_memmap_entry *entry = response->entries[i];
        if (entry != 0 && entry->type == LIMINE_MEMMAP_USABLE) {
            usable_bytes += entry->length;
        }
    }
    return usable_bytes != 0;
}

uint64_t memory_usable_bytes(void) {
    return usable_bytes;
}
