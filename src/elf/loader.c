#include "elf/elf64.h"

static bool range_in_file(size_t size, uint64_t offset, uint64_t length) {
    return offset <= size && length <= (uint64_t)size - offset;
}

static bool multiply_fits_size(uint64_t left, uint64_t right, size_t size) {
    if (left == 0 || right == 0) {
        return true;
    }
    if (left > UINT64_MAX / right) {
        return false;
    }
    return left * right <= size;
}

enum elf64_status elf64_validate(const void *data, size_t size, struct elf64_image *image) {
    if (data == 0 || image == 0) {
        return ELF64_INVALID_ARGUMENT;
    }
    if (size < sizeof(struct elf64_ehdr)) {
        return ELF64_TOO_SMALL;
    }
    const struct elf64_ehdr *header = data;
    if (header->e_ident[0] != ELF64_MAGIC0 || header->e_ident[1] != ELF64_MAGIC1 ||
        header->e_ident[2] != ELF64_MAGIC2 || header->e_ident[3] != ELF64_MAGIC3) {
        return ELF64_BAD_MAGIC;
    }
    if (header->e_ident[4] != ELFCLASS64) {
        return ELF64_UNSUPPORTED_CLASS;
    }
    if (header->e_ident[5] != ELFDATA2LSB || header->e_ident[6] != EV_CURRENT ||
        header->e_version != EV_CURRENT) {
        return ELF64_UNSUPPORTED_ENCODING;
    }
    if (header->e_machine != EM_X86_64) {
        return ELF64_UNSUPPORTED_MACHINE;
    }
    if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
        return ELF64_UNSUPPORTED_TYPE;
    }
    if (header->e_ehsize < sizeof(struct elf64_ehdr) || header->e_phentsize < sizeof(struct elf64_phdr) ||
        header->e_phnum == 0) {
        return ELF64_BAD_HEADER;
    }
    if (!multiply_fits_size(header->e_phnum, header->e_phentsize, size) ||
        !range_in_file(size, header->e_phoff, (uint64_t)header->e_phnum * header->e_phentsize)) {
        return ELF64_BAD_PROGRAM_HEADERS;
    }

    const uint8_t *program_header_bytes = (const uint8_t *)data + header->e_phoff;
    bool has_load_segment = false;
    for (uint16_t i = 0; i < header->e_phnum; ++i) {
        const struct elf64_phdr *program = (const struct elf64_phdr *)(program_header_bytes +
            (uint64_t)i * header->e_phentsize);
        if (program->p_type != PT_LOAD) {
            continue;
        }
        has_load_segment = true;
        if (program->p_filesz > program->p_memsz ||
            !range_in_file(size, program->p_offset, program->p_filesz)) {
            return ELF64_BAD_LOAD_RANGE;
        }
    }
    if (!has_load_segment) {
        return ELF64_BAD_LOAD_RANGE;
    }

    image->header = header;
    image->program_headers = (const struct elf64_phdr *)program_header_bytes;
    return ELF64_OK;
}
