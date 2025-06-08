#include "math.h"

uint32_t min(uint32_t firstNumber, uint32_t secondNumber) {
    if (firstNumber < secondNumber)
        return firstNumber;
    else
        return secondNumber;
}

// returns 0xFF (255) is no bits are set
uint8_t findLowestSetBit(uint64_t number) {
    if (number == 0)
        return 0xFF;

    uint8_t index = 0;
    while (!(number & 1) && index < 64) {
        number >>= 1;
        ++index;
    }

    return index;
}
