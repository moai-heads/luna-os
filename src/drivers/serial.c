#include "drivers/serial.h"
#include "kernel/io.h"

#define COM1 0x3f8

bool serial_init(void) {
    outb(COM1 + 1, 0x00); /* Disable interrupts. */
    outb(COM1 + 3, 0x80); /* Enable divisor latch. */
    outb(COM1 + 0, 0x03); /* 38400 baud divisor. */
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03); /* 8 data bits, no parity, one stop bit. */
    outb(COM1 + 2, 0xc7); /* Enable FIFO, clear it, 14-byte threshold. */
    outb(COM1 + 4, 0x0b); /* IRQs enabled, RTS/DSR set. */
    return true;
}

void serial_putc(char character) {
    if (character == '\n') {
        serial_putc('\r');
    }
    for (uint32_t i = 0; i < 100000; ++i) {
        if ((inb(COM1 + 5) & 0x20) != 0) {
            outb(COM1, (uint8_t)character);
            return;
        }
        cpu_pause();
    }
}

void serial_write(const char *text) {
    if (text == 0) {
        return;
    }
    while (*text != '\0') {
        serial_putc(*text++);
    }
}
