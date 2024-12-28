#pragma once
#include "vbe.h"
#include <stdint.h>

#define EMPTY_CHARACTER {.typed.character = ' ', .typed.r = 255, .typed.g = 255, .typed.b = 255}

#define SCREEN_CHARACTER_WIDTH(vbeModeInfo) (vbeModeInfo->width / 8)
#define SCREEN_CHARACTER_HEIGHT(vbeModeInfo) (vbeModeInfo->height / 8)

typedef union {
    uint32_t data;

    struct {
        char character;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } typed;
} FONT_Character;

void FONT_PutCharacter(uint16_t x, uint16_t y, FONT_Character character);
void FONT_ScrollBack(uint16_t lineCount);