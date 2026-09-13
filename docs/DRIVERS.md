# Driver plan

## Implemented in this scaffold

- **Serial:** COM1 16550-compatible output for headless logs.
- **Console:** 32-bit linear framebuffer glyph output, with VGA text fallback.
- **PCI:** legacy configuration-space enumeration and BAR/class identification.
- **PS/2:** polling keyboard set-1 decoding and three-byte mouse packets.
- **USB HID parser:** transport-independent boot keyboard and mouse report parsing.
- **xHCI boundary:** controller discovery and state record; no DMA rings yet.
- **ELF64 validation:** safe header/program-header checks.

## Next driver work, in dependency order

1. IDT, exception diagnostics, PIC disable, APIC/IOAPIC, and an APIC timer.
2. Physical-page allocator and MMIO mapping.
3. xHCI capability/operational register mapping, command/event rings, device contexts,
   port state changes, slot enable, address-device, and control transfers.
4. USB descriptors, hub traversal, interrupt endpoints, and HID report descriptors.
5. Replace PS/2 polling with IRQs, but retain it as a fallback.
6. Storage: virtio-blk first for QEMU, then AHCI/NVMe; FAT32 first for boot media.
7. Scheduler, user page tables, syscall entry, and ELF segment loading.
8. A small window/compositor layer on top of the framebuffer surface.

## Why xHCI first

Modern x86 PCs commonly expose a USB 3 xHCI controller that owns USB 2 and USB 3 ports. Supporting xHCI first gives one controller model for the QEMU target and current hardware. UHCI/OHCI/EHCI can be added as compatibility drivers if older machines or unusual firmware require them.
