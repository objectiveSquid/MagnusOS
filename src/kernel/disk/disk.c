#include "disk.h"
#include "fdc.h"
#include "util/x86.h"
#include <stdbool.h>
#include <stdint.h>

bool DISK_Initialize(uint8_t diskId) {
    return FDC_Initialize(diskId);
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint8_t count, void *dataOutput) {
    return FDC_ReadSectors(disk, lba, count, dataOutput);
}
