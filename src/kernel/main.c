#include "hal/hal.h"
#include "ps2/ps2.h"
#include "visual/font.h"
#include "visual/graphics.h"
#include "visual/stdio.h"
#include "visual/vbe.h"
#include <lib/disk/ata.h>
#include <lib/disk/disk.h>
#include <lib/disk/fat.h>
#include <lib/errors/errors.h>
#include <lib/memory/allocator.h>
#include <lib/memory/memdefs.h>
#include <lib/memory/memdetect.h>
#include <lib/memory/memory.h>
#include <lib/time/pit.h>
#include <stdint.h>

extern char __bss_start;
extern char __bss_stop;

void start(uint8_t bootDrive,
           MEMDETECT_MemoryRegion *memoryRegions,
           uint32_t memoryRegionsCount,
           uint32_t partitionLBA,
           uint32_t partitionSize,
           VbeModeInfo *vbeModeInfo) {
    memset(&__bss_start, '\0', (&__bss_stop) - (&__bss_start));

    // in use bits already initialized by stage 2
    int status;
    if ((status = ALLOCATOR_Initialize(memoryRegions, memoryRegionsCount, true)) != NO_ERROR) {
        printf("Failed to initialize the allocator! Status: %d\n", status);
        return;
    }

    void *videoBuffer;
    // initialize the graphics
    if ((status = GRAPHICS_Initialize(vbeModeInfo, &videoBuffer)) != NO_ERROR) {
        printf("Failed to initialize the graphics! Status: %d\n", status);
        return;
    }

    // initialize the font
    if ((status = FONT_Initialize(vbeModeInfo, videoBuffer)) != NO_ERROR) {
        printf("Failed to initialize the font! Status: %d\n", status); // the fallback font might still work, so it's worth a try to log the error
        return;
    }
    FONT_SetPixelScale(2);

    if ((status = HAL_Initialize()) != NO_ERROR) {
        printf("Failed to initialize the HAL! Status: %d\n", status);
        return;
    }
    puts("Initialized the HAL! (gdt, idt, isr, irq)\n");

    PIT_Initialize();
    puts("Initialized the PIT driver!\n");

    if ((status = PS2_Initialize()) != NO_ERROR) {
        printf("Failed to initialize the PS2 driver! Status: %d\n", status);
        return;
    };
    puts("Initialized the PS2 driver!\n");

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

    // everything is now initialized
    clearScreen();

    puts("Hello from kernel!\n");

    // deinitialize/free everything, technically not needed, but ill do it anyway for good measure
    GRAPHICS_DeInitialize();
    FONT_DeInitialize();
}
