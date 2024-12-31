#include "arrays.h"
#include <stdint.h>

int64_t findElementInArray(const uint64_t *array, const uint64_t arraySize, const uint64_t element) {
    for (uint64_t i = 0; i < arraySize; i++) {
        if (array[i] == element)
            return i;
    }

    return -1;
}
