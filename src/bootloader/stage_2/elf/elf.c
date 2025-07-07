#include "elf.h"
#include <lib/algorithm/math.h>
#include <lib/algorithm/string.h>
#include <lib/disk/fat.h>
#include <lib/errors/errors.h>
#include <lib/memory/allocator.h>
#include <lib/memory/memdefs.h>
#include <lib/memory/memory.h>

#define ELF_LOAD_SEGMENT_CHUNK_SIZE 0x2000 // 0x2000 = 8 kb

#define SHN_UNDEF 0

#define DT_REL 17
#define DT_RELA 7

typedef enum {
    ELF_PROGRAMHEADER_EXECUTABLE = 1,
    ELF_PROGRAMHEADER_WRITABLE = 2,
    ELF_PROGRAMHEADER_READABLE = 4,
} ELF_ProgramHeaderFlags;

typedef enum {
    ELF_PROGRAMHEADER_TYPE_NULL = 0,        // ignore the entry
    ELF_PROGRAMHEADER_TYPE_LOAD = 1,        // clear `memorySize` bytes at `virtualAddress` to 0, then copy `segmentSize` bytes from `offset` to `virtualAddress`
    ELF_PROGRAMHEADER_TYPE_DYNAMIC = 2,     // requires dynamic linking
    ELF_PROGRAMHEADER_TYPE_INTERPRETED = 3, // contains a file path to an executable to use as an interpreter for the following segment
    ELF_PROGRAMHEADER_TYPE_NOTES = 4,
} ELF_ProgramHeaderSegmentType;

typedef enum {
    SHT_NULL = 0x0,           // 	Section header table entry unused
    SHT_PROGBITS = 0x1,       // 	Program data
    SHT_SYMTAB = 0x2,         // 	Symbol table
    SHT_STRTAB = 0x3,         // 	String table
    SHT_RELA = 0x4,           // 	Relocation entries with addends
    SHT_HASH = 0x5,           // 	Symbol hash table
    SHT_DYNAMIC = 0x6,        // 	Dynamic linking information
    SHT_NOTE = 0x7,           // 	Notes
    SHT_NOBITS = 0x8,         // 	Program space with no data (bss)
    SHT_REL = 0x9,            // 	Relocation entries, no addends
    SHT_SHLIB = 0x0A,         // 	Reserved
    SHT_DYNSYM = 0x0B,        // 	Dynamic linker symbol table
    SHT_INIT_ARRAY = 0x0E,    // 	Array of constructors
    SHT_FINI_ARRAY = 0x0F,    // 	Array of destructors
    SHT_PREINIT_ARRAY = 0x10, // 	Array of pre-constructors
    SHT_GROUP = 0x11,         // 	Section group
    SHT_SYMTAB_SHNDX = 0x12,  // 	Extended section indices
    SHT_NUM = 0x13,           // 	Number of defined types.
} ELF_SectionHeaderType;

typedef struct {
    uint32_t sh_name;      // Offset into section header string table
    uint32_t sh_type;      // Section type
    uint32_t sh_flags;     // Section flags
    uint32_t sh_addr;      // Virtual address in memory
    uint32_t sh_offset;    // Offset in file
    uint32_t sh_size;      // Size of section
    uint32_t sh_link;      // Link to another section
    uint32_t sh_info;      // Misc info
    uint32_t sh_addralign; // Section alignment
    uint32_t sh_entsize;   // Entry size if section holds table
} __attribute__((packed)) Elf32_Shdr;

typedef struct {
    uint32_t p_type; // see ELF_ProgramHeaderSegmentType
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz; // size in the file
    uint32_t p_memsz;  // >= segmentSize
    uint32_t p_flags;  // see ELF_ProgramHeaderFlags
    uint32_t p_align;  // usually a power of 2
} __attribute((packed)) Elf32_Phdr;

