#pragma once

#include <lib/interrupt/isr/isr.h>
#include <lib/interrupt/pic/pic.h>

typedef void (*IRQHandler)(Registers *registers);

int i686_IRQ_Initialize();
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler);
void i686_IRQ_UnregisterHandler(int irq);
const PICDriver *i686_IRQ_GetDriver();
