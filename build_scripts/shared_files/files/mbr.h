#pragma once

#include "disk.h"
#include <stdint.h>

typedef struct {
    DISK *disk;
    uint32_t partitionLBA;
    uint32_t partitionSize;
} Partition;

void MBR_InitializePartition(Partition *partitionOut, DISK *disk, uint32_t partitionLBA, uint32_t partitionSize);

int Partition_ReadSectors(Partition *partition, uint64_t lba, uint16_t sectors, uint16_t *readCountOutput, void *dataOutput);
