#include "graphics.h"
#include "font.h"
#include "memdefs.h"
#include "memory.h"
#include "vbe.h"

#define TAB_SIZE 4
#define SCREEN_CHARACTER_WIDTH(vbeModeInfo) (vbeModeInfo->width / 8)
#define SCREEN_CHARACTER_HEIGHT(vbeModeInfo) (vbeModeInfo->height / 8)

static uint16_t g_cursorPosition[2] = {0, 0};
static FONT_Character *g_ScreenCharacterBuffer = (FONT_Character *)SCREEN_CHARACTER_BUFFER;

void writePixel(VbeModeInfo *vbeModeInfo, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (!VBE_IsInitialized()) {
        return;
    }

    uint8_t *pixelPointer = ((uint8_t *)(vbeModeInfo->framebuffer + (y * vbeModeInfo->pitch + x * (vbeModeInfo->bitsPerPixel >> 3))));
    // assuming 8x3 (24) (RGB888) bit color
    pixelPointer[(16 - vbeModeInfo->redPosition) >> 3] = r;
    pixelPointer[(16 - vbeModeInfo->greenPosition) >> 3] = g;
    pixelPointer[(16 - vbeModeInfo->bluePosition) >> 3] = b;
}

FONT_Character FONT_GetCharacter(VbeModeInfo *vbeModeInfo, uint16_t x, uint16_t y) {
    FONT_Character output = EMPTY_CHARACTER;

    if (!VBE_IsInitialized()) {
        return output;
    }

    return g_ScreenCharacterBuffer[(y * SCREEN_CHARACTER_WIDTH(vbeModeInfo)) + x];
}

void FONT_ScrollBack(VbeModeInfo *vbeModeInfo, uint16_t lineCount) {
    if (!VBE_IsInitialized()) {
        return;
    }

    // copy lines
    for (uint16_t y = lineCount; y < SCREEN_CHARACTER_HEIGHT(vbeModeInfo); ++y)
        for (uint16_t x = 0; x < SCREEN_CHARACTER_WIDTH(vbeModeInfo); ++x) {
            FONT_PutCharacter(vbeModeInfo, x, y - lineCount, FONT_GetCharacter(vbeModeInfo, x, y));
        }

    // delete last lines
    for (uint16_t y = SCREEN_CHARACTER_HEIGHT(vbeModeInfo) - lineCount; y < SCREEN_CHARACTER_HEIGHT(vbeModeInfo); ++y)
        for (uint16_t x = 0; x < SCREEN_CHARACTER_WIDTH(vbeModeInfo); ++x) {
            FONT_Character tempCharacter = EMPTY_CHARACTER;
            FONT_PutCharacter(vbeModeInfo, x, y, tempCharacter);
        }

    g_cursorPosition[1] -= lineCount;
}

void FONT_PutCharacter(VbeModeInfo *vbeModeInfo, uint16_t x, uint16_t y, FONT_Character character) {
    if (!VBE_IsInitialized()) {
        return;
    }

    for (uint8_t img_x = 0; img_x < 8; ++img_x) {
        for (uint8_t img_y = 0; img_y < 8; ++img_y) {
            if (CONSOLE_FONT_8x8[character.typed.character * 8 + img_y] & (0b10000000 >> img_x))
                writePixel(vbeModeInfo, (x * 8) + img_x, (y * 8) + img_y, character.typed.r, character.typed.g, character.typed.b);
            else
                writePixel(vbeModeInfo, (x * 8) + img_x, (y * 8) + img_y, 0, 0, 0);
        }
    }
}

void FONT_WriteCharacter(VbeModeInfo *vbeModeInfo, FONT_Character character) {
    if (!VBE_IsInitialized()) {
        return;
    }

    FONT_Character spaceCharacter = EMPTY_CHARACTER;
    spaceCharacter.typed.character = ' ';

    switch (character.typed.character) {
    case '\n':
        g_cursorPosition[0] = 0; // '\r'
        ++g_cursorPosition[1];   // '\n'
        break;
    case '\r':
        g_cursorPosition[0] = 0; // '\r'
        break;
    case '\t':
        for (uint8_t i = 0; i < TAB_SIZE - (g_cursorPosition[0] % TAB_SIZE); ++i)
            FONT_PutCharacter(vbeModeInfo, g_cursorPosition[0], g_cursorPosition[1], spaceCharacter);
        break;
    case '\b':
        if (g_cursorPosition[0] == 0) {
            g_cursorPosition[0] = SCREEN_CHARACTER_WIDTH(vbeModeInfo) - 1;
            --g_cursorPosition[1];
            break;
        }
        --g_cursorPosition[0];
        break;
    default:
        FONT_PutCharacter(vbeModeInfo, g_cursorPosition[0], g_cursorPosition[1], character);
        ++g_cursorPosition[0];
        break;
    }

    if (g_cursorPosition[0] >= SCREEN_CHARACTER_WIDTH(vbeModeInfo)) {
        g_cursorPosition[0] = 0;
        ++g_cursorPosition[1];
    }
    if (g_cursorPosition[1] >= SCREEN_CHARACTER_HEIGHT(vbeModeInfo)) {
        FONT_ScrollBack(vbeModeInfo, 1);
    }
}
