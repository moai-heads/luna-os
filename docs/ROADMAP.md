# Roadmap

## Stage 0 — bootable kernel (current)

Acceptance: Limine starts an x86-64 ELF kernel; serial and framebuffer/VGA output work; the memory map is received; PCI finds QEMU xHCI; PS/2 events reach the normalized input queue; the kernel ELF validates.

## Stage 1 — safe kernel substrate

- Install exception handlers with register dumps.
- Add GDT/TSS and a real IDT.
- Disable legacy PIC after APIC/IOAPIC discovery.
- Add a physical page allocator, kernel heap, and HHDM/MMIO mapping.
- Add PIT/HPET or APIC timer and a monotonic clock.

## Stage 2 — real USB input

- Implement xHCI capability and operational register discovery.
- Allocate DMA-safe command, event, and transfer rings.
- Handle port status changes, slot allocation, device addressing, and control transfers.
- Read device/configuration/interface/endpoint descriptors.
- Enumerate hubs and bind HID boot keyboard/mouse interfaces.
- Route interrupt-transfer completions into `input_event`.

Acceptance: a USB keyboard types into the TUI and a USB mouse moves a visible pointer in QEMU.

## Stage 3 — persistent storage

- Start with virtio-blk for the QEMU target.
- Add a block cache and FAT32 reader for the boot volume.
- Add a simple initramfs/module path so the kernel can boot without a disk driver.
- Later add AHCI/NVMe for physical hardware.

## Stage 4 — processes and ELF

- Build per-process address spaces and a scheduler.
- Load `PT_LOAD` segments with W^X permissions.
- Create a user stack, aux vector, TLS policy, and ring-3 entry/return path.
- Implement a small syscall ABI for console, input, memory, files, and process exit.
- Add a freestanding user libc, then port small Linux applications by replacing their syscall/libc assumptions.

Compatibility target: ELF64 executable format first; Linux ABI compatibility only where intentionally implemented.

## Stage 5 — GUI/TUI applications

- Make the terminal a user-space service over the input/output ABI.
- Introduce framebuffer surfaces, damage tracking, and a compositor.
- Add pointer focus, keyboard focus, clipboard, and window/event protocols.
- Keep a text-only mode for serial, recovery, and headless systems.
