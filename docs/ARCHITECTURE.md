# Architecture notes

## Boot contract

Limine is the only bootloader dependency in the first milestone. The kernel asks for:

- base protocol revision support;
- a linear framebuffer;
- the physical memory map; and
- the loaded kernel ELF file.

`src/kernel/main.c` is the single place where boot responses are converted into kernel services. This keeps bootloader-specific structures out of driver code.

## Address-space plan

The initial kernel is linked at 1 MiB and relies on the bootloader's initial mappings. Before enabling user mode, add:

1. a physical-page allocator that reserves the kernel, bootloader reclaimable pages, framebuffer, and modules;
2. a kernel virtual-memory manager with a higher-half mapping;
3. a direct physical-memory map (HHDM) or explicit MMIO mapper; and
4. per-process CR3/page-table construction.

Do not start xHCI DMA until DMA-visible, physically aligned allocations are available.

## Event path

```text
USB xHCI interrupt/completion
        -> USB transfer layer
        -> HID boot/report parser
        -> normalized input_event ring
        -> TUI/GUI focus router
        -> application/input syscall
```

During bring-up, `ps2_poll()` feeds the same `input_event` queue. This makes PS/2 and USB interchangeable at the UI boundary.

## Output path

```text
console_write()
        -> serial COM1 (always)
        -> framebuffer glyph renderer (preferred)
        -> VGA text memory (fallback)
```

The framebuffer code currently supports 32-bit linear framebuffers and uses the bootloader's RGB masks. A later compositor should consume a separate surface API instead of writing directly through the console.

## ELF boundary

`src/elf/loader.c` validates ELF64 little-endian x86-64 images and checks all program-header file ranges. The next step is a loader that:

- allocates pages for `PT_LOAD` segments;
- maps them with W^X permissions;
- zero-fills `p_memsz - p_filesz`;
- builds a user stack and auxiliary vector; and
- enters ring 3 through a controlled syscall/return path.

Linux application compatibility is a later ABI project. ELF format compatibility alone is not Linux syscall or libc compatibility.
