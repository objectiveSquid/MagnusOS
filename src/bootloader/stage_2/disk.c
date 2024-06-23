#include "disk.h"
#include "stdbool.h"
#include "stdint.h"
#include "x86.h"

#define DISK_READ_RETRY_COUNT 3

bool DISK_Initialize(DISK *disk, uint8_t driveNumber) {
    uint8_t driveType;
    uint16_t cylinders, heads, sectors;

    if (!x86_Disk_GetDriveParams(disk->id, &driveType, &cylinders, &heads, &sectors))
        return false;

    disk->id = driveNumber;
    disk->cylinders = cylinders + 1;
    disk->heads = heads + 1;
    disk->sectors = sectors;

    return true;
}

void DISK_LbaToChs(DISK *disk, uint32_t lba, uint16_t *cylinderOutput, uint16_t *headOutput, uint16_t *sectorOutput) {
    // cylinder = (lba / sectors per track) / heads
    *cylinderOutput = (lba / disk->sectors) / disk->heads;

    // head = (lba / sectors per track) % heads
    *headOutput = (lba / disk->sectors) % disk->heads;

    // sector = (lba % sectors per track) + 1
    *sectorOutput = lba % disk->sectors + 1;
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint8_t count, void __far *dataOutput) {
    uint16_t cylinder, head, sector;

    DISK_LbaToChs(disk, lba, &cylinder, &head, &sector);

    for (uint8_t i = 0; i < DISK_READ_RETRY_COUNT; ++i) {
        if (x86_Disk_Read(disk->id, cylinder, head, sector, count, dataOutput))
            return true;

        x86_Disk_Reset(disk->id);
    }

    return false;
}
