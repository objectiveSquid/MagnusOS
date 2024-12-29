#include "font.h"
#include "8x8_font.h"
#include "graphics.h"
#include "memdefs.h"
#include "memory.h"
#include "vbe.h"
#include <stdint.h>

static FONT_Character *g_ScreenCharacterBuffer = (FONT_Character *)SCREEN_CHARACTER_BUFFER;
static VbeModeInfo *g_VbeModeInfo = (VbeModeInfo *)MEMORY_VESA_MODE_INFO;
static uint8_t g_FontPixelScale = 1;

// next 2 functions are because i dont really understand variable sharing across files in c
void FONT_SetPixelScale(uint8_t scale) {
    g_FontPixelScale = scale;
}

uint8_t FONT_GetPixelScale() {
    return g_FontPixelScale;
}

FONT_Character FONT_GetCharacter(uint16_t x, uint16_t y) {
    FONT_Character output = EMPTY_CHARACTER;

    if (!VBE_VerifyInitialized()) {
        return output;
    }

    return g_ScreenCharacterBuffer[(y * FONT_ScreenCharacterWidth()) + x];
}

void FONT_ScrollBack(uint16_t lineCount) {
    if (!VBE_VerifyInitialized()) {
        return;
    }

    // copy lines
    for (uint16_t y = lineCount; y < FONT_ScreenCharacterHeight(); ++y)
        for (uint16_t x = 0; x < FONT_ScreenCharacterWidth(); ++x) {
            FONT_PutCharacter(x, y - lineCount, FONT_GetCharacter(x, y));
        }

    // delete last lines
    for (uint16_t y = FONT_ScreenCharacterHeight() - lineCount; y < FONT_ScreenCharacterHeight(); ++y)
        for (uint16_t x = 0; x < FONT_ScreenCharacterWidth(); ++x) {
            FONT_Character tempCharacter = EMPTY_CHARACTER;
            FONT_PutCharacter(x, y, tempCharacter);
        }
}

void FONT_PutCharacter(uint16_t x, uint16_t y, FONT_Character character) {
    if (!VBE_VerifyInitialized()) {
        return;
    }

    for (uint8_t img_x = 0; img_x < 8; ++img_x) {
        for (uint8_t img_y = 0; img_y < 8; ++img_y) {
            if (CONSOLE_FONT_8x8[character.typed.character * 8 + img_y] & (0b10000000 >> img_x))
                GRAPHICS_WriteScalePixel((x * 8) + img_x, (y * 8) + img_y, character.typed.r, character.typed.g, character.typed.b, g_FontPixelScale);
            else
                GRAPHICS_WriteScalePixel((x * 8) + img_x, (y * 8) + img_y, 0, 0, 0, g_FontPixelScale);
        }
    }
}

uint16_t FONT_ScreenCharacterWidth() {
    if (!VBE_VerifyInitialized()) {
        return 0;
    }

    return (g_VbeModeInfo->width / 8) / g_FontPixelScale;
}

uint16_t FONT_ScreenCharacterHeight() {
    if (!VBE_VerifyInitialized()) {
        return 0;
    }

    return (g_VbeModeInfo->height / 8) / g_FontPixelScale;
}
