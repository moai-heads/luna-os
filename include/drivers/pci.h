#pragma once

#include <stdbool.h>
#include <stdint.h>

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t header_type;
    uint64_t bar[6];
};

typedef bool (*pci_device_callback)(const struct pci_device *device, void *context);

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
uint8_t pci_config_read8(uint8_t bus, uint8_t slot, uint8_t function, uint8_t offset);
void pci_scan(pci_device_callback callback, void *context);
