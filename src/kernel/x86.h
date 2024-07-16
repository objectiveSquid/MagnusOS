#pragma once
#include <stdbool.h>
#include <stdint.h>

void __attribute__((cdecl)) x86_OutByte(uint16_t port, uint8_t value);
uint8_t __attribute__((cdecl)) x86_InByte(uint16_t port);

bool __attribute__((cdecl)) x86_Disk_Reset(uint8_t drive);
bool __attribute__((cdecl)) x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint8_t count, void *dataOutput);
bool __attribute__((cdecl)) x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveTypeOutput, uint16_t *cylindersOutput, uint16_t *headsOutput, uint16_t *sectorsOutput);
