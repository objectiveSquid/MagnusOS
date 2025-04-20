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

    DISK *disk = (DISK *)MEMORY_DISK_INFO_BUFFER;
    memset(disk, 0, sizeof(DISK));
    if (!DISK_Initialize(disk, bootDrive)) {
        puts("Failed to initialize disk.\r\n");
        return;
    }

    if (!FAT_Initialize(disk)) {
        puts("Failed to initialize FAT.\r\n");
        return;
    }

    FAT_File *kernelFd = FAT_Open(disk, "kernel.bin");
    uint32_t readCount;
    uint8_t *kernelBuffer = kernel;
    while (readCount = FAT_Read(disk, kernelFd, MEMORY_LOAD_KERNEL_CHUNK_SIZE, kernelBuffer))
        kernelBuffer += readCount;
    FAT_Close(kernelFd);

    VBE_Initialize();

run_kernel:
    KernelStart kernelStart = (KernelStart)kernel;
    kernelStart();
}
