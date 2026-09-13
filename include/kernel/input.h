#pragma once

#include <stdbool.h>
#include <stdint.h>

#define INPUT_QUEUE_CAPACITY 128

/* HID usage IDs are used for keyboard codes so USB and PS/2 share one ABI. */
enum input_event_type {
    INPUT_EVENT_KEY = 1,
    INPUT_EVENT_BUTTON = 2,
    INPUT_EVENT_MOTION = 3,
};

enum input_device {
    INPUT_DEVICE_PS2_KEYBOARD = 1,
    INPUT_DEVICE_PS2_MOUSE = 2,
    INPUT_DEVICE_USB_KEYBOARD = 3,
    INPUT_DEVICE_USB_MOUSE = 4,
};

struct input_event {
    uint8_t type;
    uint8_t device;
    uint16_t code;
    int16_t value;
    int16_t x;
    int16_t y;
    uint8_t modifiers;
    uint64_t timestamp;
};

bool input_push(const struct input_event *event);
bool input_pop(struct input_event *event);
uint32_t input_pending(void);
