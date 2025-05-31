#include "disk/disk.h"
#include "disk/fat.h"
#include "disk/mbr.h"
#include "elf/elf.h"
#include "memdefs.h"
#include "memory/allocator.h"
#include "memory/memdetect.h"
#include "prep/vbe.h"
#include "util/memory.h"
#include "util/other.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include "visual/vga.h"
#include <stdint.h>

#define MEMORY_LOAD_KERNEL_CHUNK_SIZE 0x10000

extern void _init();

extern char __bss_start;
extern char __end;

typedef void (*KernelStart)(uint8_t bootDrive,
                            MEMDETECT_MemoryRegion *memoryRegions,
                            uint32_t memoryRegionsCount,
                            uint32_t partitionLBA,
                            uint32_t partitionSize,
                            VbeModeInfo *vbeModeInfo);

void ASMCALL cstart(uint8_t bootDrive, uint32_t partitionLBA, uint32_t partitionSize) {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));
    _init(); // call global constructors

    clearScreen();

    // detect memory
    MEMDETECT_MemoryRegion *memoryRegions = (MEMDETECT_MemoryRegion *)MEMORY_MEMDETECT_REGIONS_BUFFER;
    uint32_t memoryRegionsCount;
    uint8_t memDetectErrorCode;
    if ((memDetectErrorCode = MEMDETECT_GetMemoryRegions(memoryRegions, MEMORY_MEMDETECT_MAX_REGIONS, &memoryRegionsCount)) != 0) {
        printf("Failed to get any memory regions! (%s)\n", MEMDETECT_ErrorCodeStrings[memDetectErrorCode - 1]);
        return;
    }
    printf("Got %lu memory regions!\n", memoryRegionsCount);

    uint64_t allocatorBitsSize = 0;
    for (uint32_t index = 0; index < memoryRegionsCount; ++index)
        if (memoryRegions[index].type == MEMORY_TYPE_AVAILABLE)
            allocatorBitsSize += memoryRegions[index].size / MEMORY_ALLOCATOR_CHUNK_SIZE;
    allocatorBitsSize = BITS2BYTES(allocatorBitsSize); // 8 bits in a byte, also round up

    uint16_t minKBafter1MB = (MEMORY_MAX_KERNEL_SIZE + allocatorBitsSize + 1023) / 1024; // round up
    uint16_t contiguousKBAfter1MB;
    if ((contiguousKBAfter1MB = x86_MEMDETECT_GetContiguousKBAfter1MB()) == 0) {
        puts("Failed to get contiguous memory after 1MB, cannot know if there is space for kernel and allocator bits!\n");
        return;
    }
    if (contiguousKBAfter1MB < minKBafter1MB) {
        printf("Not enough contiguous memory after 1MB! (needed %hu KB, got %hu KB)\n", minKBafter1MB, contiguousKBAfter1MB);
        return;
    }

    // initialize allocator
    ALLOCATOR_Initialize(memoryRegions, memoryRegionsCount, false);
    puts("Initialized allocator!\n");

    // disk and fat
    DISK disk;
    memset(&disk, 0, sizeof(DISK));
    if (!DISK_Initialize(&disk, bootDrive)) {
        puts("Failed to initialize disk.\n");
        return;
    }
    printf("Initialized disk! (%lu sectors)\n", disk.cylinders * disk.heads * disk.sectors);

    Partition bootPartition;
    MBR_DetectPartition(&bootPartition, &disk, partitionLBA, partitionSize);
    puts("Detected boot partition!\n");

    FAT_Filesystem *bootFilesystem = ALLOCATOR_Malloc(sizeof(FAT_Filesystem), true);
    if (bootFilesystem == NULL || (size_t)bootFilesystem > (size_t)MEMORY_HIGHEST_BIOS_ADDRESS) {
        puts("Failed to allocate memory for filesystem!\n");
        return;
    }
    bootFilesystem->partition = &bootPartition;
    if (!FAT_Initialize(bootFilesystem)) {
        puts("Failed to initialize FAT.\n");
        return;
    }
    puts("Initialized FAT!\n");

    // load kernel
    KernelStart kernelStart;
    if (!ELF_Read32Bit(bootFilesystem, "/boot/kernel.elf", (void **)&kernelStart)) {
        puts("Failed to load kernel elf.\n");
        return;
    }
    puts("Kernel loaded!\n");

    // free fat memory
    free(bootFilesystem);

    // initialize vbe (graphics)
    VbeModeInfo *selectedVbeModeInfo;
    if ((selectedVbeModeInfo = VBE_Initialize()) == NULL) {
        puts("Failed to initialize VBE.\n");
        return;
    }

    // run kernel
    kernelStart(bootDrive, memoryRegions, memoryRegionsCount, partitionLBA, partitionSize, selectedVbeModeInfo);
}
