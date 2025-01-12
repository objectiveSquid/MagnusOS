#pragma once

#include "isr.h"
#include "pic.h"

typedef void (*IRQHandler)(Registers *registers);

void i686_IRQ_Initialize();
void i686_IRQ_RegisterHandler(int irq, IRQHandler handler);
void i686_IRQ_UnregisterHandler(int irq);
const PICDriver *i686_IRQ_GetDriver();
