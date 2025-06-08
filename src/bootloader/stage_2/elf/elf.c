#include "elf.h"
#include "disk/fat.h"
#include "memory/allocator.h"
#include "util/memory.h"
#include "visual/stdio.h"
#include <lib/algorithm/math.h>
#include <lib/errors/errors.h>
#include <lib/memory/memdefs.h>

#define ELF_LOAD_SEGMENT_CHUNK_SIZE 0x2000 // 0x2000 = 8 kb

typedef enum {
    ELF_PROGRAMHEADER_EXECUTABLE = 1,
    ELF_PROGRAMHEADER_WRITABLE = 2,
    ELF_PROGRAMHEADER_READABLE = 4,
} ELF_ProgramHeaderFlags;

typedef enum {
    ELF_PROGRAMHEADER_TYPE_NULL = 0,        // ignore the entry
    ELF_PROGRAMHEADER_TYPE_LOAD = 1,        // clear `memorySize` bytes at `physicalAddress` to 0, then copy `segmentSize` bytes from `offset` to `virtualAddress`
    ELF_PROGRAMHEADER_TYPE_DYNAMIC = 2,     // requires dynamic linking
    ELF_PROGRAMHEADER_TYPE_INTERPRETED = 3, // contains a file path to an executable to use as an interpreter for the following segment
    ELF_PROGRAMHEADER_TYPE_NOTES = 3,
} ELF_ProgramHeaderSegmentType;

typedef struct {
    uint32_t type;
    uint32_t offset;
    uint32_t virtualAddress;
    uint32_t physicalAddress;
    uint32_t segmentSize;
    uint32_t memorySize; // >= segmentSize
    uint32_t flags;
    uint32_t requiredAlignment; // usually a power of 2
} __attribute((packed)) ELF_32BitProgramHeader;

typedef struct {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t virtualAddress;
    uint64_t physicalAddress;
    uint64_t segmentSize;
    uint64_t memorySize;        // >= segmentSize
    uint64_t requiredAlignment; // usually a power of 2
} __attribute((packed)) ELF_64BitProgramHeader;

#define ELF_MAGIC ("\x7F" \
                   "ELF")

typedef enum {
    ELF_BITNESS_32BIT = 1,
    ELF_BITNESS_64BIT = 2,
} ELF_HeaderBitness;

typedef enum {
    ELF_ENDIANNESS_LITTLE = 1,
    ELF_ENDIANNESS_BIG = 2,
} ELF_HeaderEndianness;

typedef enum {
    ELF_ISA_NO_SPECIFIC = 0x00,
    ELF_ISA_SPARC = 0x02,
    ELF_ISA_X86 = 0x03,
    ELF_ISA_MIPS = 0x08,
    ELF_ISA_POWERPC = 0x14,
    ELF_ISA_ARM = 0x28,
    ELF_ISA_SUPERH = 0x2A,
    ELF_ISA_IA64 = 0x32,
    ELF_ISA_X86_64 = 0x3E,
    ELF_ISA_AARCH64 = 0xB7,
    ELF_ISA_RISCV = 0xF3
} ELF_HeaderInstructionSetArchitecture;

typedef enum {
    ELF_TYPE_RELOCATABLE = 1,
    ELF_TYPE_EXECUTABLE = 2,
    ELF_TYPE_SHARED = 3,
    ELF_TYPE_CORE = 4
} ELF_HeaderType;

typedef struct {
    char magic[4];      // see ELF_MAGIC
    uint8_t bitness;    // see ELF_HeaderBitness
    uint8_t endianness; // see ELF_HeaderEndianness
    uint8_t headerVersion;
    uint8_t osABI; // 0 is usually systemv
    uint8_t _padding[8];
    uint16_t type;           // see ELF_HeaderType
    uint16_t instructionSet; // see ELF_HeaderInstructionSetArchitecture
    uint32_t elfVersion;     // 1 at the time of writing this struct
    uint32_t programEntryOffset;
    uint32_t programHeaderTableOffset;
    uint32_t sectionHeaderTableOffset;
    uint32_t flags; // can probably be ignored
    uint16_t headerSize;
    uint16_t programHeaderTableEntrySize;
    uint16_t programHeaderTableEntryCount;
    uint16_t sectionHeaderTableEntrySize;
    uint16_t sectionHeaderTableEntryCount;
    uint16_t sectionNamesIndex;
} __attribute__((packed)) ELF_32BitHeader;

typedef struct {
    char magic[4];      // first byte 0x7F, then 'ELF' in ascii
    uint8_t bitness;    // 1 = 32 bit, 2 = 64 bit
    uint8_t endianness; // 1 = little endian, 2 = big endian
    uint8_t headerVersion;
    uint8_t osABI; // 0 is usually systemv
    uint8_t _padding[8];
    uint16_t type;           // 1 = relocatable, 2 = executable, 3 = shared, 4 = core
    uint16_t instructionSet; // see ELF_HeaderInstructionSetArchitecture
    uint32_t elfVersion;     // 1 at the time of writing this struct
    uint64_t programEntryOffset;
    uint64_t programHeaderTableOffset;
    uint64_t sectionHeaderTableOffset;
    uint32_t flags; // can probably be ignored
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionHeaderStringTableIndex;
} __attribute__((packed)) ELF_64BitHeader;

