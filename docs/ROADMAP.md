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

## Stage 4 — POSIX execution substrate

- Install GDT/TSS, IDT, exception diagnostics, APIC/timer, and a scheduler.
- Add physical pages, kernel heap, page tables, user/kernel isolation, and ring-3 entry.
- Load ELF64 `PT_LOAD` segments with W^X permissions.
- Define a versioned syscall ABI and implement process, descriptor, memory, file, signal, time, and TTY primitives.

Acceptance: a static user program can start, use memory/files/descriptors, and exit safely.

## Stage 5 — small POSIX userland

- Build `crt1` and `luna-libc`.
- Add an initramfs and `/init`.
- Implement a small shell plus basic utilities: `echo`, `cat`, `ls`, `cp`, `rm`, `mkdir`, `grep`, `uname`, and `sleep`.
- Add TTY line discipline, pipes, redirection, child reaping, signals, and a regression suite.

Acceptance: Luna boots into a keyboard-driven shell from a clean initramfs.

## Stage 6 — persistence and optional compatibility

- Add virtio-blk, FAT32/ext2, `/proc`, permissions, users/groups, and job control.
- Add networking and broader POSIX functionality as required.
- Treat BusyBox as a later integration test, not a prerequisite.

## Stage 7 — GUI/TUI applications

- Make the terminal a user-space service over the input/output ABI.
- Introduce framebuffer surfaces, damage tracking, and a compositor.
- Add pointer focus, keyboard focus, clipboard, and window/event protocols.
- Keep a text-only mode for serial, recovery, and headless systems.
