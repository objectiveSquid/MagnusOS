#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GET_BIT(array, bit) (array[(bit) / 8] & (1 << ((bit) % 8)))
#define SET_BIT(array, bit) (array[(bit) / 8] |= (1 << ((bit) % 8)))
#define UNSET_BIT(array, bit) (array[(bit) / 8] &= ~(1 << ((bit) % 8)))

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof((array)[0])))

#define DIV_ROUND_UP(a, b) (((a) + (b) - 1) / (b))
#define BITS2BYTES(bits) DIV_ROUND_UP(bits, 8)

uint32_t min(uint32_t firstNumber, uint32_t secondNumber);
char toUpper(char character);
uint8_t findLowestSetBit(uint64_t number);
