#pragma once
#include "stdbool.h"
#include "stdint.h"

uint32_t align(uint32_t number, uint32_t alignTo);
uint32_t min(uint32_t firstNumber, uint32_t secondNumber);
uint32_t max(uint32_t firstNumber, uint32_t secondNumber);

bool isLower(char character);
char toUpper(char character);
