#include "font.h"
#include "8x8_font.h"
#include "graphics.h"
#include "memdefs.h"
#include "memory.h"
#include "vbe.h"
#include <stdint.h>

static FONT_Character *g_ScreenCharacterBuffer = (FONT_Character *)SCREEN_CHARACTER_BUFFER;
static VbeModeInfo *g_VbeModeInfo = (VbeModeInfo *)MEMORY_VESA_MODE_INFO;

FONT_Character FONT_GetCharacter(uint16_t x, uint16_t y) {
    FONT_Character output = EMPTY_CHARACTER;

    if (!VBE_VerifyInitialized()) {
        return output;
    }

    return g_ScreenCharacterBuffer[(y * SCREEN_CHARACTER_WIDTH(g_VbeModeInfo)) + x];
}

void FONT_ScrollBack(uint16_t lineCount) {
    if (!VBE_VerifyInitialized()) {
        return;
    }

    // copy lines
    for (uint16_t y = lineCount; y < SCREEN_CHARACTER_HEIGHT(g_VbeModeInfo); ++y)
        for (uint16_t x = 0; x < SCREEN_CHARACTER_WIDTH(g_VbeModeInfo); ++x) {
            FONT_PutCharacter(x, y - lineCount, FONT_GetCharacter(x, y));
        }

    // delete last lines
    for (uint16_t y = SCREEN_CHARACTER_HEIGHT(g_VbeModeInfo) - lineCount; y < SCREEN_CHARACTER_HEIGHT(g_VbeModeInfo); ++y)
        for (uint16_t x = 0; x < SCREEN_CHARACTER_WIDTH(g_VbeModeInfo); ++x) {
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
                GRAPHICS_WritePixel((x * 8) + img_x, (y * 8) + img_y, character.typed.r, character.typed.g, character.typed.b);
            else
                GRAPHICS_WritePixel((x * 8) + img_x, (y * 8) + img_y, 0, 0, 0);
        }
    }
}
