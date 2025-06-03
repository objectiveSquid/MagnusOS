#include "graphics.h"
#include "memdefs.h"
#include "util/memory.h"
#include "vbe.h"
#include <stddef.h>

static VbeModeInfo *g_VbeModeInfo = NULL;

void GRAPHICS_WriteScalePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t scale) {
    x *= scale;
    y *= scale;
    for (uint8_t relx = 0; relx < scale; ++relx)
        for (uint8_t rely = 0; rely < scale; ++rely)
            GRAPHICS_WritePixel(x + relx, y + rely, r, g, b);
}

void GRAPHICS_WritePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t *pixelPointer = ((uint8_t *)(g_VbeModeInfo->framebuffer + (y * g_VbeModeInfo->pitch + x * (g_VbeModeInfo->bitsPerPixel >> 3))));
    // assuming 8x3 (24) (RGB888) bit color
    pixelPointer[g_VbeModeInfo->redPosition >> 3] = r;
    pixelPointer[g_VbeModeInfo->greenPosition >> 3] = g;
    pixelPointer[g_VbeModeInfo->bluePosition >> 3] = b;
}

void GRAPHICS_ClearScreen() {
    memset((void *)g_VbeModeInfo->framebuffer, 0, g_VbeModeInfo->pitch * g_VbeModeInfo->height);
}

void GRAPHICS_Initialize(VbeModeInfo *vbeModeInfo) {
    g_VbeModeInfo = vbeModeInfo;
    GRAPHICS_ClearScreen();
}
