#pragma once
#include "vbe.h"
#include <stdbool.h>
#include <stdint.h>

#define ASMCALL __attribute__((__cdecl__))

void ASMCALL x86_OutByte(uint16_t port, uint8_t value);
uint8_t ASMCALL x86_InByte(uint16_t port);

bool ASMCALL x86_Disk_Reset(uint8_t drive);
bool ASMCALL x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint8_t count, void *dataOutput);
bool ASMCALL x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveTypeOutput, uint16_t *cylindersOutput, uint16_t *headsOutput, uint16_t *sectorsOutput);

uint8_t ASMCALL x86_VBE_GetControllerInfo(void *infoOutput);
uint8_t ASMCALL x86_VBE_GetModeInfo(uint16_t mode, void *infoOutput);
uint8_t ASMCALL x86_VBE_SetVideoMode(uint16_t mode);

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} E820MemoryBlock;

enum E820MemoryBlockType {
    E820_USABLE = 1,
    E820_RESERVED = 2,
    E820_ACPI_RECLAIMABLE = 3,
    E820_ACPI_NVS = 4,
    E820_BAD_MEMORY = 5,
};

int ASMCALL x86_E820GetNextBlock(E820MemoryBlock *block, uint32_t *continuationId);