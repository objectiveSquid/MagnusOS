#include "arrays.h"

int64_t findElementInArray(const uint16_t *array, const uint64_t arraySize, const uint16_t element) {
    for (uint64_t i = 0; i < arraySize; i++) {
        if (array[i] == element)
            return i;
    }

    return -1;
}
