#pragma once

#include "memory/memdetect.h"
#include <stdbool.h>
#include <stdint.h>

#define ASMCALL __attribute__((__cdecl__))

void ASMCALL x86_OutByte(uint16_t port, uint8_t value);
uint8_t ASMCALL x86_InByte(uint16_t port);

bool ASMCALL x86_Disk_Reset(uint8_t drive);
uint8_t ASMCALL x86_Disk_Read(uint8_t drive, uint32_t lba, uint16_t count, uint8_t *readCountOutput, void *dataOutput);
uint8_t ASMCALL x86_Disk_GetDriveParams(uint8_t drive, uint16_t *infoFlagsOutput, uint32_t *cylindersOutput, uint32_t *headsOutput, uint32_t *sectorsPerTrackOutput, uint32_t *totalSectorsOutput, uint16_t *bytesPerSectorOutput);
/*
FROM RALPH BROWN LIST:

Bitfields for IBM/MS INT 13 Extensions information flags:

Bit(s)  Description     (Table 00274)
0      DMA boundary errors handled transparently
1      cylinder/head/sectors-per-track information is valid
2      removable drive
3      write with verify supported
4      drive has change-line support (required if drive >= 80h is removable)
5      drive can be locked (required if drive >= 80h is removable)
6      CHS information set to maximum supported values, not current media
15-7   reserved (0)
*/

uint8_t ASMCALL x86_VBE_GetControllerInfo(void *infoOutput);
uint8_t ASMCALL x86_VBE_GetModeInfo(uint16_t mode, void *infoOutput);
uint8_t ASMCALL x86_VBE_SetVideoMode(uint16_t mode);

// return error codes are described in MEMDETECT_GetMemoryRegionsErrorCode
uint8_t ASMCALL x86_MEMDETECT_GetRegion(MEMDETECT_MemoryRegion *regionOutput, uint32_t *offset);
uint16_t ASMCALL x86_MEMDETECT_GetContiguousKBAfter1MB();

void ASMCALL x86_Halt();
