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

    // everything is now initialized
    clearScreen();

    puts("Hello from kernel!\n");

    // deinitialize/free everything, technically not needed, but ill do it anyway for good measure
    GRAPHICS_DeInitialize();
    FONT_DeInitialize();
}
