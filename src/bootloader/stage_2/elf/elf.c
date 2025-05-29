#include "elf.h"
#include "disk/fat.h"
#include "memdefs.h"
#include "memory/allocator.h"
#include "util/memory.h"
#include "visual/stdio.h"

bool ELF_Open(Partition *partition, FAT_File *fd) {
    ELF_32BitHeader *header = (ELF_32BitHeader *)ALLOCATOR_Malloc(sizeof(ELF_32BitHeader), true);
    if (header == NULL) {
        puts("ELF: Failed to allocate buffer for ELF header.\n");
        return false;
    }

    // read header
    fd->position = 0;
    if (FAT_Read(partition, fd, sizeof(ELF_32BitHeader), (uint8_t *)header) != sizeof(ELF_32BitHeader)) {
        puts("ELF: Failed to read ELF header.\n");
        return false;
    }

    // header checks
    if (memcmp(header->magic, ELF_MAGIC, sizeof(header->magic)) != 0) {
        puts("ELF: Not an ELF file.\n");
        return false;
    }

    if (header->headerVersion != 1) {
        puts("ELF: Not a version 1 ELF header, only version 1 is supported.\n");
        return false;
    }

    if (header->elfVersion != 1) {
        puts("ELF: Not a version 1 ELF file, only version 1 is supported.\n");
        return false;
    }

    if (header->bitness != ELF_BITNESS_32BIT) {
        puts("ELF: Not a 32 bit ELF file, this is a 32 bit machine.\n");
        return false;
    }

    if (header->endianness != ELF_ENDIANNESS_LITTLE) {
        puts("ELF: Not a little endian ELF file, this is a little endian machine.\n");
        return false;
    }

    if (header->instructionSet != ELF_ISA_X86) {
        puts("ELF: Not an x86 ELF file, this is an x86 machine.\n");
        return false;
    }

    if (header->type != ELF_TYPE_EXECUTABLE) {
        puts("ELF: Not an executable ELF file, only executable ELF files are supported.\n");
        return false;
    }

    fd->position = header->programHeaderTableOffset;
    uint32_t programHeaderSize = header->programHeaderTableEntryCount * header->programHeaderTableEntrySize;

    char buffer[programHeaderSize];
    if (FAT_Read(partition, fd, programHeaderSize, buffer) != programHeaderSize) {
        puts("ELF: Failed to read ELF header.\n");
        return false;
    }

    FAT_Close(fd);

    return true;
}
