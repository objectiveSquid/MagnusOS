#include "disk/disk.h"
#include "disk/fat.h"
#include "memdefs.h"
#include "prep/memdetect.h"
#include "prep/vbe.h"
#include "util/memory.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include "visual/vga.h"
#include <stdint.h>

#define MEMORY_LOAD_KERNEL_CHUNK_SIZE 0x00010000

extern void _init();

extern char __bss_start;
extern char __end;

typedef void (*KernelStart)(uint8_t bootDrive, MEMDETECT_MemoryRegion *memoryRegions, uint64_t memoryRegionsCount);

static uint8_t *kernelAddress = (uint8_t *)MEMORY_KERNEL_ADDRESS;

void ASMCALL cstart(uint8_t bootDrive) {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));
    _init(); // call global constructors

    clearScreen();

    DISK disk;
    memset(&disk, 0, sizeof(DISK));
    if (!DISK_Initialize(&disk, bootDrive)) {
        puts("Failed to initialize disk.\n");
        return;
    }
    printf("Initialized disk! (%llu sectors)\n", disk.cylinders * disk.heads * disk.sectors);

    if (!FAT_Initialize(&disk)) {
        puts("Failed to initialize FAT.\n");
        return;
    }
    puts("Initialized FAT!\n");

    FAT_File *kernelFd = FAT_Open(&disk, "boot/kernel.bin");
    if (kernelFd == NULL) {
        puts("Failed to open kernel file.\n");
        return;
    }
    uint32_t readCount;
    uint8_t *kernelBuffer = kernelAddress;
    while (readCount = FAT_Read(&disk, kernelFd, MEMORY_LOAD_KERNEL_CHUNK_SIZE, kernelBuffer))
        kernelBuffer += readCount;
    FAT_Close(kernelFd);
    printf("Kernel loaded! (%llu bytes)\n", kernelBuffer - kernelAddress);

    MEMDETECT_MemoryRegion *memoryRegions = (MEMDETECT_MemoryRegion *)MEMORY_MEMDETECT_REGIONS_BUFFER;
    uint64_t memoryRegionsCount;
    uint8_t memDetectErrorCode;
    if ((memDetectErrorCode = MEMDETECT_GetMemoryRegions(memoryRegions, MEMORY_MEMDETECT_MAX_REGIONS, &memoryRegionsCount)) != 0) {
        printf("Failed to get any memory regions! (%s)\n", MEMDETECT_ErrorCodeStrings[memDetectErrorCode - 1]);
        return;
    }
    printf("Got %llu memory regions!\n", memoryRegionsCount);

    if (!VBE_Initialize()) {
        puts("Failed to initialize VBE.\n");
        return;
    }
    puts("Initialized VBE!\n");

    KernelStart kernelStart = (KernelStart)kernelAddress;
    kernelStart(bootDrive, memoryRegions, memoryRegionsCount);
}
