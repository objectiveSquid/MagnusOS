#include "disk.h"
#include "ata.h"
#include <stdbool.h>
#include <stdint.h>

bool DISK_Initialize(uint8_t diskId) {
    // implement
    return true;
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint8_t count, void *dataOutput) {
    return ATA_ReadSectors(lba, dataOutput, count, true);
}