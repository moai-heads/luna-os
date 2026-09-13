PROJECT := luna-os
BUILD := build
ISO_ROOT := $(BUILD)/iso_root

CC ?= gcc
LD := $(CC)
OBJCOPY ?= objcopy
QEMU ?= qemu-system-x86_64
LIMINE ?= limine
XORRISO ?= xorriso

CFLAGS := -std=gnu11 -O2 -g \
          -ffreestanding -fno-builtin -fno-stack-protector -fno-stack-check \
          -fPIE -fno-asynchronous-unwind-tables -fno-unwind-tables \
          -m64 -march=x86-64 -mgeneral-regs-only -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -Wall -Wextra -Werror -Wno-unused-parameter \
          -Iinclude
ASFLAGS := -g
LDFLAGS := -nostdlib -pie \
           -Wl,-T,linker.ld -Wl,-z,max-page-size=0x1000 -Wl,-z,text \
           -Wl,--build-id=none
LDLIBS := -lgcc

C_SOURCES := $(shell find src -name '*.c' -print)
ASM_SOURCES := $(shell find src -name '*.S' -print)
C_OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(C_SOURCES))
ASM_OBJECTS := $(patsubst %.S,$(BUILD)/%.o,$(ASM_SOURCES))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

.PHONY: all clean iso run run-headless check

all: $(BUILD)/kernel.elf

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -Iinclude -c $< -o $@

$(BUILD)/kernel.elf: $(OBJECTS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)
	@$(OBJCOPY) --only-keep-debug $@ $(BUILD)/kernel.debug
	@$(OBJCOPY) --strip-debug $@

$(BUILD)/iso_root/boot/kernel.elf: $(BUILD)/kernel.elf
	@mkdir -p $(ISO_ROOT)/boot $(ISO_ROOT)/EFI/BOOT
	cp $< $@
	cp limine.conf $(ISO_ROOT)/limine.conf
	cp /usr/share/limine/limine-bios.sys $(ISO_ROOT)/limine-bios.sys
	cp /usr/share/limine/limine-bios-cd.bin $(ISO_ROOT)/boot/limine-bios-cd.bin
	cp /usr/share/limine/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine-uefi-cd.bin
	cp /usr/share/limine/BOOTX64.EFI $(ISO_ROOT)/EFI/BOOT/BOOTX64.EFI

$(BUILD)/$(PROJECT).iso: $(BUILD)/iso_root/boot/kernel.elf
	$(XORRISO) -as mkisofs -R -r -J \
		-b boot/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 \
		-boot-info-table -hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine-uefi-cd.bin -efi-boot-part \
		--efi-boot-image --protective-msdos-label \
		$(ISO_ROOT) -o $@
	$(LIMINE) bios-install $@

iso: $(BUILD)/$(PROJECT).iso

run: iso
	$(QEMU) -M q35 -m 512M -cdrom $(BUILD)/$(PROJECT).iso \
		-device qemu-xhci,id=xhci \
		-serial stdio -monitor none -display gtk

run-headless: iso
	$(QEMU) -M q35 -m 512M -cdrom $(BUILD)/$(PROJECT).iso \
		-device qemu-xhci,id=xhci \
		-serial stdio -monitor none -display none

check: all
	readelf -h $(BUILD)/kernel.elf
	readelf -l $(BUILD)/kernel.elf

clean:
	rm -rf $(BUILD)

-include $(C_OBJECTS:.o=.d)
