#include <hal/hal.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>

extern char __bss_start;
extern char __end;

void __attribute__((section(".entry"))) start(uint16_t bootDrive) {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));

    printf("Hello from Kernel!\n");
    HAL_Initialize();

end:
    i686_Halt();
}
