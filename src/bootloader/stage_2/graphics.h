#pragma once

#include "vbe.h"

#define EMPTY_CHARACTER {.typed.character = ' ', .typed.r = 255, .typed.g = 255, .typed.b = 255}

typedef union {
    uint32_t data;

    struct {
        char character;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } typed;
} FONT_Character;

void FONT_PutCharacter(VbeModeInfo *vbeModeInfo, uint16_t x, uint16_t y, FONT_Character character);
void FONT_WriteCharacter(VbeModeInfo *vbeModeInfo, FONT_Character character);
void writePixel(VbeModeInfo *vbeModeInfo, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
