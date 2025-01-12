#pragma once

#include <stdint.h>

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof(array[0])))

int64_t findElementInArray(const uint16_t *array, const uint64_t arraySize, const uint16_t element);
