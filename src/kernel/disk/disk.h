#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t id;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
} DISK;

bool DISK_Initialize(uint8_t diskId);
bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint8_t count, void *dataOutput);
