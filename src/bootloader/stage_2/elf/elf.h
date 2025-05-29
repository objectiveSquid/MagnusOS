#pragma once

#include "disk/fat.h"
#include "disk/mbr.h"
#include <stdbool.h>
#include <stdint.h>

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

bool ELF_Open32Bit(Partition *partition, const char *filepath, void **entryPoint);
