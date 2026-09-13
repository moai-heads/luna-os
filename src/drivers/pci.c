#include "drivers/pci.h"
#include "kernel/io.h"

#define PCI_CONFIG_ADDRESS 0xcf8
#define PCI_CONFIG_DATA 0xcfc

static uint32_t config_address(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    return 0x80000000u |
           ((uint32_t)bus << 16) |
           ((uint32_t)slot << 11) |
           ((uint32_t)function << 8) |
           (offset & 0xfcu);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    outl(PCI_CONFIG_ADDRESS, config_address(bus, slot, function, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t value = pci_config_read32(bus, slot, function, offset);
    return (uint16_t)(value >> ((offset & 2u) * 8u));
}

uint8_t pci_config_read8(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset) {
    uint32_t value = pci_config_read32(bus, slot, function, offset);
    return (uint8_t)(value >> ((offset & 3u) * 8u));
}

static bool device_present(uint8_t bus, uint8_t slot, uint8_t function) {
    return pci_config_read16(bus, slot, function, 0x00) != 0xffffu;
}

static void read_bars(struct pci_device *device) {
    for (uint8_t i = 0; i < 6; ++i) {
        uint8_t offset = (uint8_t)(0x10u + i * 4u);
        uint32_t low = pci_config_read32(device->bus, device->slot, device->function, offset);
        if (low == 0 || low == 0xffffffffu) {
            device->bar[i] = 0;
            continue;
        }
        if ((low & 1u) != 0) {
            device->bar[i] = low & ~3u;
            continue;
        }
        uint64_t address = low & ~0x0fu;
        uint8_t type = (uint8_t)((low >> 1) & 3u);
        uint8_t bar_index = i;
        if (type == 2 && i < 5) {
            uint32_t high = pci_config_read32(device->bus, device->slot, device->function, (uint8_t)(offset + 4));
            address |= (uint64_t)high << 32;
            device->bar[i + 1] = 0;
            ++i;
        }
        device->bar[bar_index] = address;
    }
}

void pci_scan(pci_device_callback callback, void *context) {
    if (callback == 0) {
        return;
    }
    for (uint32_t bus = 0; bus < 256; ++bus) {
        for (uint32_t slot = 0; slot < 32; ++slot) {
            if (!device_present((uint8_t)bus, (uint8_t)slot, 0)) {
                continue;
            }
            uint8_t header_type = pci_config_read8((uint8_t)bus, (uint8_t)slot, 0, 0x0e);
            uint8_t functions = (header_type & 0x80u) ? 8 : 1;
            for (uint8_t function = 0; function < functions; ++function) {
                if (!device_present((uint8_t)bus, (uint8_t)slot, function)) {
                    continue;
                }
                struct pci_device device = {
                    .bus = (uint8_t)bus,
                    .slot = (uint8_t)slot,
                    .function = function,
                    .vendor_id = pci_config_read16((uint8_t)bus, (uint8_t)slot, function, 0x00),
                    .device_id = pci_config_read16((uint8_t)bus, (uint8_t)slot, function, 0x02),
                    .revision = pci_config_read8((uint8_t)bus, (uint8_t)slot, function, 0x08),
                    .class_code = pci_config_read8((uint8_t)bus, (uint8_t)slot, function, 0x0b),
                    .subclass = pci_config_read8((uint8_t)bus, (uint8_t)slot, function, 0x0a),
                    .prog_if = pci_config_read8((uint8_t)bus, (uint8_t)slot, function, 0x09),
                    .header_type = pci_config_read8((uint8_t)bus, (uint8_t)slot, function, 0x0e),
                };
                read_bars(&device);
                if (!callback(&device, context)) {
                    return;
                }
            }
        }
    }
}
