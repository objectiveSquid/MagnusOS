#pragma once

#include <stdint.h>

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof((array)[0])))

uint32_t min(uint32_t firstNumber, uint32_t secondNumber);

char toUpper(char character);

uint8_t findLowestSetBit(uint64_t number);
