#pragma once
#include <stdint.h>

#define i686_DisableInterrupts() asm("cli")
#define i686_EnableInterrupts() asm("sti")

// ASM functions
void __attribute__((cdecl)) i686_OutByte(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) i686_InByte(uint16_t port);

// C functions
void i686_IOWait();
