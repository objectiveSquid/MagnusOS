#include "hal.h"
#include <lib/errors/errors.h>
#include <lib/interrupt/gdt/gdt.h>
#include <lib/interrupt/idt/idt.h>
#include <lib/interrupt/irq/irq.h>
#include <lib/interrupt/isr/isr.h>

int HAL_Initialize() {
    int status;

    i686_GDT_Initialize();
    i686_IDT_Initialize();
    i686_ISR_Initialize();

    if ((status = i686_IRQ_Initialize()) != NO_ERROR)
        return status;

    return NO_ERROR;
}
