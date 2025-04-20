#include "arch/i686/irq.h"
#include "disk/disk.h"
#include "disk/fat.h"
#include "disk/fdc.h"
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

extern char __bss_start;
extern char __end;

void __attribute__((section(".entry"))) start() {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));

    GRAPHICS_ClearScreen();
    FONT_SetPixelScale(2);
    HAL_Initialize();
    PIT_Initialize();
    if (!PS2_Initialize()) {
        printf("Failed to initialize PS2!\n");
        return;
    }

    DISK *disk = (DISK *)MEMORY_DISK_INFO_BUFFER;
    if (!DISK_Initialize(disk->id)) {
        printf("Failed to initialize disk!\n");
        return;
    }
    printf("Initialized disk!\n");
    char aaa[513];
    memset(aaa, 0, 513);
    if (!DISK_ReadSectors(disk, 0, 1, aaa)) {
        printf("Disk read failed! lba=0\n");
        for (;;)
            ;
    }
    printf("Read from disk!\n");
    for (;;)
        ;

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
