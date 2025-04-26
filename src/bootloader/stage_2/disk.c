#include "disk.h"
#include "visual/stdio.h"
#include "x86.h"
#include <stdbool.h>
#include <stdint.h>

#define DISK_READ_RETRY_COUNT 3

bool DISK_Initialize(DISK *disk, uint8_t driveNumber) {
    uint16_t infoFlags, bytesPerSector;
    uint32_t cylinders, heads, sectors, totalSectors;

    uint8_t errorCode = x86_Disk_GetDriveParams(driveNumber, &infoFlags, &cylinders, &heads, &sectors, &totalSectors, &bytesPerSector);

    if (errorCode != 0)
        return false;

    disk->id = driveNumber;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->sectors = sectors;
    disk->totalSectors = totalSectors;
    disk->infoFlags = infoFlags;
    disk->bytesPerSector = bytesPerSector;

    return true;
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint16_t count, void *dataOutput) {
    uint8_t readCount, errorCode;

    for (uint8_t i = 0; i < DISK_READ_RETRY_COUNT; ++i) {
        if (x86_Disk_Read(disk->id, lba, count, &readCount, dataOutput) == 0)
            return true;

        x86_Disk_Reset(disk->id);
    }

    return false;
}
