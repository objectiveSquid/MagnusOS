#pragma once

#include "util/misc.h"
#include <stdint.h>

uint8_t ASMCALL i686_EnableInterrupts();
uint8_t ASMCALL i686_DisableInterrupts();

// ASM functions
void ASMCALL i686_OutByte(uint16_t port, uint8_t value);
uint8_t ASMCALL i686_InByte(uint16_t port);

// C functions
void i686_IOWait();
