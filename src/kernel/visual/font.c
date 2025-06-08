#include "font.h"
#include "disk/fat.h"
#include "fallback_font.h"
#include "graphics.h"
#include "memory/allocator.h"
#include "rasterfont_sizes.h"
#include "stdio.h"
#include "util/memory.h"
#include "util/string.h"
#include "vbe.h"
#include <lib/algorithm/arrays.h>
#include <lib/algorithm/math.h>
#include <lib/errors/errors.h>
#include <lib/memory/memdefs.h>
#include <stdbool.h>
#include <stdint.h>

#define FONT_READ_CHUNK_SIZE 0x1000
#define FONT_MAX_CHARACTERS 0xFF

static FONT_Character *g_ScreenCharacterBuffer = NULL;
static void *g_VideoBuffer = NULL;
static VbeModeInfo *g_VbeModeInfo = NULL;
static uint8_t *g_FontBits = NULL;
static const FONT_FontInfo *g_FontInfo = NULL;
static uint16_t g_FontPixelScale = 1;

void ensureFontInfoSet() {
    if (g_FontInfo != NULL)
        return;

    // no font loaded
    g_FontInfo = &FALLBACK_FONT_INFO;
    g_FontBits = FALLBACK_FONT_8x8;
}

void FONT_DeInitialize() {
    if (g_ScreenCharacterBuffer != NULL) {
        free(g_ScreenCharacterBuffer);
        g_ScreenCharacterBuffer = NULL;
    }

    if (g_FontBits != NULL) {
        free(g_FontBits);
        g_FontBits = NULL;
    }
}

