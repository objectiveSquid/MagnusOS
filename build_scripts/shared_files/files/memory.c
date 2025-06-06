#include "memory.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void memcpy(void *destination, const void *source, size_t amount) {
    char *charDestination = (char *)destination;
    const char *charSource = (const char *)source;

    for (size_t i = 0; i < amount; ++i)
        charDestination[i] = charSource[i];
}

void memset(void *pointer, char value, size_t amount) {
    char *charPointer = (char *)pointer;
    for (size_t i = 0; i < amount; ++i)
        charPointer[i] = value;
}

int memcmp(const void *pointer1, const void *pointer2, size_t amount) {
    const char *charPointer1 = (const char *)pointer1;
    const char *charPointer2 = (const char *)pointer2;

    for (size_t i = 0; i < amount; ++i)
        if (charPointer1[i] != charPointer2[i])
            return 1;

    return 0;
}
