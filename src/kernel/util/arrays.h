#pragma once

#include <stdint.h>

#define ARRAY_SIZE(array) ((sizeof(array)) / (sizeof(array[0])))

int64_t findElementInArray(const uint64_t *array, const uint64_t arraySize, const uint64_t element);