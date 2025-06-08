#include "graphics.h"
#include "memory/allocator.h"
#include "util/memory.h"
#include "vbe.h"
#include <lib/errors/errors.h>
#include <lib/memory/memdefs.h>
#include <stddef.h>

static VbeModeInfo *g_VbeModeInfo = NULL;
static void *g_VideoBuffer = NULL;

void GRAPHICS_WriteScalePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint16_t scale) {
    x *= scale;
    y *= scale;
    for (uint16_t relx = 0; relx < scale; ++relx)
        for (uint16_t rely = 0; rely < scale; ++rely)
            GRAPHICS_WritePixel(x + relx, y + rely, r, g, b);
}

void GRAPHICS_WritePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t *pixelPointer = (uint8_t *)(g_VideoBuffer + (y * g_VbeModeInfo->pitch + x * (g_VbeModeInfo->bitsPerPixel / 8)));
    // assuming 8x3 (24bit) (RGB888) color
    pixelPointer[g_VbeModeInfo->redPosition / 8] = r;
    pixelPointer[g_VbeModeInfo->greenPosition / 8] = g;
    pixelPointer[g_VbeModeInfo->bluePosition / 8] = b;
}

void GRAPHICS_PushBufferRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    uint8_t *videoBufferStartLine, *frameBufferStartLine;
    uint8_t bytesPerPixel = g_VbeModeInfo->bitsPerPixel / 8;

    // copy 1 line at a time
    for (uint16_t currentY = y; currentY < y + height; ++currentY) {
        videoBufferStartLine = (uint8_t *)(g_VideoBuffer + (currentY * g_VbeModeInfo->pitch + (x * bytesPerPixel)));
        frameBufferStartLine = (uint8_t *)(g_VbeModeInfo->framebuffer + (currentY * g_VbeModeInfo->pitch + (x * bytesPerPixel)));
        memcpy(frameBufferStartLine, videoBufferStartLine, width * bytesPerPixel);
    }
}

void GRAPHICS_PushBufferRectangleScale(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t scale) {
    GRAPHICS_PushBufferRectangle(x * scale, y * scale, width * scale, height * scale);
}

void GRAPHICS_PushBuffer() {
    memcpy((void *)g_VbeModeInfo->framebuffer, (void *)g_VideoBuffer, g_VbeModeInfo->pitch * g_VbeModeInfo->height);
}

void GRAPHICS_ClearScreen() {
    memset((void *)g_VideoBuffer, 0, g_VbeModeInfo->pitch * g_VbeModeInfo->height);
}

// !!! YOU ARE RESPONSIBLE FOR FREEING `videoBufferOutput` WITH `GRAPHICS_DeInitialize` !!!
int GRAPHICS_Initialize(VbeModeInfo *vbeModeInfo, void **videoBufferOutput) {
    if (vbeModeInfo == NULL)
        return NULL_ERROR;

    g_VbeModeInfo = vbeModeInfo;

    g_VideoBuffer = malloc(g_VbeModeInfo->pitch * g_VbeModeInfo->height);
    if (g_VideoBuffer == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;
    *videoBufferOutput = g_VideoBuffer;

    GRAPHICS_ClearScreen();
    GRAPHICS_PushBuffer();

    return NO_ERROR;
}

void GRAPHICS_DeInitialize() {
    free(g_VideoBuffer);
}
