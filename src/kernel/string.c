#include <stddef.h>
#include <stdint.h>

void *memset(void *destination, int value, size_t length) {
    uint8_t *bytes = destination;
    for (size_t i = 0; i < length; ++i) {
        bytes[i] = (uint8_t)value;
    }
    return destination;
}

void *memcpy(void *destination, const void *source, size_t length) {
    uint8_t *out = destination;
    const uint8_t *in = source;
    for (size_t i = 0; i < length; ++i) {
        out[i] = in[i];
    }
    return destination;
}

int memcmp(const void *left, const void *right, size_t length) {
    const uint8_t *a = left;
    const uint8_t *b = right;
    for (size_t i = 0; i < length; ++i) {
        if (a[i] != b[i]) {
            return (a[i] < b[i]) ? -1 : 1;
        }
    }
    return 0;
}

size_t strlen(const char *string) {
    size_t length = 0;
    while (string[length] != '\0') {
        ++length;
    }
    return length;
}
