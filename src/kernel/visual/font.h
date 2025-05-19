#pragma once

#include "disk/disk.h"
#include <stdbool.h>
#include <stdint.h>

#define EMPTY_CHARACTER {.typed.character = 0, .typed.r = 255, .typed.g = 255, .typed.b = 255}

typedef union {
    uint32_t data;

    struct {
        char character;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } typed;
} FONT_Character;

typedef struct {
    const char filename[11];
    uint8_t width;
    uint8_t height;
} FONT_FontInfo;

const FONT_FontInfo *FONT_FindFontInfo(const char *filename, int16_t width, int16_t height);
void FONT_SetFont(DISK *fontsDisk, const FONT_FontInfo *fontInfo, bool reDraw);
void FONT_PutCharacter(uint16_t x, uint16_t y, FONT_Character character);
void FONT_ScrollBack(uint16_t lineCount);
uint16_t FONT_ScreenCharacterWidth();
uint16_t FONT_ScreenCharacterHeight();
void FONT_SetPixelScale(uint8_t scale);
uint8_t FONT_GetPixelScale();