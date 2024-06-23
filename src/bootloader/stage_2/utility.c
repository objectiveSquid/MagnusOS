#include "utility.h"
#include "stdbool.h"
#include "stdint.h"

uint32_t align(uint32_t number, uint32_t alignTo) {
    if (alignTo == 0)
        return number;

    uint32_t remainder = number & alignTo;
    if (remainder > 0)
        return number + alignTo - remainder;
    else
        return number;
}

uint32_t min(uint32_t firstNumber, uint32_t secondNumber) {
    if (firstNumber < secondNumber)
        return firstNumber;
    else
        return secondNumber;
}
uint32_t max(uint32_t firstNumber, uint32_t secondNumber) {
    if (firstNumber > secondNumber)
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
