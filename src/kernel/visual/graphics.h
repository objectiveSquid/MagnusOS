#pragma once

#include "vbe.h"
#include <stdint.h>

int GRAPHICS_Initialize(VbeModeInfo *vbeModeInfo, void **videoBufferOutput);
void GRAPHICS_DeInitialize();
void GRAPHICS_WriteScalePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint16_t scale);
void GRAPHICS_WritePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
void GRAPHICS_ClearScreen();
void GRAPHICS_PushBufferRectangleScale(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t scale);
void GRAPHICS_PushBufferRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void GRAPHICS_PushBuffer();
