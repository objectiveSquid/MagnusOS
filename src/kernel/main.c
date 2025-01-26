#include "arch/i686/irq.h"
#include "hal/hal.h"
#include "ps2/ps2.h"
#include "util/memory.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include <stdint.h>

extern char __bss_start;
extern char __end;

void __attribute__((section(".entry"))) start() {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));

    clearScreen();
    HAL_Initialize();
    PS2_Initialize();

    printf("Hello from kernel!\n");

    for (;;)
        ;

    x86_Halt();
}
