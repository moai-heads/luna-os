#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ELF64_IDENT_SIZE 16
#define ELF64_MAGIC0 0x7f
#define ELF64_MAGIC1 'E'
#define ELF64_MAGIC2 'L'
#define ELF64_MAGIC3 'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define EM_X86_64 62
#define ET_EXEC 2
#define ET_DYN 3
#define PT_LOAD 1

struct elf64_ehdr {
    uint8_t e_ident[ELF64_IDENT_SIZE];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct elf64_image {
    const struct elf64_ehdr *header;
    const struct elf64_phdr *program_headers;
};

enum elf64_status {
    ELF64_OK = 0,
    ELF64_INVALID_ARGUMENT,
    ELF64_TOO_SMALL,
    ELF64_BAD_MAGIC,
    ELF64_UNSUPPORTED_CLASS,
    ELF64_UNSUPPORTED_ENCODING,
    ELF64_UNSUPPORTED_MACHINE,
    ELF64_UNSUPPORTED_TYPE,
    ELF64_BAD_HEADER,
    ELF64_BAD_PROGRAM_HEADERS,
    ELF64_BAD_LOAD_RANGE,
};

enum elf64_status elf64_validate(const void *data, size_t size, struct elf64_image *image);
