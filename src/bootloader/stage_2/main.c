#include "disk.h"
#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "visual/font.h"
#include "visual/graphics.h"
#include "visual/stdio.h"
#include "visual/vbe.h"
#include "visual/vga.h"
#include "x86.h"
#include <stdint.h>

#define MEMORY_LOAD_KERNEL_CHUNK_SIZE 0x00010000

typedef void (*KernelStart)();

static uint8_t *kernel = (uint8_t *)MEMORY_KERNEL_ADDRESS;
static uint8_t *g_FontBits = (uint8_t *)MEMORY_RASTERFONT_BITS;

void ASMCALL cstart(uint16_t bootDrive) {
    VGA_ClearScreen();

    DISK *disk = (DISK *)MEMORY_FAT12_DISK_BUFFER;
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

    // kernel doesnt support graphics yet
    goto run_kernel;

    if (!VBE_Initialize()) {
        puts("Failed to initialize graphics.\r\n");
        return;
    }

    setCursorPosition(0, 0);
    FONT_SetPixelScale(2);
    FONT_SetFont(FONT_FindFontInfo(NULL, 8, 16), false);
    printf("Hello from 8x16 font scaled to double size\n");

    for (;;)
        ;

run_kernel:
    KernelStart kernelStart = (KernelStart)kernel;
    kernelStart();
}
