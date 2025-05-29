#include "memdetect.h"
#include "util/x86.h"

const char *MEMDETECT_ErrorCodeStrings[] = {
    "BIOS function not supported",
    "Carry flag set",
    "Other error",
};

// maxRegions is assumed to be at least 1
uint8_t MEMDETECT_GetMemoryRegions(MEMDETECT_MemoryRegion *regionsOutput, uint32_t maxRegions, uint32_t *regionCountOutput) {
    uint32_t regionCount = 0;
    uint32_t offset = 0;
    uint8_t errorCode;

    do {
        errorCode = x86_MEMDETECT_GetRegion(regionsOutput + regionCount, &offset);
        if (errorCode != 0) {
            return errorCode;
        }
        regionCount++;
    } while (offset != 0 && regionCount < maxRegions);

    *regionCountOutput = regionCount;
    return 0; // 0 means no error
}
