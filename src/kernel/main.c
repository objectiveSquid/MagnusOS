#include "arch/i686/irq.h"
#include "arch/i686/misc.h"
#include "hal/hal.h"
#include "memory.h"
#include "ps2.h"
#include "stdio.h"
#include <stdint.h>

extern char __bss_start;
extern char __end;

void __attribute__((section(".entry"))) start() {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));

    HAL_Initialize();
    clearScreen();

    printf("Hello from kernel!\n");

    PS2_Initialize();
    for (;;)
        ;

end:
    i686_Halt();
}
