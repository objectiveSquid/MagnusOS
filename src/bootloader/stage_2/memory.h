#pragma once
#include "stdbool.h"
#include "stdint.h"

void memcpy(void __far *destination, const void __far *source, uint16_t amount);
void memset(void __far *pointer, char value, uint16_t amount);
// returns true if the pointers are the same
bool memcmp(const void __far *pointer1, const void __far *pointer2, uint16_t amount);
