#pragma once
#include <stdbool.h>
#include <stdint.h>

void memcpy(void *destination, const void *source, uint16_t amount);
void memset(void *pointer, char value, uint16_t amount);
// returns true if the pointers are the same
bool memcmp(const void *pointer1, const void *pointer2, uint16_t amount);
