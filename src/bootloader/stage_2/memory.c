#include "memory.h"
#include "stdbool.h"
#include "stdint.h"

void memcpy(void __far *destination, const void __far *source, uint16_t amount) {
    char __far *charDestination = (char __far *)destination;
    const char __far *charSource = (const char __far *)source;

    for (uint16_t i = 0; i < amount; ++i)
        charDestination[i] = charSource[i];
}

void memset(void __far *pointer, char value, uint16_t amount) {
    char __far *charPointer = (char __far *)pointer;
    for (uint16_t i = 0; i < amount; ++i)
        charPointer[i] = value;
}

bool memcmp(const void __far *pointer1, const void __far *pointer2, uint16_t amount) {
    const char __far *charPointer1 = (const char __far *)pointer1;
    const char __far *charPointer2 = (const char __far *)pointer2;

    for (uint16_t i = 0; i < amount; ++i)
        if (charPointer1[i] != charPointer2[i])
            return false;

    return true;
}
