#include "drivers/usb.h"
#include "kernel/input.h"

static uint8_t previous_keyboard_keys[6];
static uint8_t previous_mouse_buttons;

static bool contains_key(const uint8_t *keys, uint8_t key) {
    for (uint8_t i = 0; i < 6; ++i) {
        if (keys[i] == key) {
            return true;
        }
    }
    return false;
}

static void push_key(uint16_t code, bool pressed, uint8_t modifiers) {
    if (code == 0) {
        return;
    }
    struct input_event event = {
        .type = INPUT_EVENT_KEY,
        .device = INPUT_DEVICE_USB_KEYBOARD,
        .code = code,
        .value = pressed ? 1 : 0,
        .modifiers = modifiers,
    };
    (void)input_push(&event);
}

bool usb_hid_keyboard_report(const uint8_t *report, size_t length, uint8_t modifiers) {
    if (report == 0 || length < 8) {
        return false;
    }
    const uint8_t *keys = report + 2;
    for (uint8_t i = 0; i < 6; ++i) {
        if (keys[i] != 0 && !contains_key(previous_keyboard_keys, keys[i])) {
            push_key(keys[i], true, modifiers);
        }
    }
    for (uint8_t i = 0; i < 6; ++i) {
        if (previous_keyboard_keys[i] != 0 && !contains_key(keys, previous_keyboard_keys[i])) {
            push_key(previous_keyboard_keys[i], false, modifiers);
        }
    }
    for (uint8_t i = 0; i < 6; ++i) {
        previous_keyboard_keys[i] = keys[i];
    }
    return true;
}

bool usb_hid_mouse_report(const uint8_t *report, size_t length) {
    if (report == 0 || length < 3) {
        return false;
    }
    uint8_t buttons = report[0] & 0x07u;
    int16_t dx = (int8_t)report[1];
    int16_t dy = -(int8_t)report[2];
    if (dx != 0 || dy != 0) {
        struct input_event motion = {
            .type = INPUT_EVENT_MOTION,
            .device = INPUT_DEVICE_USB_MOUSE,
            .x = dx,
            .y = dy,
        };
        (void)input_push(&motion);
    }
    uint8_t changed = previous_mouse_buttons ^ buttons;
    for (uint8_t button = 0; button < 3; ++button) {
        if ((changed & (1u << button)) != 0) {
            struct input_event event = {
                .type = INPUT_EVENT_BUTTON,
                .device = INPUT_DEVICE_USB_MOUSE,
                .code = (uint16_t)(1u + button),
                .value = (buttons & (1u << button)) ? 1 : 0,
            };
            (void)input_push(&event);
        }
    }
    previous_mouse_buttons = buttons;
    return true;
}
