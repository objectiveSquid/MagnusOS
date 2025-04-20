#include "font.h"
#include "disk/fat.h"
#include "fallback_font.h"
#include "graphics.h"
#include "memdefs.h"
#include "rasterfont_sizes.h"
#include "stdio.h"
#include "util/memory.h"
#include "util/other.h"
#include "util/string.h"
#include "vbe.h"
#include <stdbool.h>
#include <stdint.h>

#define FONT_READ_CHUNK_SIZE 0x1000

static FONT_Character *g_ScreenCharacterBuffer = (FONT_Character *)MEMORY_SCREEN_CHARACTER_BUFFER;
static VbeModeInfo *g_VbeModeInfo = (VbeModeInfo *)MEMORY_VESA_MODE_INFO;
static uint8_t *g_FontBits = (uint8_t *)MEMORY_RASTERFONT_BITS;
static const FONT_FontInfo *g_FontInfo = NULL;
static uint8_t g_FontPixelScale = 1;

// finds a font based on name, width and height (can be NULL, -1, -1 to ignore). returns NULL if none are found or all three inputs are NULL, -1, -1
const FONT_FontInfo *FONT_FindFontInfo(const char *filename, int16_t width, int16_t height) {
    if (filename == NULL && width == -1 && height == -1)
        return NULL;

    for (uint8_t i = 0; i < ARRAY_SIZE(FONT_RasterFontSizes); ++i)
        if ((strncmp(FONT_RasterFontSizes[i].filename, filename, 11) == 0 || filename == NULL) && (FONT_RasterFontSizes[i].width == width || width == -1) && (FONT_RasterFontSizes[i].height == height || height == -1))
            return &FONT_RasterFontSizes[i];

    return NULL;
}

void readFont() {
    char fontPath[6 + 10 + 1] = {0}; // 6 for "fonts/", 10 for filename, 1 for null terminator = 17
    strcpy(fontPath, "fonts/");
    strcpy(fontPath + strlen("fonts/"), g_FontInfo->filename);
    fontPath[strlen("fonts/") + strlen(g_FontInfo->filename)] = '\0';

    DISK *disk = (DISK *)MEMORY_DISK_INFO_BUFFER;
    FAT_File *fontFd = FAT_Open(disk, fontPath);
    if (fontFd == NULL) {
        printf("Failed to open font file: %s\n", fontPath);
        return;
    }

    uint8_t *fontBuffer = g_FontBits;
    uint32_t readCount;
    while (readCount = FAT_Read(disk, fontFd, FONT_READ_CHUNK_SIZE, fontBuffer))
        fontBuffer += readCount;

    FAT_Close(fontFd);
}

void FONT_SetFont(const FONT_FontInfo *fontInfo, bool reDraw) {
    if (fontInfo == g_FontInfo)
        return;

    g_FontInfo = fontInfo;
    readFont();

    if (!reDraw)
        return;

    GRAPHICS_ClearScreen();
    for (uint16_t y = 0; y < FONT_ScreenCharacterHeight(); ++y)
        for (uint16_t x = 0; x < FONT_ScreenCharacterWidth(); ++x)
            FONT_PutCharacter(x, y, g_ScreenCharacterBuffer[(y * FONT_ScreenCharacterWidth()) + x]);
}

void ensureFontInfoSet() {
    if (g_FontInfo != NULL)
        return;

    g_FontInfo = &FALLBACK_FONT_INFO;
    g_FontBits = FALLBACK_FONT_8x8;
}

// next 2 functions are because i dont really understand variable sharing across files in c
void FONT_SetPixelScale(uint8_t scale) {
    g_FontPixelScale = scale;
}

uint8_t FONT_GetPixelScale() {
    return g_FontPixelScale;
}

FONT_Character FONT_GetCharacter(uint16_t x, uint16_t y) {
    FONT_Character output = EMPTY_CHARACTER;

    return g_ScreenCharacterBuffer[(y * FONT_ScreenCharacterWidth()) + x];
}

void FONT_ScrollBack(uint16_t lineCount) {
    // copy lines
    for (uint16_t y = lineCount; y < FONT_ScreenCharacterHeight(); ++y)
        for (uint16_t x = 0; x < FONT_ScreenCharacterWidth(); ++x)
            FONT_PutCharacter(x, y - lineCount, FONT_GetCharacter(x, y));

    // delete last lines
    for (uint16_t y = FONT_ScreenCharacterHeight() - lineCount; y < FONT_ScreenCharacterHeight(); ++y)
        for (uint16_t x = 0; x < FONT_ScreenCharacterWidth(); ++x) {
            FONT_Character tempCharacter = EMPTY_CHARACTER;
            FONT_PutCharacter(x, y, tempCharacter);
        }
}

void FONT_PutCharacter(uint16_t x, uint16_t y, FONT_Character character) {
    ensureFontInfoSet();

    uint32_t characterBitIndex = character.typed.character * g_FontInfo->width * g_FontInfo->height;
    for (uint8_t img_x = 0; img_x < g_FontInfo->width; ++img_x) {
        for (uint8_t img_y = 0; img_y < g_FontInfo->height; ++img_y) {
            uint32_t relativeBitIndex = img_y * g_FontInfo->width + img_x;
            uint32_t absoluteBitIndex = characterBitIndex + relativeBitIndex;
            uint32_t absoluteByteIndex = absoluteBitIndex / 8;
            uint32_t byteBitIndex = 7 - absoluteBitIndex % 8;

            if (g_FontBits[absoluteByteIndex] & (1 << byteBitIndex))
                GRAPHICS_WriteScalePixel((x * g_FontInfo->width) + img_x, (y * g_FontInfo->height) + img_y, character.typed.r, character.typed.g, character.typed.b, g_FontPixelScale);
            else
                GRAPHICS_WriteScalePixel((x * g_FontInfo->width) + img_x, (y * g_FontInfo->height) + img_y, 0, 0, 0, g_FontPixelScale);
        }
    }
}

uint16_t FONT_ScreenCharacterWidth() {
    ensureFontInfoSet();

    return (g_VbeModeInfo->width / g_FontInfo->width) / g_FontPixelScale;
}

uint16_t FONT_ScreenCharacterHeight() {
    ensureFontInfoSet();

    return (g_VbeModeInfo->height / g_FontInfo->height) / g_FontPixelScale;
}
