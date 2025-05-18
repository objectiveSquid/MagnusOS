#include "disk/disk.h"
#include "disk/fat.h"
#include "memdefs.h"
#include "prep/vbe.h"
#include "util/memory.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include "visual/vga.h"
#include <stdint.h>

#define MEMORY_LOAD_KERNEL_CHUNK_SIZE 0x00010000

typedef void (*KernelStart)();

static uint8_t *kernel = (uint8_t *)MEMORY_KERNEL_ADDRESS;

void ASMCALL cstart(uint16_t bootDrive) {
    clearScreen();

    DISK *disk = (DISK *)MEMORY_FAT12_DISK_BUFFER;
    memset(disk, 0, sizeof(DISK));
    if (!DISK_Initialize(disk, bootDrive)) {
        puts("Failed to initialize disk.\n");
        return;
    }
    puts("Initialized disk!\n");

    if (!FAT_Initialize(disk)) {
        puts("Failed to initialize FAT.\n");
        return;
    }
    puts("Initialized FAT!\n");

    FAT_File *kernelFd = FAT_Open(disk, "boot/kernel.bin");
    if (kernelFd == NULL) {
        puts("Failed to open kernel file.\n");
        return;
    }
    uint32_t readCount;
    uint8_t *kernelBuffer = kernel;
    while (readCount = FAT_Read(disk, kernelFd, MEMORY_LOAD_KERNEL_CHUNK_SIZE, kernelBuffer))
        kernelBuffer += readCount;
    FAT_Close(kernelFd);
    puts("Kernel loaded!\n");

    if (!VBE_Initialize()) {
        puts("Failed to initialize VBE.\n");
        return;
    }
    puts("Initialized VBE!\n");

    KernelStart kernelStart = (KernelStart)kernel;
    kernelStart();
}
