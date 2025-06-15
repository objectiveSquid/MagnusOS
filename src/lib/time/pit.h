#pragma once

#include <stdint.h>

void PIT_Initialize();
void PIT_Delay(uint64_t milliseconds);
uint64_t PIT_GetTimeMs();
