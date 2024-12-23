#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "vbe.h"
#include "x86.h"
#include <stdint.h>

char *kernelLoadBuffer = (char *)MEMORY_LOAD_KERNEL_BUFFER;
char *kernel = (char *)MEMORY_KERNEL_ADDRESS;

typedef void (*KernelStart)();

void writePixel(VbeModeInfo *vbeModeInfo, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t *pixelPointer = ((uint8_t *)(vbeModeInfo->framebuffer + (y * vbeModeInfo->pitch + x * (vbeModeInfo->bitsPerPixel >> 3))));
    // assuming 8 bit color
    pixelPointer[(16 - vbeModeInfo->redPosition) >> 3] = r;
    pixelPointer[(16 - vbeModeInfo->greenPosition) >> 3] = g;
    pixelPointer[(16 - vbeModeInfo->bluePosition) >> 3] = b;
}

void ASMCALL cstart(uint16_t bootDrive) {
    clearScreen();

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive)) {
        puts("Failed to initialize disk.\r\n");
        return;
    }

    if (!FAT_Initialize(&disk)) {
        puts("Failed to initialize FAT.\r\n");
        return;
    }

    FAT_File *kernelFd = FAT_Open(&disk, "kernel.bin");
    FAT_DirectoryEntry entry;
    uint32_t readCount;
    char *kernelBuffer = kernel;
    while (readCount = FAT_Read(&disk, kernelFd, MEMORY_LOAD_KERNEL_CHUNK_SIZE, kernelLoadBuffer)) {
        memcpy(kernelBuffer, kernelLoadBuffer, readCount);
        kernelBuffer += readCount;
    }
    FAT_Close(kernelFd);

    // skip the graphics
    // VBE_Initialize();

run_kernel:
    KernelStart kernelStart = (KernelStart)kernel;
    kernelStart();
}
