#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "x86.h"
#include <stdint.h>

char *kernelLoadBuffer = (char *)MEMORY_LOAD_KERNEL;
char *kernel = (char *)MEMORY_KERNEL_ADDRESS;

typedef void (*KernelStart)();

void __attribute__((cdecl))
cstart(uint16_t bootDrive) {
    clearScreen();

    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive)) {
        puts("Failed to initialize disk.\r\n");
        return;
    }

    if (!FAT_Initialize(&disk)) {
        puts("Failed to initialize FAT.\r\n");
        goto end;
    }

    FAT_File *kernelFd = FAT_Open(&disk, "kernel.bin");
    FAT_DirectoryEntry entry;
    uint32_t readCount;
    char *kernelBuffer = kernel;
    while (readCount = FAT_Read(&disk, kernelFd, MEMORY_LOAD_SIZE, kernelLoadBuffer)) {
        memcpy(kernelBuffer, kernelLoadBuffer, readCount);
        kernelBuffer += readCount;
    }
    FAT_Close(kernelFd);

    KernelStart kernelStart = (KernelStart)kernel;
    kernelStart();

end:
    asm(
        "cli;"
        "hlt;");
}
