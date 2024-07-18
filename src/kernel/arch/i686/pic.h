#pragma once
#include "irq.h"
#include <stdint.h>

typedef struct {
    union {
        struct {
            uint8_t first;
            uint8_t second;
        };
        uint16_t both;
    };
} Split_uint16_t;

typedef Split_uint16_t IRQRequestRegisters;
typedef Split_uint16_t InServiceRegisters;

void i686_PIC_Configure(uint8_t offsetPic_1, uint8_t offsetPic_2);
void i686_PIC_SendEndOfInterrupt(irq_t irq);
void i686_PIC_Unmask(irq_t irq);
IRQRequestRegisters i686_PIC_GetIRQRequestRegister();
InServiceRegisters i686_PIC_GetInServiceRegister();
