#include "elf.h"
#include "disk/fat.h"
#include "memdefs.h"
#include "memory/allocator.h"
#include "util/memory.h"
#include "util/other.h"
#include "visual/stdio.h"

#define ELF_LOAD_SEGMENT_CHUNK_SIZE 0x2000 // 0x2000 = 8 kb

// if I call `FAT_Read` here with the output buffer being `NULL`, it is to seek
bool ELF_Open32Bit(Partition *partition, const char *filepath, void **entryPoint) {
    bool returnValue = true;

    FAT_File *elfFd = FAT_Open(partition, filepath);
    if (elfFd == NULL) {
        printf("ELF: Failed to open ELF file %s.\n", filepath);
        return false;
    }

    ELF_32BitHeader *header = (ELF_32BitHeader *)ALLOCATOR_Malloc(sizeof(ELF_32BitHeader), true);
    if (header == NULL) {
        puts("ELF: Failed to allocate buffer for ELF header.\n");
        returnValue = false;
        goto close_file;
    }

    // read header
    uint32_t readHeaderCount;
    if ((readHeaderCount = FAT_Read(partition, elfFd, sizeof(ELF_32BitHeader), (uint8_t *)header)) != sizeof(ELF_32BitHeader)) {
        printf("ELF: Failed to read ELF header. Read %lu bytes instead of %lu.\n", readHeaderCount, sizeof(ELF_32BitHeader));
        returnValue = false;
        goto free_header;
    }

    // header checks
    if (memcmp(header->magic, ELF_MAGIC, sizeof(header->magic)) != 0) {
        puts("ELF: Not an ELF file.\n");
        returnValue = false;
        goto free_header;
    }

    if (header->headerVersion != 1) {
        puts("ELF: Not a version 1 ELF header, only version 1 is supported.\n");
        returnValue = false;
        goto free_header;
    }

    if (header->elfVersion != 1) {
        puts("ELF: Not a version 1 ELF file, only version 1 is supported.\n");
        returnValue = false;
        goto free_header;
    }

    if (header->bitness != ELF_BITNESS_32BIT) {
        puts("ELF: Not a 32 bit ELF file, use the 64 bit version of this function.\n");
        returnValue = false;
        goto free_header;
    }

    if (header->endianness != ELF_ENDIANNESS_LITTLE) {
        puts("ELF: Not a little endian ELF file, this is a little endian machine.\n");
        returnValue = false;
        goto free_header;
    }

    if (header->instructionSet != ELF_ISA_X86) {
        puts("ELF: Not an x86 ELF file, this is an x86 machine.\n");
        returnValue = false;
        goto free_header;
    }

    if (header->type != ELF_TYPE_EXECUTABLE) {
        puts("ELF: Not an executable ELF file, only executable ELF files are supported.\n");
        returnValue = false;
        goto free_header;
    }

    // entry point is where the executable starts
    *entryPoint = (void *)header->programEntryOffset;

    // the program header table contains information about the segments in the program
    if (FAT_Read(partition, elfFd, header->programHeaderTableOffset - elfFd->position, NULL) != (header->programHeaderTableOffset - elfFd->position)) {
        puts("ELF: Failed to seek to ELF program header table.\n");
        returnValue = false;
        goto free_header;
    }
    uint32_t programHeaderTableSize = header->programHeaderTableEntryCount * header->programHeaderTableEntrySize;

    ELF_32BitProgramHeader *programHeaderTable = ALLOCATOR_Malloc(programHeaderTableSize, true);
    if (FAT_Read(partition, elfFd, programHeaderTableSize, programHeaderTable) != programHeaderTableSize) {
        puts("ELF: Failed to read ELF header.\n");
        returnValue = false;
        goto free_header;
    }
    FAT_Close(elfFd); // closing it because i may need to seek backwards later

    uint8_t *loadSegmentBuffer = ALLOCATOR_Malloc(ELF_LOAD_SEGMENT_CHUNK_SIZE, true);
    if (loadSegmentBuffer == NULL) {
        puts("ELF: Failed to allocate buffer for ELF load segment.\n");
        returnValue = false;
        goto free_header;
    }
    ELF_32BitProgramHeader *currentProgramHeader;
    for (size_t i = 0; i < header->programHeaderTableEntryCount; ++i) {
        currentProgramHeader = &programHeaderTable[i];
        if (currentProgramHeader->type != ELF_PROGRAMHEADER_TYPE_LOAD)
            continue;

        // clean memory
        memset((void *)currentProgramHeader->virtualAddress, 0, currentProgramHeader->memorySize);

        // im sorry for this. i have not yet implemented a seek function in the fat driver
        elfFd = FAT_Open(partition, filepath);
        if (elfFd == NULL) {
            puts("ELF: Failed to open ELF file.\n");
            returnValue = false;
            goto free_segment_buffer;
        }
        while (elfFd->position < currentProgramHeader->offset) {
            uint32_t shouldRead = min(currentProgramHeader->offset, ELF_LOAD_SEGMENT_CHUNK_SIZE);
            uint32_t readCount = FAT_Read(partition, elfFd, shouldRead, NULL);
            if (readCount != shouldRead) {
                printf("ELF: Failed to seek to ELF load segment.!\n");
                returnValue = false;
                goto free_segment_buffer;
            }
        }

        // spaghetti, this is where we read the segments and load them (naively) directly to their written virtual address
        size_t bytesToRead = currentProgramHeader->segmentSize;
        while (bytesToRead > 0) {
            size_t readNow = min(bytesToRead, ELF_LOAD_SEGMENT_CHUNK_SIZE);
            if (FAT_Read(partition, elfFd, readNow, loadSegmentBuffer) != readNow) {
                printf("ELF: Failed to read ELF load segment chunk (%lu bytes at file offset %lu).\n", readNow, elfFd->position);
                returnValue = false;
                goto free_segment_buffer;
            }

            memcpy((void *)(currentProgramHeader->virtualAddress + currentProgramHeader->segmentSize - bytesToRead), loadSegmentBuffer, readNow);
            bytesToRead -= readNow;
        }

        FAT_Close(elfFd);
    }

free_segment_buffer:
    free(loadSegmentBuffer);
free_header:
    free(header);
close_file:
    FAT_Close(elfFd);

    return returnValue;
}
