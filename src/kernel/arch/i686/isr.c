#include "isr.h"
#include "gdt.h"
#include "idt.h"
#include "isr_gen.h"
#include "misc.h"
#include <stdio.h>

ISRHandler g_ISRHandlers[256];

static const char *const g_Exceptions[] = {
    "Divide by zero error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "",
    "",
    "",
    "",
    "",
    "",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "",
};

void i686_ISR_Initialize() {
    i686_ISR_InitializeGates();
    for (uint16_t interruptNumber = 0; interruptNumber < 256; ++interruptNumber)
        i686_IDT_EnableGate(interruptNumber);
}

void __attribute__((cdecl)) i686_ISR_Handler(Registers *registers) {
    if (g_ISRHandlers[registers->interrupt])
        g_ISRHandlers[registers->interrupt](registers);
    else if (registers->interrupt >= 32)
        printf("Unhandled interrupt %lu\n", registers->interrupt);
    else {
        printf("Unhandled interrupt %lu, %s\n", registers->interrupt, g_Exceptions[registers->interrupt]);

        printf("  eax=%x  ebx=%x  ecx=%x  edx=%x  esi=%x  edi=%x\n",
               registers->eax, registers->ebx, registers->ecx,
               registers->edx, registers->esi, registers->edi);
        printf("  esp=%x  ebp=%x  eip=%x  eflags=%x  cs=%x  ds=%x  ss=%x\n",
               registers->esp, registers->ebp, registers->eip,
               registers->eflags, registers->cs, registers->ds,
               registers->ss);
        printf("  interrupt=%x  errorcode=%x\n", registers->interrupt, registers->error);
        printf("!!! KERNEL PANIC !!!\n");
        i686_Halt();
    }
}

void i686_ISR_RegisterHandler(uint16_t interrupt, ISRHandler handler) {
    g_ISRHandlers[interrupt] = handler;
    i686_IDT_EnableGate(interrupt);
}