typedef struct {
    uint32_t p_type;  // see ELF_ProgramHeaderSegmentType
    uint32_t p_flags; // see ELF_ProgramHeaderFlags
    uint64_t offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz; // size in the file
    uint64_t p_memsz;  // >= segmentSize
    uint64_t p_align;  // usually a power of 2
} __attribute((packed)) ELF64_Phdr;

#define ELF_MAGIC ("\x7F" \
                   "ELF")

typedef struct {
    int32_t tag;
    union {
        uint32_t value;
        uint32_t pointer;
    } _union;
} __attribute__((packed)) Elf32_Dyn;

typedef struct {
    uint32_t offset; // where to apply the relocation
    uint32_t info;   // symbol + type
} __attribute__((packed)) Elf32_Rel;

typedef struct {
    uint32_t offset; // where to apply the relocation
    uint32_t info;   // symbol + type
    uint32_t addend;
} __attribute__((packed)) Elf32_RelA;
#define ELF32_GET_SYMBOL(info) ((info) >> 8)
#define ELF32_GET_TYPE(info) ((info) & 0xFF)

typedef struct {
    uint32_t name;  // string table offset
    uint32_t value; // symbol value (virtual address)
    uint32_t size;
    uint8_t info;
    uint8_t other;
    uint16_t shndx; // section header index
} __attribute__((packed)) ELF_32BitSymbol;

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

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_ABIVERSION 8
#define EI_PAD 9

