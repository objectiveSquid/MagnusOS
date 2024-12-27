#include "vbe.h"
#include "memdefs.h"
#include "memory.h"
#include "stdio.h"
#include "x86.h"

#define DESIRED_WIDTH 1920
#define DESIRED_HEIGHT 1080
#define DESIRED_BITS_PER_PIXEL 24

bool VBE_GetControllerInfo(VbeInfoBlock *infoOutput) {
    if (x86_VBE_GetControllerInfo(infoOutput) == 0x00) {
        // convert videoModePtr to linear address
        infoOutput->videoModePtr = MEMORY_SEGMENT_OFFSET_TO_LINEAR(infoOutput->videoModePtr);
        return true;
    }

    return false;
}

bool VBE_GetModeInfo(uint16_t mode, VbeModeInfo *infoOutput) {
    return x86_VBE_GetModeInfo(mode, infoOutput) == 0x00;
}

bool VBE_SetVideoMode(uint16_t mode) {
    return x86_VBE_SetVideoMode(mode) == 0x00;
}

bool VBE_Initialize() {
    uint16_t pickedMode = UINT16_MAX;

    VbeInfoBlock *vbeInfo = (VbeInfoBlock *)MEMORY_VESA_INFO;
    VbeModeInfo *vbeModeInfo = (VbeModeInfo *)MEMORY_VESA_MODE_INFO;
    if (VBE_GetControllerInfo(vbeInfo)) {
        puts("Got VBE controller info!\n");

        uint16_t *modes = (uint16_t *)(vbeInfo->videoModePtr);
        for (uint16_t i = 0; modes[i] != 0xFFFF; ++i) {
            if (!VBE_GetModeInfo(modes[i], vbeModeInfo)) {
                printf("Failed to get mode info for VBE extension: %hx\n", modes[i]);
                continue;
            }

            if ((vbeModeInfo->attributes & 0x90) == 0x90 &&
                DESIRED_WIDTH == vbeModeInfo->width &&
                DESIRED_HEIGHT == vbeModeInfo->height &&
                DESIRED_BITS_PER_PIXEL == vbeModeInfo->bitsPerPixel) {
                pickedMode = modes[i];
                break;
            }
        }
    } else {
        puts("Failed to get VBE controller info!\n");
        return false;
    }

    if (pickedMode == UINT16_MAX || !VBE_SetVideoMode(pickedMode)) {
        puts("No suitable VBE mode found!\n");
        return false;
    }

    return true;
}