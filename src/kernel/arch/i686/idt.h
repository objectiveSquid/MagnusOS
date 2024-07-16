#pragma once
#include <stdint.h>

typedef enum {
    IDT_FLAG_GATE_TASK = 0x5,
    IDT_FLAG_GATE_16_BIT_INT = 0x6,
    IDT_FLAG_GATE_16_BIT_TRAP = 0x7,
    IDT_FLAG_GATE_32_BIT_INT = 0xE,
    IDT_FLAG_GATE_32_BIT_TRAP = 0xF,

    IDT_FLAG_RING_0 = (0 << 5),
    IDT_FLAG_RING_1 = (1 << 5),
    IDT_FLAG_RING_2 = (2 << 5),
    IDT_FLAG_RING_3 = (3 << 5),

    IDT_FLAG_PRESENT = 0x80
} IDT_FLAGS;

void i686_IDT_SetGate(int interrupt, void *base, uint16_t segmentDescriptor, uint8_t flags);
void i686_IDT_EnableGate(int interrupt);
void i686_IDT_DisableGate(int interrupt);
void i686_IDT_Initialize();
