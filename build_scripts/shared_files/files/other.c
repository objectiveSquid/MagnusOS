#include "other.h"
#include <stdbool.h>
#include <stdint.h>

uint32_t min(uint32_t firstNumber, uint32_t secondNumber) {
    if (firstNumber < secondNumber)
        return firstNumber;
    else
        return secondNumber;
}

bool isLower(char character) {
    return character >= 'a' && character <= 'z';
}

char toUpper(char character) {
    if (isLower(character))
        return character - 32;
    return character;
}
