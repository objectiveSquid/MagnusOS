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
    // PS2_Initialize(); a little bit too buggy for now

    DISK masterDisk;
    DISK slaveDisk;
    DISK_InitializeResult diskInitializeResult;
    DISK_Initialize(&diskInitializeResult, &masterDisk, &slaveDisk);

    if (!diskInitializeResult.initializedMasterDisk) {
        puts("Failed to initialize master (main) disk!\n");
        return;
    }
    puts("Initialized disks!\n");

    if (!FAT_Initialize(&masterDisk)) {
        puts("Failed to initialize FAT!\n");
        return;
    }
    puts("Initialized FAT!\n");

    char aaa[512];
    FAT_File *fd = FAT_Open(&masterDisk, "hello.txt");
    if (fd == NULL) {
        puts("Failed to open file!\n");
        return;
    }
    FAT_Read(&masterDisk, fd, 100, aaa);
    FAT_Close(fd);
    puts(aaa);
    goto halt;

    // TODO: check if other fonts can be loaded

    // everything is now initialized
    clearScreen();

    puts("Hello from kernel!\n");

halt:
    x86_Halt();
}
