#pragma once

#include <stdint.h>

#define DIV_ROUND_UP(a, b) (((a) + (b) - 1) / (b))
#define BITS2BYTES(bits) DIV_ROUND_UP(bits, 8)

uint32_t min(uint32_t firstNumber, uint32_t secondNumber);
uint32_t max(uint32_t firstNumber, uint32_t secondNumber);
uint8_t findLowestSetBit(uint64_t number);
