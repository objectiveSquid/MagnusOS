#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t id;
    uint32_t cylinders;
    uint32_t sectors;
    uint32_t heads;
    uint32_t totalSectors;
    uint16_t infoFlags;
    uint16_t bytesPerSector;
} DISK;

bool DISK_Initialize(DISK *disk, uint8_t driveNumber);
uint8_t DISK_ReadSectors(DISK *disk, uint32_t lba, uint16_t count, void *dataOutput);
