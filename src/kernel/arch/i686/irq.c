#include "irq.h"
#include "i8259.h"
#include "io.h"
#include "pic.h"
#include "stdio.h"
#include "util/arrays.h"
#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQHandlers[16] = {NULL};
static const PICDriver *g_Driver = NULL;

void i686_IRQ_Handler(Registers *registers) {
    int irq = registers->interrupt - PIC_REMAP_OFFSET;

    if (g_IRQHandlers[irq] != NULL)
        g_IRQHandlers[irq](registers);
    else
        printf("Unhandled IRQ %d\n", irq);

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

const PICDriver *i686_IRQ_GetDriver() {
    return g_Driver;
}

void i686_IRQ_RegisterHandler(int irq, IRQHandler handler) {
    g_IRQHandlers[irq] = handler;
}

void i686_IRQ_UnregisterHandler(int irq) {
    g_IRQHandlers[irq] = NULL;
}