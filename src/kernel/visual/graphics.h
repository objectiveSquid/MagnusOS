#pragma once

#include "vbe.h"
#include <stdint.h>

void GRAPHICS_Initialize(VbeModeInfo *vbeModeInfo);
void GRAPHICS_WriteScalePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t scale);
void GRAPHICS_WritePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void GRAPHICS_ClearScreen();
