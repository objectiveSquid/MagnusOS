#pragma once
#include <stdint.h>

typedef struct {
    uint32_t ds;                                            // data segment pushed in isr_common
    uint32_t edi, esi, ebp, kernel_esp, ebx, edx, ecx, eax; // pusha
    uint32_t interrupt, error;                              // we pushed in interrupt asm routine
    uint32_t eip, cs, eflags, esp, ss;                      // pushed by the cpu
} __attribute__((packed)) Registers;

typedef void (*ISRHandler)(Registers *registers);

void i686_ISR_Initialize();
void i686_ISR_RegisterHandler(int interrupt);
