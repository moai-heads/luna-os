#include "drivers/ps2.h"
#include "kernel/input.h"
#include "kernel/io.h"

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

static bool mouse_present;
static uint8_t mouse_packet[3];
static uint8_t mouse_packet_index;
static uint8_t mouse_buttons;
static bool extended_scancode;

static bool wait_input_clear(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if ((inb(PS2_STATUS) & 2u) == 0) {
            return true;
        }
        cpu_pause();
    }
    return false;
}

static bool wait_output_full(void) {
    for (uint32_t i = 0; i < 100000; ++i) {
        if ((inb(PS2_STATUS) & 1u) != 0) {
            return true;
        }
        cpu_pause();
    }
    return false;
}

static void mouse_write(uint8_t value) {
    if (!wait_input_clear()) {
        return;
    }
    outb(PS2_COMMAND, 0xd4);
    if (wait_input_clear()) {
        outb(PS2_DATA, value);
    }
}

static uint16_t scancode_to_hid(uint8_t code) {
    switch (code) {
        case 0x01: return 0x29; /* Escape */
        case 0x02: return 0x1e; case 0x03: return 0x1f; case 0x04: return 0x20;
        case 0x05: return 0x21; case 0x06: return 0x22; case 0x07: return 0x23;
        case 0x08: return 0x24; case 0x09: return 0x25; case 0x0a: return 0x26;
        case 0x0b: return 0x27; case 0x0c: return 0x2d; case 0x0d: return 0x2e;
        case 0x0e: return 0x2a; case 0x0f: return 0x2b; case 0x10: return 0x14;
        case 0x11: return 0x1a; case 0x12: return 0x08; case 0x13: return 0x15;
        case 0x14: return 0x17; case 0x15: return 0x1c; case 0x16: return 0x18;
        case 0x17: return 0x0c; case 0x18: return 0x12; case 0x19: return 0x13;
        case 0x1a: return 0x2f; case 0x1b: return 0x30; case 0x1c: return 0x28;
        case 0x1d: return 0xe1; case 0x1e: return 0x04; case 0x1f: return 0x16;
        case 0x20: return 0x07; case 0x21: return 0x09; case 0x22: return 0x0a;
        case 0x23: return 0x0b; case 0x24: return 0x0d; case 0x25: return 0x0e;
        case 0x26: return 0x0f; case 0x27: return 0x33; case 0x28: return 0x34;
        case 0x29: return 0x35; case 0x2a: return 0xe1; case 0x2b: return 0x31;
        case 0x2c: return 0x1d; case 0x2d: return 0x1b; case 0x2e: return 0x06;
        case 0x2f: return 0x19; case 0x30: return 0x05; case 0x31: return 0x11;
        case 0x32: return 0x10; case 0x33: return 0x36; case 0x34: return 0x37;
        case 0x35: return 0x38; case 0x36: return 0xe5; case 0x37: return 0x55;
        case 0x38: return 0xe2; case 0x39: return 0x2c; case 0x3a: return 0x39;
        case 0x3b: return 0x3a; case 0x3c: return 0x3b; case 0x3d: return 0x3c;
        case 0x3e: return 0x3d; case 0x3f: return 0x3e; case 0x40: return 0x3f;
        case 0x41: return 0x40; case 0x42: return 0x41; case 0x43: return 0x42;
        case 0x44: return 0x43; case 0x45: return 0x53; case 0x46: return 0x47;
        case 0x47: return 0x5f; case 0x48: return 0x60; case 0x49: return 0x61;
        case 0x4a: return 0x56; case 0x4b: return 0x5c; case 0x4c: return 0x5d;
        case 0x4d: return 0x5e; case 0x4e: return 0x57; case 0x4f: return 0x59;
        case 0x50: return 0x5a; case 0x51: return 0x5b; case 0x52: return 0x62;
        case 0x53: return 0x63; case 0x57: return 0x44; case 0x58: return 0x45;
        default: return 0;
    }
}

