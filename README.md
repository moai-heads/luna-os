# Luna OS

A deliberately small x86-64 operating-system kernel scaffold. The first milestone is a bootable kernel with a framebuffer TUI, serial diagnostics, PS/2 bring-up input, PCI discovery, an xHCI/HID driver boundary, and ELF64 validation for future user processes.

This is a **kernel-first scaffold**, not yet a general-purpose operating system. USB transport, scheduling, virtual memory, filesystems, and user mode are intentionally staged behind small interfaces so they can be implemented without rewriting the kernel entry point.

## Current milestone

- Boots through Limine in BIOS or UEFI mode.
- Enters a freestanding x86-64 C kernel with a dedicated stack.
- Requests the bootloader memory map, framebuffer, and executable ELF file.
- Emits logs to COM1 and renders a small framebuffer TUI; VGA text mode is the fallback.
- Polls an 8042 PS/2 keyboard and mouse for early hardware bring-up.
- Scans legacy PCI configuration space and identifies xHCI-class controllers.
- Includes a transport-independent USB HID boot keyboard/mouse report parser.
- Includes checked ELF64 header/program-header validation, but no loader or user mode yet.

## Required inputs and outputs

### Inputs

1. **Boot contract:** Limine supplies the kernel entry, usable/reserved physical-memory map, framebuffer metadata, and the kernel's ELF image.
2. **CPU/platform:** x86-64 long mode, CPUID, paging, interrupt controller, and a timer.
3. **Input hardware:** PCI-discovered USB host controller (xHCI first), USB hubs, HID keyboard, and HID mouse. A PS/2 fallback is kept for early bring-up and virtual machines.
4. **Output hardware:** a linear framebuffer from GOP/VBE, with serial COM1 as a headless/debug path. VGA text memory is a fallback only.
5. **Future program input:** ELF64 bytes loaded from a filesystem or boot module, plus arguments/environment and an initial address-space policy.

### Outputs

1. **Diagnostics:** serial log stream and a panic/halt state.
2. **Human interface:** framebuffer text UI now; later a compositor/window surface API.
3. **Input API:** normalized keyboard, button, and relative-pointer events in a kernel ring buffer.
4. **Future process API:** ELF image validation/loading, virtual address spaces, syscalls, file descriptors, and exit status.

## Minimum driver set

| Area | Minimum implementation | Why |
|---|---|---|
| Boot/platform | Limine protocol, memory map, ACPI hand-off | Establishes safe memory and firmware inputs. |
| CPU | GDT/TSS, IDT, exceptions, `sysret`/`iret` boundary | Required before reliable interrupts or user mode. |
| Interrupts/timers | Local APIC/IOAPIC, PIT or HPET, APIC timer | Drives USB completions, scheduling, and timeouts. |
| Memory | Physical page allocator, page tables, kernel heap | Required by xHCI rings, USB descriptors, processes, and ELF loading. |
| PCI | PCI enumeration and BAR discovery | Finds the USB host controller and later storage/network devices. |
| USB host | xHCI first; EHCI/OHCI/UHCI compatibility later | xHCI covers modern USB 3 controllers; older machines need legacy HCs. |
| USB topology | port reset, device address, hubs, control/bulk/interrupt transfers | Turns a controller into usable USB devices. |
| USB class | HID boot keyboard and HID mouse; HID report descriptors later | Provides the requested keyboard/mouse inputs. |
| Input | event queue, key state, pointer state, focus routing | Stable boundary for TUI now and GUI later. |
| Display | linear framebuffer, pixel format conversion, glyph renderer | Provides the initial TUI and later GUI primitives. |
| Storage/filesystem | virtio-blk or AHCI/NVMe, then FAT32/ext2 | Needed to load programs and persistent data. |
| Executables | ELF64 parser/loader, relocations, user stack, ABI | Makes future Linux-style application ports possible. |

The current tree implements the boot/display/serial/PS/2 pieces and the PCI/xHCI/HID/ELF interfaces. The xHCI controller itself is deliberately not marked complete until physical-page allocation, identity/HHDM mapping, and interrupts exist.

## Build and run

On an Arch Linux build host:

```sh
make
make iso
make run-headless   # serial output, no window
make run            # QEMU graphical window plus serial output
```

The Makefile uses the system `gcc`, `xorriso`, and Limine tools. A cross compiler can be supplied with `make CC=x86_64-elf-gcc` if desired.

QEMU is launched with `qemu-xhci` so the PCI/xHCI discovery path can be exercised. The current kernel reports the controller and HID parser readiness; it does not yet enumerate a real USB device.

## Layout

```text
include/             public kernel and driver interfaces
src/arch/x86_64/     entry point and x86 I/O primitives
src/kernel/          kernel entry, console-independent services, input queue
src/drivers/         serial, framebuffer/VGA console, PCI, PS/2, USB HID/xHCI
src/elf/             ELF64 validation boundary
docs/                architecture, driver plan, and staged roadmap
```

## Design constraints

- Keep hardware drivers behind narrow interfaces and normalize events early.
- Keep the first user ABI ELF64 + x86-64 System V-like calling convention, but do not promise Linux syscall compatibility yet.
- Prefer polling during bring-up; move USB and timers to interrupt-driven operation after IDT/APIC/memory are ready.
- Do not call into libc or assume a hosted runtime. The kernel owns its string/memory primitives.

## License

The Luna OS scaffold is MIT licensed. `include/limine.h` retains its upstream 0BSD notice.
