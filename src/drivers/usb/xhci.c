#include "drivers/usb.h"
#include "drivers/pci.h"
#include "drivers/serial.h"

static struct xhci_controller controller;

bool xhci_probe(const struct pci_device *device) {
    if (device == 0 || device->class_code != 0x0c || device->subclass != 0x03 ||
        device->prog_if != 0x30) {
        return false;
    }
    controller.pci = *device;
    controller.mmio_base = device->bar[0] & ~0x0full;
    controller.discovered = true;
    controller.initialized = false;
    serial_write("USB: XHCI CONTROLLER DISCOVERED\n");
    serial_write("USB: DMA RINGS AND ENUMERATION ARE NEXT\n");
    return true;
}

const struct xhci_controller *xhci_controller(void) {
    return &controller;
}

void xhci_poll(void) {
    /* TODO: map capability/operational registers, run the event ring, and enumerate. */
}
