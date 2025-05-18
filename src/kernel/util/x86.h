#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ASMCALL __attribute__((__cdecl__))

void ASMCALL x86_OutByte(uint16_t port, uint8_t value);
uint8_t ASMCALL x86_InByte(uint16_t port);
void ASMCALL x86_OutWord(uint16_t port, uint16_t value);
uint16_t ASMCALL x86_InWord(uint16_t port);

void ASMCALL x86_EnableInterrupts();
void ASMCALL x86_DisableInterrupts();

void ASMCALL x86_Halt();