static void emit_key(uint16_t code, bool pressed) {
    if (code == 0) {
        return;
    }
    struct input_event event = {
        .type = INPUT_EVENT_KEY,
        .device = INPUT_DEVICE_PS2_KEYBOARD,
        .code = code,
        .value = pressed ? 1 : 0,
    };
    (void)input_push(&event);
}

static void handle_keyboard_byte(uint8_t byte) {
    if (byte == 0xe0) {
        extended_scancode = true;
        return;
    }
    bool released = (byte & 0x80u) != 0;
    uint8_t code = byte & 0x7fu;
    uint16_t hid = scancode_to_hid(code);
    if (extended_scancode) {
        switch (code) {
            case 0x1c: hid = 0x58; break; /* Keypad enter. */
            case 0x35: hid = 0x54; break; /* Keypad slash. */
            case 0x48: hid = 0x52; break; /* Up. */
            case 0x50: hid = 0x51; break; /* Down. */
            case 0x4b: hid = 0x50; break; /* Left. */
            case 0x4d: hid = 0x4f; break; /* Right. */
            case 0x53: hid = 0x4c; break; /* Delete. */
            default: break;
        }
    }
    emit_key(hid, !released);
    extended_scancode = false;
}

static void handle_mouse_byte(uint8_t byte) {
    if (mouse_packet_index == 0 && (byte & 0x08u) == 0) {
        return; /* Re-synchronise to the first packet byte. */
    }
    mouse_packet[mouse_packet_index++] = byte;
    if (mouse_packet_index != 3) {
        return;
    }
    mouse_packet_index = 0;
    int16_t dx = (int8_t)mouse_packet[1];
    int16_t dy = -(int8_t)mouse_packet[2];
    uint8_t buttons = mouse_packet[0] & 0x07u;
    if (dx != 0 || dy != 0) {
        struct input_event motion = {
            .type = INPUT_EVENT_MOTION,
            .device = INPUT_DEVICE_PS2_MOUSE,
            .x = dx,
            .y = dy,
        };
        (void)input_push(&motion);
    }
    uint8_t changed = buttons ^ mouse_buttons;
    for (uint8_t button = 0; button < 3; ++button) {
        if ((changed & (1u << button)) != 0) {
            struct input_event event = {
                .type = INPUT_EVENT_BUTTON,
                .device = INPUT_DEVICE_PS2_MOUSE,
                .code = (uint16_t)(1u + button),
                .value = (buttons & (1u << button)) ? 1 : 0,
            };
            (void)input_push(&event);
        }
    }
    mouse_buttons = buttons;
}

bool ps2_init(void) {
    /* Disable both ports while the controller is configured. */
    if (wait_input_clear()) outb(PS2_COMMAND, 0xad);
    if (wait_input_clear()) outb(PS2_COMMAND, 0xa7);

    if (wait_input_clear()) outb(PS2_COMMAND, 0x20);
    uint8_t config = wait_output_full() ? inb(PS2_DATA) : 0;
    config |= 0x02;  /* Enable keyboard IRQ bit for eventual interrupt mode. */
    config &= (uint8_t)~0x20; /* Enable mouse clock. */
    if (wait_input_clear()) outb(PS2_COMMAND, 0x60);
    if (wait_input_clear()) outb(PS2_DATA, config);

    if (wait_input_clear()) outb(PS2_COMMAND, 0xae);
    if (wait_input_clear()) outb(PS2_COMMAND, 0xa8);

    mouse_write(0xf6); /* Set defaults. */
    if (wait_output_full()) (void)inb(PS2_DATA);
    mouse_write(0xf4); /* Enable data reporting. */
    if (wait_output_full()) (void)inb(PS2_DATA);
    mouse_present = true;
    return true;
}

void ps2_poll(void) {
    while ((inb(PS2_STATUS) & 1u) != 0) {
        uint8_t status = inb(PS2_STATUS);
        uint8_t byte = inb(PS2_DATA);
        if ((status & 0x20u) != 0 && mouse_present) {
            handle_mouse_byte(byte);
        } else {
            handle_keyboard_byte(byte);
        }
    }
}
