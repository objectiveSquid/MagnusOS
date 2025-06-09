#pragma once

#include <lib/memory/memdetect.h>
#include <lib/x86/misc.h> // for asmcall
#include <stdbool.h>
#include <stdint.h>

uint8_t ASMCALL x86_VBE_GetControllerInfo(void *infoOutput);
uint8_t ASMCALL x86_VBE_GetModeInfo(uint16_t mode, void *infoOutput);
uint8_t ASMCALL x86_VBE_SetVideoMode(uint16_t mode);

// return error codes are described in MEMDETECT_GetMemoryRegionsErrorCode
uint8_t ASMCALL x86_MEMDETECT_GetRegion(MEMDETECT_MemoryRegion *regionOutput, uint32_t *offset);
uint16_t ASMCALL x86_MEMDETECT_GetContiguousKBAfter1MB();
