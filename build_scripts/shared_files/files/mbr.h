#pragma once

#include "disk.h"
#include <stdint.h>

typedef struct {
    DISK *disk;
    uint32_t partitionLBA;
    uint32_t partitionSize;
} Partition;

void MBR_DetectPartition(Partition *partitionOut, DISK *disk, uint32_t partitionLBA, uint32_t partitionSize);

uint16_t Partition_ReadSectors(Partition *partition, uint64_t lba, uint16_t sectors, void *dataOutput);
