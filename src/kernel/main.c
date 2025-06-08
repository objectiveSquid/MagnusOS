#include "arch/i686/irq.h"
#include "disk/ata.h"
#include "disk/disk.h"
#include "disk/fat.h"
#include "hal/hal.h"
#include "memory/allocator.h"
#include "pit/pit.h"
#include "ps2/ps2.h"
#include "util/x86.h"
#include "visual/font.h"
#include "visual/graphics.h"
#include "visual/stdio.h"
#include "visual/vbe.h"
#include <lib/errors/errors.h>
#include <lib/memory/memdefs.h>
#include <lib/memory/memdetect.h>
#include <lib/memory/memory.h>
#include <stdint.h>

extern void _init();

void start(uint8_t bootDrive,
           MEMDETECT_MemoryRegion *memoryRegions,
           uint32_t memoryRegionsCount,
           uint32_t partitionLBA,
           uint32_t partitionSize,
           VbeModeInfo *vbeModeInfo) {
    _init(); // call global constructors

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

    HAL_Initialize();
    puts("Initialized the HAL! (gdt, idt, isr, irq)\n");

    PIT_Initialize();
    puts("Initialized the PIT driver!\n");

    if ((status = PS2_Initialize()) != NO_ERROR) {
        printf("Failed to initialize the PS2 driver! Status: %d\n", status);
        return;
    };
    puts("Initialized the PS2 driver!\n");

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
        printf("Failed to initialize FAT! Status: %d\n", status);
        return;
    }
    puts("Initialized FAT!\n");

    // everything is now initialized
    clearScreen();

    puts("Hello from kernel!\n");

    // deinitialize/free everything, technically not needed, but ill do it anyway for good measure
    DISK_DeInitialize(&masterDisk);
    DISK_DeInitialize(&slaveDisk);
    free(bootFilesystem);
    GRAPHICS_DeInitialize();
    FONT_DeInitialize();
}
