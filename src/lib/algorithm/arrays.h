#pragma once

#include <stdint.h>

#define GET_ARRAY_BIT(array, bit) (array[(bit) / 8] & (1 << ((bit) % 8)))
#define SET_ARRAY_BIT(array, bit) (array[(bit) / 8] |= (1 << ((bit) % 8)))
#define UNSET_ARRAY_BIT(array, bit) (array[(bit) / 8] &= ~(1 << ((bit) % 8)))

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof((array)[0])))

int64_t findElementInArray(const uint16_t *array, const uint64_t arraySize, const uint16_t element);
