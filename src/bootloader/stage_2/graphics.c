#include "graphics.h"
#include "memdefs.h"
#include "memory.h"
#include "vbe.h"

static VbeModeInfo *g_VbeModeInfo = (VbeModeInfo *)MEMORY_VESA_MODE_INFO;

void GRAPHICS_WritePixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (!VBE_VerifyInitialized()) {
        return;
    }

    uint8_t *pixelPointer = ((uint8_t *)(g_VbeModeInfo->framebuffer + (y * g_VbeModeInfo->pitch + x * (g_VbeModeInfo->bitsPerPixel >> 3))));
    // assuming 8x3 (24) (RGB888) bit color
    pixelPointer[(16 - g_VbeModeInfo->redPosition) >> 3] = r;
    pixelPointer[(16 - g_VbeModeInfo->greenPosition) >> 3] = g;
    pixelPointer[(16 - g_VbeModeInfo->bluePosition) >> 3] = b;
}

void GRAPHICS_ClearScreen() {
    if (!VBE_VerifyInitialized()) {
        return;
    }

    memset((void *)g_VbeModeInfo->framebuffer, 0, g_VbeModeInfo->pitch * g_VbeModeInfo->height);
}
