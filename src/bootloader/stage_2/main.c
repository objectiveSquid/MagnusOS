#include "disk/fat.h"
#include "elf/elf.h"
#include "prep/vbe.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include "visual/vga.h"
#include <lib/algorithm/math.h>
#include <lib/disk/disk.h>
#include <lib/disk/mbr.h>
#include <lib/errors/errors.h>
#include <lib/memory/allocator.h>
#include <lib/memory/memdefs.h>
#include <lib/memory/memdetect.h>
#include <lib/memory/memory.h>
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

    // used for various things
    int status;

    // detect memory
    MEMDETECT_MemoryRegion *memoryRegions = (MEMDETECT_MemoryRegion *)MEMORY_MEMDETECT_REGIONS_BUFFER;
    uint32_t memoryRegionsCount;
    uint8_t memoryRegionsErrorCode;
    if ((status = MEMDETECT_GetMemoryRegions(memoryRegions, MEMORY_MEMDETECT_MAX_REGIONS, &memoryRegionsCount, &memoryRegionsErrorCode)) != NO_ERROR) {
        printf("Failed to get any memory regions! Status: %d (%s)\n", status, MEMDETECT_ErrorCodeStrings[memoryRegionsErrorCode - 1]);
        return;
    }
    printf("Got %lu memory regions!\n", memoryRegionsCount);

    uint64_t allocatorBitsSize = 0;
    for (uint32_t index = 0; index < memoryRegionsCount; ++index)
        if (memoryRegions[index].type == MEMORY_TYPE_AVAILABLE)
            allocatorBitsSize += memoryRegions[index].size / MEMORY_ALLOCATOR_CHUNK_SIZE;
    allocatorBitsSize = BITS2BYTES(allocatorBitsSize);

    uint16_t minKBafter1MB = DIV_ROUND_UP(MEMORY_MAX_KERNEL_SIZE + allocatorBitsSize, 1024);
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
    if ((status = ALLOCATOR_Initialize(memoryRegions, memoryRegionsCount, false)) != NO_ERROR) {
        printf("Failed to initialize allocator. Status: %d\n", status);
        return;
    };
    puts("Initialized allocator!\n");

    // initialize disks
    DISK masterDisk;
    DISK slaveDisk;
    DISK_InitializeResult diskInitializeResult;
    if ((status = DISK_Initialize(&diskInitializeResult, &masterDisk, &slaveDisk)) != NO_ERROR) {
        printf("Failed to initialize disks! Status: %d\n", status);
        return;
    }
    if (!diskInitializeResult.initializedMasterDisk) {
        puts("Failed to initialize master disk!\n");
        return;
    }
    puts("Initialized disks!\n");

    Partition bootPartition;
    MBR_InitializePartition(&bootPartition, &masterDisk, partitionLBA, partitionSize);
    puts("Detected boot partition!\n");

    FAT_Filesystem *bootFilesystem = malloc(sizeof(FAT_Filesystem));
    if (bootFilesystem == NULL) {
        puts("Failed to allocate memory for filesystem!\n");
        return;
    }
    bootFilesystem->partition = &bootPartition;
    if ((status = FAT_Initialize(bootFilesystem)) != NO_ERROR) {
        printf("Failed to initialize FAT. Status: %d\n", status);
        return;
    }
    puts("Initialized FAT!\n");

    // load kernel
    KernelStart kernelStart;
    if ((status = ELF_Read32Bit(bootFilesystem, "/boot/kernel.elf", (void **)&kernelStart)) != NO_ERROR) {
        printf("Failed to load kernel elf. Status: %d\n", status);
        return;
    }
    puts("Kernel loaded!\n");

    // no longer need this, we have loaded kernel
    free(bootFilesystem);

    // initialize vbe (graphics)
    VbeModeInfo *selectedVbeModeInfo = ALLOCATOR_Malloc(sizeof(VbeModeInfo), true);
    if (selectedVbeModeInfo == NULL || (size_t)selectedVbeModeInfo > (size_t)MEMORY_HIGHEST_BIOS_ADDRESS) {
        puts("Failed to allocate proper memory for VBE data!\n");
        return;
    }
    if ((status = VBE_Initialize(selectedVbeModeInfo)) != NO_ERROR) {
        puts("Failed to initialize VBE.\n");
        return;
    }

    // run kernel
    kernelStart(bootDrive, memoryRegions, memoryRegionsCount, partitionLBA, partitionSize, selectedVbeModeInfo);

    // dont need to free stuff but ill do it anyways just for good measure
    free(memoryRegions);
    free(selectedVbeModeInfo);
}
