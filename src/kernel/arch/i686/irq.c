#include "irq.h"
#include "io.h"
#include "pic.h"
#include <stddef.h>
#include <stdio.h>

#define PIC_REMAP_OFFSET 0x20

IRQHandler g_IRQHandlers[16];

void i686_IRQ_Handler(Registers *registers) {
    irq_t irq = registers->interrupt - PIC_REMAP_OFFSET;

    if (g_IRQHandlers[irq] != NULL)
        g_IRQHandlers[irq](registers);
    else
        printf("Unhandled IRQ %u\n", irq);

    i686_PIC_SendEndOfInterrupt(irq);
}

void i686_IRQ_Initialize() {
    i686_PIC_Configure(PIC_REMAP_OFFSET, PIC_REMAP_OFFSET + 8);

    for (uint8_t i = 0; i < 16; ++i)
        i686_ISR_RegisterHandler(PIC_REMAP_OFFSET + i, i686_IRQ_Handler);

    i686_EnableInterrupts();
};
void i686_IRQ_RegisterHandler(irq_t irq, IRQHandler handler) {
    g_IRQHandlers[irq] = handler;
}
