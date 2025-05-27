#pragma once

#include "mbr.h"
#include <stdbool.h>
#include <stdint.h>

void MBR_DetectPartition(Partition *partitionOut, DISK *disk, uint32_t partitionLBA, uint32_t partitionSize) {
    partitionOut->disk = disk;
    partitionOut->partitionLBA = partitionLBA;
    partitionOut->partitionSize = partitionSize;
}

uint16_t Partition_ReadSectors(Partition *partition, uint64_t lba, uint16_t sectors, void *dataOutput) {
    return DISK_ReadSectors(partition->disk, partition->partitionLBA + lba, sectors, dataOutput);
}
