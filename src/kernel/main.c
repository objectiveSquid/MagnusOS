#include "memory.h"
#include "stdio.h"
#include <stdint.h>

extern char __bss_start;
extern char __end;

void __attribute__((section(".entry"))) cstart(uint16_t bootDrive) {
    memset(&__bss_start, '\0', (&__end) - (&__bss_start));

    printf("Hello from Kernel!\n");

end:
    asm(
        "cli;"
        "hlt;");
}
