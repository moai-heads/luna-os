#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "drivers/pci.h"

struct xhci_controller {
    struct pci_device pci;
    uint64_t mmio_base;
    uint8_t cap_length;
    uint8_t version_major;
    uint8_t version_minor;
    bool discovered;
    bool initialized;
};

bool xhci_probe(const struct pci_device *device);
const struct xhci_controller *xhci_controller(void);
void xhci_poll(void);

/* These parsers are independent of the eventual xHCI transfer implementation. */
bool usb_hid_keyboard_report(const uint8_t *report, size_t length, uint8_t modifiers);
bool usb_hid_mouse_report(const uint8_t *report, size_t length);
