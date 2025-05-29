#include "arch/i686/irq.h"
#include "disk/ata.h"
#include "disk/disk.h"
#include "disk/fat.h"
#include "hal/hal.h"
#include "memdefs.h"
#include "memory/allocator.h"
#include "memory/memdetect.h"
#include "pit/pit.h"
#include "ps2/ps2.h"
#include "util/memory.h"
#include "util/x86.h"
#include "visual/font.h"
#include "visual/graphics.h"
#include "visual/stdio.h"
#include "visual/vbe.h"
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
    ALLOCATOR_Initialize(memoryRegions, memoryRegionsCount, true);

    // initialize the graphics
    GRAPHICS_Initialize(vbeModeInfo);
    GRAPHICS_ClearScreen();

    // initialize the font
    if (!FONT_Initialize(vbeModeInfo)) {
        puts("Failed to initialize the font!\n"); // the fallback font might still work, so it's worth a try to log the error
        return;
    }
    FONT_SetPixelScale(2);

    HAL_Initialize();
    puts("Initialized the HAL! (gdt, idt, isr, irq)\n");

    PIT_Initialize();
    puts("Initialized the PIT driver!\n");

    // a little bit too buggy for now
    // PS2_Initialize();
    // puts("Initialized the PS2 driver!\n");

    DISK masterDisk;
    DISK slaveDisk;
    DISK_InitializeResult diskInitializeResult;
    DISK_Initialize(&diskInitializeResult, &masterDisk, &slaveDisk);
    if (!diskInitializeResult.initializedMasterDisk) {
        puts("Failed to initialize master disk!\n");
        return;
    }
    puts("Initialized disks!\n");

    Partition bootPartition;
    MBR_DetectPartition(&bootPartition, &masterDisk, partitionLBA, partitionSize);
    puts("Detected boot partition!\n");

    if (!FAT_Initialize(&bootPartition, true)) {
        puts("Failed to initialize FAT!\n");
        return;
    }
    puts("Initialized FAT!\n");

    // everything is now initialized
    clearScreen();

    puts("Hello from kernel!\n");

    for (;;)
        ;

    // deinitialize/free everything
    DISK_DeInitialize(&masterDisk);
    DISK_DeInitialize(&slaveDisk);
    FAT_DeInitialize();
    FONT_DeInitialize();
}
