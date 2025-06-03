#include "memdetect.h"
#include "util/errors.h"
#include "util/x86.h"
#include <stddef.h>

const char *MEMDETECT_ErrorCodeStrings[] = {
    "BIOS function not supported",
    "Carry flag set",
    "Other error",
};

// maxRegions is assumed to be at least 1
int MEMDETECT_GetMemoryRegions(MEMDETECT_MemoryRegion *regionsOutput, uint32_t maxRegions, uint32_t *regionCountOutput, uint8_t *errorCodeOutput) {
    uint32_t offset = 0;
    uint8_t errorCode;

    do {
        errorCode = x86_MEMDETECT_GetRegion(regionsOutput + (*regionCountOutput), &offset);
        if (errorCode != 0) { // 0 means no error
            if (errorCodeOutput != NULL)
                *errorCodeOutput = errorCode;
            return FAILED_TO_DETECT_MEMORY_ERROR;
        }
        (*regionCountOutput)++;
    } while (offset != 0 && (*regionCountOutput) < maxRegions);

    return NO_ERROR; // 0 means no error
}
