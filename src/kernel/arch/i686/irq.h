#pragma once
#include "isr.h"

typedef int irq_t;

typedef void (*IRQHandler)(Registers *registers);

void i686_IRQ_Initialize();
void i686_IRQ_RegisterHandler(irq_t irq, IRQHandler handler);
