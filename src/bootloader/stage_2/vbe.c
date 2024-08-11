#include "vbe.h"
#include "memory.h"
#include "x86.h"

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
