#include "disk.h"
#include "fat.h"
#include "graphics.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "vbe.h"
#include "x86.h"
#include <stdint.h>

char *kernelLoadBuffer = (char *)MEMORY_LOAD_KERNEL_BUFFER;
char *kernel = (char *)MEMORY_KERNEL_ADDRESS;

typedef void (*KernelStart)();

void ASMCALL cstart(uint16_t bootDrive) {
    VGA_ClearScreen();

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

    // kernel doesnt support graphics yet
    goto run_kernel;

    if (!VBE_Initialize()) {
        puts("Failed to initialize graphics.\r\n");
        return;
    }

run_kernel:
    KernelStart kernelStart = (KernelStart)kernel;
    kernelStart();
}