int ELF_Read32Bit(FAT_Filesystem *filesystem, const char *filepath, void **entryPoint) {
    int status;

    FAT_File *elfFd;
    if ((status = FAT_Open(filesystem, filepath, &elfFd)) != NO_ERROR)
        return status;

    ELF_32BitHeader *header = ALLOCATOR_Malloc(sizeof(ELF_32BitHeader), true);
    if (header == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto close_file;
    }
    if ((size_t)header > (size_t)MEMORY_HIGHEST_BIOS_ADDRESS) {
        status = ALLOCATED_MEMORY_TOO_HIGH_ERROR;
        goto free_header;
    }

    // read header
    uint32_t readHeaderCount;
    if ((status = FAT_Read(filesystem, elfFd, sizeof(ELF_32BitHeader), &readHeaderCount, header)) == NO_ERROR) {
        if (readHeaderCount != sizeof(ELF_32BitHeader)) {
            status = ELF_FILE_TOO_SMALL_ERROR;
            goto free_header;
        }
    } else {
        goto free_header;
    }

    // header checks
    if (memcmp(header->magic, ELF_MAGIC, sizeof(header->magic)) != 0) {
        status = ELF_NOT_AN_ELF_FILE;
        goto free_header;
    }

    if (header->headerVersion != 1) {
        status = ELF_UNSUPPORTED_HEADER_VERSION;
        goto free_header;
    }

    if (header->elfVersion != 1) {
        status = ELF_UNSUPPORTED_ELF_VERSION;
        goto free_header;
    }

    if (header->bitness != ELF_BITNESS_32BIT) {
        status = ELF_UNSUPPORTED_BITNESS;
        goto free_header;
    }

    if (header->endianness != ELF_ENDIANNESS_LITTLE) {
        status = ELF_UNSUPPORTED_ENDIANNESS;
        goto free_header;
    }

    if (header->instructionSet != ELF_ISA_X86) {
        status = ELF_UNSUPPORTED_INSTRUCTION_SET;
        goto free_header;
    }

    if (header->type != ELF_TYPE_EXECUTABLE) {
        status = ELF_UNSUPPORTED_ELF_TYPE;
        goto free_header;
    }

    // entry point is where the executable starts
    *entryPoint = (void *)header->programEntryOffset;

    // the program header table contains information about the segments in the program
    if ((status = FAT_Seek(filesystem, elfFd, header->programHeaderTableOffset, FAT_WHENCE_SET)) != NO_ERROR)
        goto free_header; // failed to seek

    uint32_t programHeaderTableSize = header->programHeaderTableEntryCount * header->programHeaderTableEntrySize;

    ELF_32BitProgramHeader *programHeaderTable = ALLOCATOR_Malloc(programHeaderTableSize, true);
    if (programHeaderTable == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto free_header;
    }
    if ((size_t)programHeaderTable > (size_t)MEMORY_HIGHEST_BIOS_ADDRESS) {
        status = ALLOCATED_MEMORY_TOO_HIGH_ERROR;
        goto free_table_buffer;
    }
    uint32_t readProgramHeaderTableCount;
    if ((status = FAT_Read(filesystem, elfFd, programHeaderTableSize, &readProgramHeaderTableCount, programHeaderTable)) == NO_ERROR) {
        if (readProgramHeaderTableCount != programHeaderTableSize) {
            status = ELF_FILE_TOO_SMALL_ERROR;
            goto free_table_buffer;
        }
    } else {
        goto free_table_buffer;
    }

    uint8_t *loadSegmentBuffer = ALLOCATOR_Malloc(ELF_LOAD_SEGMENT_CHUNK_SIZE, true);
    if (loadSegmentBuffer == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto free_table_buffer;
    }
    if ((size_t)loadSegmentBuffer > (size_t)MEMORY_HIGHEST_BIOS_ADDRESS) {
        status = ALLOCATED_MEMORY_TOO_HIGH_ERROR;
        goto free_segment_buffer;
    }
    ELF_32BitProgramHeader *currentProgramHeader;
    for (size_t i = 0; i < header->programHeaderTableEntryCount; ++i) {
        currentProgramHeader = &programHeaderTable[i];
        if (currentProgramHeader->type != ELF_PROGRAMHEADER_TYPE_LOAD)
            continue;

        // clean memory
        memset((void *)currentProgramHeader->virtualAddress, 0, currentProgramHeader->memorySize);

        if ((status = FAT_Seek(filesystem, elfFd, currentProgramHeader->offset, FAT_WHENCE_SET)) != NO_ERROR)
            goto free_segment_buffer;

        // spaghetti, this is where we read the segments and load them (naively) directly to their listed virtual address
        size_t bytesToRead = currentProgramHeader->segmentSize;
        while (bytesToRead > 0) {
            size_t readNow = min(bytesToRead, ELF_LOAD_SEGMENT_CHUNK_SIZE);
            size_t readCount;
            if (FAT_Read(filesystem, elfFd, readNow, &readCount, loadSegmentBuffer) == NO_ERROR) {
                if (readCount != readNow) {
                    status = ELF_FILE_TOO_SMALL_ERROR;
                    goto free_segment_buffer;
                }
            } else {
                goto free_segment_buffer;
            }

            memcpy((void *)(currentProgramHeader->virtualAddress + currentProgramHeader->segmentSize - bytesToRead), loadSegmentBuffer, readNow);
            bytesToRead -= readNow;
        }
    }

free_segment_buffer:
    free(loadSegmentBuffer);
free_table_buffer:
    free(programHeaderTable);
free_header:
    free(header);
close_file:
    FAT_Close(filesystem, elfFd); // here we'll ignore the error for once

    return status;
}
