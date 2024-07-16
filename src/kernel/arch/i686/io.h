#pragma once
#include <stdbool.h>
#include <stdint.h>

void __attribute__((cdecl)) x86_OutByte(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) x86_InByte(uint16_t port);
