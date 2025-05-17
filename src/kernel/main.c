#include "arch/i686/irq.h"
#include "disk/disk.h"
#include "disk/fat.h"
#include "hal/hal.h"
#include "memdefs.h"
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

void __attribute__((section(".entry"))) start() {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));
    _init(); // call global constructors

    GRAPHICS_ClearScreen();
    FONT_SetPixelScale(2);
    HAL_Initialize();
    PIT_Initialize();
    clearScreen();

    DISK *disk = (DISK *)MEMORY_FAT12_DISK_BUFFER;
    if (!DISK_Initialize(disk->id)) {
        printf("Failed to initialize disk!\n");
        return;
    }
    printf("Initialized disk!\n");

    if (!FAT_Initialize(disk)) {
        printf("Failed to initialize FAT!\n");
        return;
    }
    printf("Initialized FAT!\n");

    printf("Hello from kernel!\n");

    for (;;)
        ;

    x86_Halt();
}