int FONT_Initialize(VbeModeInfo *vbeModeInfo, void *videoBuffer) {
    g_VbeModeInfo = vbeModeInfo;
    g_VideoBuffer = videoBuffer;

    ensureFontInfoSet();

    if ((g_ScreenCharacterBuffer = calloc(FONT_ScreenCharacterWidth() * FONT_ScreenCharacterHeight(), sizeof(FONT_Character))) == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;

    return NO_ERROR;
}

// finds a font based on name, width and height (can be NULL, -1, -1 to ignore).
// returns NULL if none are found or all three inputs are NULL, -1, -1
const FONT_FontInfo *FONT_FindFontInfo(const char *filename, int16_t width, int16_t height) {
    if (filename == NULL && width == -1 && height == -1)
        return NULL;

    for (uint8_t i = 0; i < ARRAY_SIZE(FONT_RasterFontSizes); ++i)
        if ((strncmp(FONT_RasterFontSizes[i].filename, filename, 11) == 0 || filename == NULL) && (FONT_RasterFontSizes[i].width == width || width == -1) && (FONT_RasterFontSizes[i].height == height || height == -1))
            return &FONT_RasterFontSizes[i];

    return NULL;
}

int readFont(FAT_Filesystem *fontsFilesystem) {
    char fontPath[6 + 10 + 1] = {0}; // 6 for "fonts/", 10 for filename, 1 for null terminator = 17
    strcpy(fontPath, "fonts/");
    strcpy(fontPath + strlen("fonts/"), g_FontInfo->filename);
    fontPath[strlen("fonts/") + strlen(g_FontInfo->filename)] = '\0';

    FAT_File *fontFd;
    int status;

    if ((status = FAT_Open(fontsFilesystem, fontPath, &fontFd)) != NO_ERROR)
        return status;

    uint8_t *fontBuffer = g_FontBits;
    uint32_t readCount;
    while ((status = FAT_Read(fontsFilesystem, fontFd, FONT_READ_CHUNK_SIZE, &readCount, fontBuffer)) == NO_ERROR && readCount == FONT_READ_CHUNK_SIZE)
        fontBuffer += readCount;

    if (status != NO_ERROR)
        return status;

    FAT_Close(fontsFilesystem, fontFd); // we'll ignore errors here

    return NO_ERROR;
}

int FONT_SetFont(FAT_Filesystem *fontsFilesystem, const FONT_FontInfo *fontInfo, bool reDraw) {
    if (fontInfo == g_FontInfo)
        return NO_ERROR;

    if (fontsFilesystem == NULL || fontInfo == NULL)
        return NULL_ERROR;

    // backup old fontinfo
    size_t oldScreenCharacterBufferSize = FONT_ScreenCharacterWidth() * FONT_ScreenCharacterHeight() * sizeof(FONT_Character);
    const FONT_FontInfo *oldFontInfo = g_FontInfo;
    uint8_t *oldFontBits = g_FontBits;
    FONT_Character *oldScreenCharacterBuffer = g_ScreenCharacterBuffer;

    g_FontInfo = fontInfo;

    if ((g_FontBits = malloc(DIV_ROUND_UP(g_FontInfo->height * g_FontInfo->width, 8) * FONT_MAX_CHARACTERS)) == NULL) {
        g_FontInfo = oldFontInfo;
        g_FontBits = oldFontBits;
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;
    }

    // possibly restore old fontinfo
    int status;
    if ((status = readFont(fontsFilesystem)) != NO_ERROR) {
        g_FontInfo = oldFontInfo;
        g_FontBits = oldFontBits;
        return status;
    }

    if (oldFontBits != FALLBACK_FONT_8x8)
        free(oldFontBits);

    if (reDraw) {
        GRAPHICS_ClearScreen();
        for (uint16_t y = 0; y < FONT_ScreenCharacterHeight(); ++y)
            for (uint16_t x = 0; x < FONT_ScreenCharacterWidth(); ++x)
                FONT_DrawCharacter(x, y, g_ScreenCharacterBuffer[(y * FONT_ScreenCharacterWidth()) + x]);
    }

    size_t screenCharacterBufferEntries = FONT_ScreenCharacterWidth() * FONT_ScreenCharacterHeight();
    size_t screenCharacterBufferSize = screenCharacterBufferEntries * sizeof(FONT_Character);
    g_ScreenCharacterBuffer = calloc(screenCharacterBufferEntries, sizeof(FONT_Character));
    if (g_ScreenCharacterBuffer == NULL) {
        g_FontInfo = oldFontInfo;
        g_FontBits = oldFontBits;
        g_ScreenCharacterBuffer = oldScreenCharacterBuffer;
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;
    }

    if (oldScreenCharacterBufferSize > screenCharacterBufferSize)
        memcpy(g_ScreenCharacterBuffer, ((void *)oldScreenCharacterBuffer) + (oldScreenCharacterBufferSize - screenCharacterBufferSize), min(oldScreenCharacterBufferSize, screenCharacterBufferSize));
    else
        memcpy(g_ScreenCharacterBuffer, oldScreenCharacterBuffer, min(oldScreenCharacterBufferSize, screenCharacterBufferSize));
    free(oldScreenCharacterBuffer);

    return NO_ERROR;
}

// next 2 functions are because i dont really understand variable sharing across files in c
void FONT_SetPixelScale(uint16_t scale) {
    g_FontPixelScale = scale;
}

uint16_t FONT_GetPixelScale() {
    return g_FontPixelScale;
}

void FONT_SetCharacter(uint16_t x, uint16_t y, FONT_Character character) {
    ensureFontInfoSet();
    g_ScreenCharacterBuffer[(y * FONT_ScreenCharacterWidth()) + x] = character;
}

FONT_Character FONT_GetCharacter(uint16_t x, uint16_t y) {
    ensureFontInfoSet();
    return g_ScreenCharacterBuffer[(y * FONT_ScreenCharacterWidth()) + x];
}

void FONT_ScrollBack(uint16_t lineCount) {
    if (lineCount == 0)
        return;

    size_t offset = g_VbeModeInfo->pitch * g_FontInfo->height * lineCount * g_FontPixelScale;
    size_t memoryToCopy = g_VbeModeInfo->pitch * g_VbeModeInfo->height - offset;

    memcpy(g_VideoBuffer, g_VideoBuffer + offset, memoryToCopy);
    memset(g_VideoBuffer + (FONT_ScreenCharacterHeight() * g_FontInfo->height * g_FontPixelScale * g_VbeModeInfo->pitch) - offset, 0, offset);

    GRAPHICS_PushBuffer();
}

void FONT_DrawCharacter(uint16_t x, uint16_t y, FONT_Character character) {
    ensureFontInfoSet();

    FONT_SetCharacter(x, y, character);

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

    GRAPHICS_PushBufferRectangleScale(x * g_FontInfo->width, y * g_FontInfo->height, g_FontInfo->width, g_FontInfo->height, g_FontPixelScale);
}

// returns the number of characters that can fit on the screen horizontally
uint16_t FONT_ScreenCharacterWidth() {
    ensureFontInfoSet();

    return (g_VbeModeInfo->width / g_FontInfo->width) / g_FontPixelScale;
}

// returns the number of characters that can fit on the screen vertically
uint16_t FONT_ScreenCharacterHeight() {
    ensureFontInfoSet();

    return (g_VbeModeInfo->height / g_FontInfo->height) / g_FontPixelScale;
}
