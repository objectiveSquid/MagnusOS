#include "disk.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include <lib/errors/errors.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DISK_READ_RETRY_COUNT 3

int DISK_Initialize(DISK *disk, uint8_t driveNumber) {
    uint16_t infoFlags, bytesPerSector;
    uint32_t cylinders, heads, sectors, totalSectors;

    uint8_t errorCode = x86_Disk_GetDriveParams(driveNumber, &infoFlags, &cylinders, &heads, &sectors, &totalSectors, &bytesPerSector);

    if (errorCode != 0)
        return DISK_ERROR;

    disk->id = driveNumber;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->sectors = sectors;
    disk->totalSectors = totalSectors;
    disk->infoFlags = infoFlags;
    disk->bytesPerSector = bytesPerSector;

    return NO_ERROR;
}

int DISK_ReadSectors(DISK *disk, uint32_t lba, uint16_t count, uint16_t *readCountOutput, void *dataOutput) {
    uint8_t errorCode;
    uint16_t unused;
    if (readCountOutput == NULL)
        readCountOutput = &unused;

    for (uint8_t i = 0; i < DISK_READ_RETRY_COUNT; ++i) {
        if ((errorCode = x86_Disk_Read(disk->id, lba, count, readCountOutput, dataOutput)) == 0) {
            if (*readCountOutput != count)
                return DISK_NOT_ENOUGH_SECTORS_WARNING;
            return NO_ERROR;
        }

        x86_Disk_Reset(disk->id);
    }

    if (errorCode != 0)
        return DISK_READ_ERROR;

    return DISK_READ_RETRIES_EXHAUSTED_ERROR;
}
