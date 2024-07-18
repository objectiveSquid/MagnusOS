#include <arch/i686/irq.h>
#include <arch/i686/misc.h>
#include <hal/hal.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>

extern char __bss_start;
extern char __end;

void onTimer(Registers *registers) {
    printf("Timer hit!\n");
}

void __attribute__((section(".entry"))) start(uint16_t bootDrive) {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));

    printf("Hello from Kernel!\n");
    HAL_Initialize();

    i686_IRQ_RegisterHandler(0, onTimer);
    for (;;)
        ;

end:
    i686_Halt();
}
