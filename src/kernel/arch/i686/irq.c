#include "irq.h"
#include "i8259.h"
#include "io.h"
#include <stddef.h>
#include <stdio.h>
#include <util/arrays.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQHandlers[16];
static const PICDriver *g_Driver = NULL;

void i686_IRQ_Handler(Registers *registers) {
    irq_t irq = registers->interrupt - PIC_REMAP_OFFSET;

    if (g_IRQHandlers[irq] != NULL)
        g_IRQHandlers[irq](registers);
    else
        printf("Unhandled IRQ %u\n", irq);

    g_Driver->sendEndOfInterrupt(irq);
}

void i686_IRQ_Initialize() {
    const PICDriver *drivers[] = {
        i8259_GetDriver(),
    };

    for (uint16_t i = 0; i < ARRAY_SIZE(drivers); ++i)
        if (drivers[i]->probe())
            g_Driver = drivers[i];

    if (g_Driver == NULL) {
        puts("Warning: No PIC driver found!\n");
        return;
    }

    printf("Found PIC driver %s\n", g_Driver->name);
    g_Driver->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, false);

    for (uint8_t i = 0; i < 16; ++i)
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);

    i686_EnableInterrupts();
}

void i686_IRQ_RegisterHandler(irq_t irq, IRQHandler handler) {
    g_IRQHandlers[irq] = handler;
}
