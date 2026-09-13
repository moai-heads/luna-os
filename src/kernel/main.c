#include <stdbool.h>
#include "limine.h"
#include "elf/elf64.h"
#include "drivers/pci.h"
#include "drivers/ps2.h"
#include "drivers/serial.h"
#include "drivers/usb.h"
#include "kernel/console.h"
#include "kernel/input.h"
#include "kernel/io.h"
#include "kernel/memory.h"

__attribute__((used, section(".limine_requests_start_marker")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests")))
static volatile uint64_t base_revision[] = LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_file_request executable_file_request = {
    .id = LIMINE_EXECUTABLE_FILE_REQUEST_ID,
    .revision = 0,
};

__attribute__((used, section(".limine_requests_end_marker")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

static bool xhci_seen;

static bool pci_boot_callback(const struct pci_device *device, void *context) {
    (void)context;
    if (device->class_code == 0x0c && device->subclass == 0x03) {
        serial_write("PCI: USB HOST CONTROLLER FOUND\n");
        if (device->prog_if == 0x30 && xhci_probe(device)) {
            xhci_seen = true;
        }
    }
    return true;
}

static void draw_boot_ui(void) {
    console_fill_rect(0, 0, 1024, 34, 0x161b22);
    console_fill_rect(0, 38, 1024, 2, 0x238636);
    console_write_at(2, 1, "LUNA OS KERNEL", 0x58a6ff);
    console_write_at(2, 5, "BOOT AND PLATFORM", 0x8b949e);
    console_write_at(2, 10, "INPUT PIPELINE", 0x8b949e);
    console_write_at(2, 15, "EXECUTABLES", 0x8b949e);
}

static void show_input_event(const struct input_event *event) {
    if (event == 0) {
        return;
    }
    if (event->type == INPUT_EVENT_KEY) {
        console_write_at(2, 20, event->value ? "LAST INPUT: KEY DOWN" : "LAST INPUT: KEY UP", 0x3fb950);
    } else if (event->type == INPUT_EVENT_MOTION) {
        console_write_at(2, 20, "LAST INPUT: POINTER MOVE", 0x3fb950);
    } else if (event->type == INPUT_EVENT_BUTTON) {
        console_write_at(2, 20, event->value ? "LAST INPUT: BUTTON DOWN" : "LAST INPUT: BUTTON UP", 0x3fb950);
    }
}

void kmain(void) {
    serial_init();
    serial_write("LUNA OS: ENTERED KERNEL\n");

    if (!LIMINE_BASE_REVISION_SUPPORTED(base_revision)) {
        serial_write("LUNA OS: UNSUPPORTED LIMINE BASE REVISION\n");
        cpu_halt();
    }

    serial_write("LUNA OS: LIMINE REVISION OK\n");
    serial_write("LUNA OS: READING FRAMEBUFFER RESPONSE\n");
    const struct limine_framebuffer *framebuffer = 0;
    if (framebuffer_request.response != 0 && framebuffer_request.response->framebuffer_count != 0) {
        framebuffer = framebuffer_request.response->framebuffers[0];
    }
    serial_write("LUNA OS: FRAMEBUFFER RESPONSE READ\n");
    uint64_t hhdm_offset = 0;
    if (hhdm_request.response != 0) {
        hhdm_offset = hhdm_request.response->offset;
    }
    console_init(framebuffer, hhdm_offset);
    serial_write("LUNA OS: CONSOLE INITIALIZED\n");
    serial_write("LUNA OS: DRAWING BOOT UI\n");
    draw_boot_ui();
    serial_write("LUNA OS: BOOT UI DRAWN\n");
    console_write("LUNA OS: BOOTED X86_64 KERNEL\n");

    if (memory_init(memmap_request.response)) {
        console_write("MEMORY: MAP RECEIVED\n");
    } else {
        console_write("MEMORY: MAP UNAVAILABLE\n");
    }
    console_write("VIDEO: FRAMEBUFFER OR VGA READY\n");

    xhci_seen = false;
    pci_scan(pci_boot_callback, 0);
    console_write(xhci_seen ? "USB: XHCI SKELETON READY\n" : "USB: NO XHCI FOUND\n");

    if (ps2_init()) {
        console_write("INPUT: PS2 KEYBOARD AND MOUSE POLLING\n");
    } else {
        console_write("INPUT: PS2 INIT FAILED\n");
    }
    console_write("INPUT: USB HID PARSER READY\n");

    struct elf64_image kernel_image;
    if (executable_file_request.response != 0 &&
        executable_file_request.response->executable_file != 0 &&
        elf64_validate(executable_file_request.response->executable_file->address,
                       executable_file_request.response->executable_file->size,
                       &kernel_image) == ELF64_OK) {
        console_write("ELF64: KERNEL IMAGE VALID\n");
    } else {
        console_write("ELF64: VALIDATION FAILED\n");
    }
    console_write("READY: POLLING INPUT; USER MODE IS NEXT\n");

    for (;;) {
        ps2_poll();
        xhci_poll();
        struct input_event event;
        while (input_pop(&event)) {
            show_input_event(&event);
        }
        cpu_pause();
    }
}