typedef struct {
    char e_ident[16];
    uint16_t e_type;    // see ELF_HeaderType
    uint16_t e_machine; // see ELF_HeaderInstructionSetArchitecture
    uint32_t e_version; // 1 at the time of writing this struct
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags; // can probably be ignored
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf32_Hdr;

typedef struct {
    char e_ident[16];
    uint16_t e_type;    // see ELF_HeaderType
    uint16_t e_machine; // see ELF_HeaderInstructionSetArchitecture
    uint32_t e_version; // 1 at the time of writing this struct
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags; // can probably be ignored
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf64_Hdr;

int ELF_CheckHeader(Elf32_Hdr *header) {
    if (memcmp(header->e_ident, ELF_MAGIC, 4) != 0)
        return ELF_NOT_AN_ELF_FILE;

    if (header->e_ident[EI_VERSION] != 1)
        return ELF_UNSUPPORTED_HEADER_VERSION;

    if (header->e_version != 1)
        return ELF_UNSUPPORTED_ELF_VERSION;

    if (header->e_ident[EI_CLASS] != ELF_BITNESS_32BIT)
        return ELF_UNSUPPORTED_BITNESS;

    if (header->e_ident[EI_DATA] != ELF_ENDIANNESS_LITTLE)
        return ELF_UNSUPPORTED_ENDIANNESS;

    if (header->e_machine != ELF_ISA_X86)
        return ELF_UNSUPPORTED_INSTRUCTION_SET;

    if (header->e_type != ELF_TYPE_SHARED)
        return ELF_UNSUPPORTED_ELF_TYPE;

    return NO_ERROR;
}

#define ELF32_ST_BIND(i) ((i) >> 4)
#define ELF32_ST_TYPE(i) ((i) & 0xF)

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2

int applyRelocations(void *loadBase, void *symbolTab, void *stringTab, Elf32_Rel *table, uint32_t entryCount) {
    for (uint32_t i = 0; i < entryCount; i++) {
        uint32_t type = ELF32_GET_TYPE(table[i].info);
        uint32_t *targetAddress = (uint32_t *)(loadBase + table[i].offset);

        switch (type) {
        case 8: // R_386_RELATIVE
            *targetAddress += (uint32_t)loadBase;
            break;
        default:
            return ELF_UNSUPPORTED_RELOCATION_TYPE;
        }
    }

    return NO_ERROR;
}

int applyRelocationsA(void *loadBase, void *symbolTab, void *stringTab, Elf32_RelA *table, uint32_t entryCount) {
    for (uint32_t i = 0; i < entryCount; i++) {
        uint32_t type = ELF32_GET_TYPE(table[i].info);
        uint32_t *targetAddress = (uint32_t *)((uint8_t *)loadBase + table[i].offset);
        int32_t addend = table[i].addend; /* signed 32‑bit */

        switch (type) {
        case 8: /* R_386_RELATIVE */
            *targetAddress = (uint32_t)loadBase + (uint32_t)addend;
            break;
        default:
            return ELF_UNSUPPORTED_RELOCATION_TYPE;
        }
    }

    return NO_ERROR;
}

Elf32_Shdr *findSectionHeader(Elf32_Hdr *header, Elf32_Shdr *sectionHeaderTable, const char *sectionName, const char *stringTable) {
    Elf32_Shdr *relDynSectionHeader = NULL;

    for (size_t i = 0; i < header->e_shnum; i++) {
        Elf32_Shdr *sectionHeader = (Elf32_Shdr *)(&sectionHeaderTable[i]);
        const char *currentSectionName = stringTable + sectionHeader->sh_name;
        if (strcmp(currentSectionName, sectionName) == 0) {
            relDynSectionHeader = sectionHeader;
            break;
        }
    }

    return relDynSectionHeader;
}

int getStringTable(Elf32_Hdr *header, Elf32_Shdr *sectionHeaderTable, FAT_Filesystem *filesystem, FAT_File *elfFd, char **stringTableOutput) {
    int status;

    Elf32_Shdr *stringSectionHeader = (Elf32_Shdr *)(&sectionHeaderTable[header->e_shstrndx]);
    char *stringTable = malloc(stringSectionHeader->sh_size);
    if (stringTable == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;

    if ((status = FAT_Seek(filesystem, elfFd, stringSectionHeader->sh_offset, FAT_WHENCE_SET)) != NO_ERROR)
        return status;

    uint32_t readCount;
    if ((status = FAT_Read(filesystem, elfFd, stringSectionHeader->sh_size, &readCount, stringTable)) != NO_ERROR)
        if (readCount != stringSectionHeader->sh_size)
            return ELF_FILE_TOO_SMALL_ERROR;

    *stringTableOutput = stringTable;

    return status;
}

int loadSegments(Elf32_Hdr *header, Elf32_Phdr *programHeaderTable, FAT_Filesystem *filesystem, FAT_File *elfFd, uintptr_t loadBase) {
    int status;
    uint32_t readCount;
    Elf32_Phdr *currentProgramHeader = NULL;

    char *loadSegmentBuffer = malloc(ELF_LOAD_SEGMENT_CHUNK_SIZE);
    if (loadSegmentBuffer == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;

    for (size_t i = 0; i < header->e_phnum; ++i) {
        currentProgramHeader = &programHeaderTable[i];

        if (currentProgramHeader->p_type != ELF_PROGRAMHEADER_TYPE_LOAD)
            continue;

        // clean memory
        void *segmentDestination = (void *)(currentProgramHeader->p_vaddr + loadBase);
        memset(segmentDestination, 0, currentProgramHeader->p_memsz);

        if ((status = FAT_Seek(filesystem, elfFd, currentProgramHeader->p_offset, FAT_WHENCE_SET)) != NO_ERROR) {
            free(loadSegmentBuffer);
            return status;
        }

        // spaghetti
        size_t bytesToRead = currentProgramHeader->p_filesz;
        while (bytesToRead > 0) {
            size_t readNow = min(bytesToRead, ELF_LOAD_SEGMENT_CHUNK_SIZE);
            if ((status = FAT_Read(filesystem, elfFd, readNow, &readCount, loadSegmentBuffer)) == NO_ERROR) {
                if (readCount != readNow) {
                    free(loadSegmentBuffer);
                    return ELF_FILE_TOO_SMALL_ERROR;
                }
            } else {
                free(loadSegmentBuffer);
                return status;
            }

            memcpy((void *)(segmentDestination + (currentProgramHeader->p_filesz - bytesToRead)), loadSegmentBuffer, readCount);
            bytesToRead -= readCount;
        }
    }

    free(loadSegmentBuffer);
    return NO_ERROR;
}

int handleRelADynSection(void *loadBase, Elf32_Shdr *sectionHeader, FAT_Filesystem *filesystem, FAT_File *elfFd) {
    int status;

    Elf32_RelA *sectionBuffer = malloc(sectionHeader->sh_size);
    if (sectionBuffer == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;

    // read rela.dyn
    if ((status = FAT_Seek(filesystem, elfFd, sectionHeader->sh_offset, FAT_WHENCE_SET)) != NO_ERROR) {
        free(sectionBuffer);
        return status;
    }

    uint32_t readCount;
    if ((status = FAT_Read(filesystem, elfFd, sectionHeader->sh_size, &readCount, sectionBuffer)) != NO_ERROR)
        if (readCount != sectionHeader->sh_size) {
            free(sectionBuffer);
            return ELF_FILE_TOO_SMALL_ERROR;
        }

    // apply relocations
    status = applyRelocationsA(loadBase, NULL, NULL, sectionBuffer, sectionHeader->sh_size / sizeof(Elf32_RelA));

    free(sectionBuffer);
    return status;
}

int handleRelDynSection(void *loadBase, Elf32_Shdr *sectionHeader, FAT_Filesystem *filesystem, FAT_File *elfFd) {
    int status;

    Elf32_Rel *sectionBuffer = malloc(sectionHeader->sh_size);
    if (sectionBuffer == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;

    // read .rel.dyn
    if ((status = FAT_Seek(filesystem, elfFd, sectionHeader->sh_offset, FAT_WHENCE_SET)) != NO_ERROR) {
        free(sectionBuffer);
        return status;
    }

    uint32_t readCount;
    if ((status = FAT_Read(filesystem, elfFd, sectionHeader->sh_size, &readCount, sectionBuffer)) != NO_ERROR)
        if (readCount != sectionHeader->sh_size) {
            free(sectionBuffer);
            return ELF_FILE_TOO_SMALL_ERROR;
        }

    // apply relocations
    status = applyRelocations(loadBase, NULL, NULL, sectionBuffer, sectionHeader->sh_size / sizeof(Elf32_Rel));

    free(sectionBuffer);
    return status;
}

int ELF_Load32Bit(FAT_Filesystem *filesystem, const char *filepath, void **entryPoint) {
    int status = NO_ERROR;

    FAT_File *elfFd;
    if ((status = FAT_Open(filesystem, filepath, &elfFd)) != NO_ERROR)
        return status;
    FAT_Seek(filesystem, elfFd, 0, FAT_WHENCE_SET); // ignore error, since we are most definitely already at the beginning of the file

    Elf32_Hdr *header = malloc(sizeof(Elf32_Hdr));
    if (header == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto close_file;
    }

    // read header
    uint32_t readCount;
    if ((status = FAT_Read(filesystem, elfFd, sizeof(Elf32_Hdr), &readCount, header)) == NO_ERROR) {
        if (readCount != sizeof(Elf32_Hdr)) {
            status = ELF_FILE_TOO_SMALL_ERROR;
            goto free_header;
        }
    } else {
        goto free_header;
    }

    // header checks
    if ((status = ELF_CheckHeader(header)) != NO_ERROR)
        goto free_header;

    // the program header table contains information about the segments in the program
    if ((status = FAT_Seek(filesystem, elfFd, header->e_phoff, FAT_WHENCE_SET)) != NO_ERROR)
        goto free_header; // failed to seek

    uint32_t programHeaderTableSize = header->e_phnum * header->e_phentsize;

    Elf32_Phdr *programHeaderTable = malloc(programHeaderTableSize);
    if (programHeaderTable == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto free_header;
    }

    uint32_t readProgramHeaderTableCount;
    if ((status = FAT_Read(filesystem, elfFd, programHeaderTableSize, &readProgramHeaderTableCount, programHeaderTable)) == NO_ERROR) {
        if (readProgramHeaderTableCount != programHeaderTableSize) {
            status = ELF_FILE_TOO_SMALL_ERROR;
            goto free_program_table_buffer;
        }
    } else {
        goto free_program_table_buffer;
    }

    // calculate the memory needed to load the program and allocate it
    Elf32_Phdr *currentProgramHeader;
    void *loadBase;
    uintptr_t loadRangeStart = UINTPTR_MAX;
    uintptr_t loadRangeStop = 0;
    for (size_t i = 0; i < header->e_phnum; ++i) {
        currentProgramHeader = &programHeaderTable[i];
        if (currentProgramHeader->p_type != ELF_PROGRAMHEADER_TYPE_LOAD)
            continue;

        // ensure alignment
        loadRangeStart = min(loadRangeStart, currentProgramHeader->p_vaddr & ~(PAGE_SIZE - 1));                                                   // round down
        loadRangeStop = max(loadRangeStop, (currentProgramHeader->p_vaddr + currentProgramHeader->p_memsz + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1)); // round up
    }
    uintptr_t loadRangeSize = loadRangeStop - loadRangeStart;
    loadBase = mallocPageAligned(loadRangeSize);
    if (loadBase == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto free_program_table_buffer;
    }

    // entry point will be where the executable starts
    *entryPoint = (void *)(header->e_entry + loadBase);

    // load segments into memory
    if ((status = loadSegments(header, programHeaderTable, filesystem, elfFd, (uintptr_t)loadBase)) != NO_ERROR)
        goto free_loadbase;

    // get sections
    size_t sectionHeaderTableSize = header->e_shnum * header->e_shentsize;

    Elf32_Shdr *sectionHeaderTable = malloc(sectionHeaderTableSize);
    if (sectionHeaderTable == NULL) {
        status = FAILED_TO_ALLOCATE_MEMORY_ERROR;
        goto free_loadbase;
    }

    if ((status = FAT_Seek(filesystem, elfFd, header->e_shoff, FAT_WHENCE_SET)) != NO_ERROR)
        goto free_section_table_buffer; // failed to seek

    // read section headers
    if ((status = FAT_Read(filesystem, elfFd, sectionHeaderTableSize, &readCount, sectionHeaderTable)) != NO_ERROR)
        if (readCount != sectionHeaderTableSize) {
            status = ELF_FILE_TOO_SMALL_ERROR;
            goto free_section_table_buffer;
        }

    // get string table
    char *stringTable;
    if ((status = getStringTable(header, sectionHeaderTable, filesystem, elfFd, &stringTable)) != NO_ERROR)
        goto free_string_table_buffer; // its ok to free it even if the allocation fails as if a pointer given to free is NULL the call is ignored

    // find .rel.dyn and .rela.dyn section and handle relocations
    Elf32_Shdr *relDynSectionHeader = findSectionHeader(header, sectionHeaderTable, ".rel.dyn", stringTable);
    Elf32_Shdr *relADynSectionHeader = findSectionHeader(header, sectionHeaderTable, ".rela.dyn", stringTable);

    if (relDynSectionHeader != NULL)
        if ((status = handleRelDynSection(loadBase, relDynSectionHeader, filesystem, elfFd)) != NO_ERROR)
            goto free_string_table_buffer;

    if (relADynSectionHeader != NULL)
        if ((status = handleRelADynSection(loadBase, relADynSectionHeader, filesystem, elfFd)) != NO_ERROR)
            goto free_string_table_buffer;

free_string_table_buffer:
    free(stringTable);
free_section_table_buffer:
    free(sectionHeaderTable);
free_loadbase:
    if (status != NO_ERROR)
        free(loadBase); // only free this on error
free_program_table_buffer:
    free(programHeaderTable);
free_header:
    free(header);
close_file:
    FAT_Close(filesystem, elfFd); // here we'll ignore the error for once

    return status;
}
