#include "vbe.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include <lib/errors/errors.h>
#include <lib/memory/allocator.h>
#include <lib/memory/memdefs.h>
#include <lib/memory/memory.h>

#define DESIRED_WIDTH 1920
#define DESIRED_HEIGHT 1080
#define DESIRED_BITS_PER_PIXEL 24 // RGB, 8 bits per color channel

int VBE_GetControllerInfo(VbeInfoBlock *infoOutput, uint8_t *errorCodeOutput) {
    uint8_t errorCode = x86_VBE_GetControllerInfo(infoOutput);

    if (errorCodeOutput != NULL)
        *errorCodeOutput = errorCode;

    if (errorCode != 0)
        return VBE_FAILED_TO_GET_CONTROLLER_INFO_ERROR;

    infoOutput->videoModePtr = MEMORY_SEGMENT_OFFSET_TO_LINEAR(infoOutput->videoModePtr);

    return NO_ERROR;
}

int VBE_GetModeInfo(uint16_t mode, VbeModeInfo *infoOutput, uint8_t *errorCodeOutput) {
    uint8_t errorCode = x86_VBE_GetModeInfo(mode, infoOutput);

    if (errorCodeOutput != NULL)
        *errorCodeOutput = errorCode;

    if (errorCode != 0)
        return VBE_FAILED_TO_GET_MODE_INFO_ERROR;

    return NO_ERROR;
}

int VBE_SetVideoMode(uint16_t mode, uint8_t *errorCodeOutput) {
    uint8_t errorCode = x86_VBE_SetVideoMode(mode);

    if (errorCodeOutput != NULL)
        *errorCodeOutput = errorCode;

    if (errorCode != 0)
        return VBE_FAILED_TO_SET_MODE_ERROR;

    return NO_ERROR;
}

int VBE_Initialize(VbeModeInfo *modeInfo) {
    uint16_t pickedMode = UINT16_MAX;
    int status;

    VbeInfoBlock *controllerInfo = (VbeInfoBlock *)ALLOCATOR_Malloc(sizeof(VbeInfoBlock), true);
    if ((status = VBE_GetControllerInfo(controllerInfo, NULL)) == NO_ERROR) {

        uint16_t *modes = (uint16_t *)(controllerInfo->videoModePtr);
        for (uint16_t i = 0; modes[i] != 0xFFFF; ++i) {
            if ((status = VBE_GetModeInfo(modes[i], modeInfo, NULL)) != NO_ERROR)
                continue;

            if ((modeInfo->attributes & 0x90) == 0x90 &&
                DESIRED_WIDTH == modeInfo->width &&
                DESIRED_HEIGHT == modeInfo->height &&
                DESIRED_BITS_PER_PIXEL == modeInfo->bitsPerPixel) {
                pickedMode = modes[i];
                break;
            }
        }
    } else {
        goto free_controller_info;
    }

    if (pickedMode == UINT16_MAX) {
        status = VBE_FAILED_TO_FIND_SUITABLE_MODE_ERROR;
        goto free_controller_info;
    }

    if ((status = VBE_SetVideoMode(pickedMode, NULL)) != NO_ERROR)
        puts("Failed to set VBE mode!\n"); // no need to `goto`/jump, no logic to skip

free_controller_info:
    free(controllerInfo);

    return status;
}
