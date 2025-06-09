#include "irq.h"
#include <lib/algorithm/arrays.h>
#include <lib/errors/errors.h>
#include <lib/interrupt/i8259/i8259.h>
#include <lib/interrupt/pic/pic.h>
#include <lib/x86/general.h>
#include <stddef.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQHandlers[16] = {NULL};
static const PICDriver *g_Driver = NULL;

void i686_IRQ_Handler(Registers *registers) {
    int irq = registers->interrupt - PIC_REMAP_OFFSET;

    if (g_IRQHandlers[irq] != NULL)
        g_IRQHandlers[irq](registers);

    g_Driver->sendEndOfInterrupt(irq);
}

int i686_IRQ_Initialize() {
    const PICDriver *drivers[] = {
        i8259_GetDriver(),
    };

    for (uint16_t i = 0; i < ARRAY_SIZE(drivers); ++i)
        if (drivers[i]->probe())
            g_Driver = drivers[i];

    if (g_Driver == NULL)
        return NO_PIC_DRIVER_FOUND;

    g_Driver->initialize(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8, false);

    for (uint8_t i = 0; i < 16; ++i)
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);

    x86_EnableInterrupts();

    return NO_ERROR;
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
