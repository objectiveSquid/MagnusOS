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
#include <stdint.h>

extern void _init();

extern char __bss_start;
extern char __end;

void __attribute__((section(".entry"))) start(uint8_t bootDrive, MEMDETECT_MemoryRegion *memoryRegions, uint32_t memoryRegionsCount, uint32_t partitionLBA, uint32_t partitionSize) {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));
    _init(); // call global constructors

    GRAPHICS_ClearScreen();
    FONT_SetPixelScale(2);

    HAL_Initialize();
    puts("Initialized the HAL! (gdt, idt, isr, irq)\n");

    PIT_Initialize();
    puts("Initialized the PIT driver!\n");

    // a little bit too buggy for now
    // PS2_Initialize();
    // puts("Initialized the PS2 driver!\n");

    ALLOCATOR_Initialize(memoryRegions, memoryRegionsCount);
    puts("Initialized allocator!\n");

    DISK masterDisk;
    DISK slaveDisk;
    DISK_InitializeResult diskInitializeResult;
    DISK_Initialize(&diskInitializeResult, &masterDisk, &slaveDisk);
    if (!diskInitializeResult.initializedMasterDisk) {
        puts("Failed to initialize master disk!\n");
        return;
    }
    puts("Initialized disks!\n");

    if (!FAT_Initialize(&masterDisk, partitionLBA)) {
        puts("Failed to initialize FAT!\n");
        return;
    }
    puts("Initialized FAT!\n");

    // TODO: check if other fonts can be loaded

    // everything is now initialized
    clearScreen();

    puts("Hello from kernel!\n");
}
