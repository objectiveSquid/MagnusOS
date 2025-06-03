#include "mbr.h"
#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

void MBR_InitializePartition(Partition *partitionOut, DISK *disk, uint32_t partitionLBA, uint32_t partitionSize) {
    partitionOut->disk = disk;
    partitionOut->partitionLBA = partitionLBA;
    partitionOut->partitionSize = partitionSize;
}

int Partition_ReadSectors(Partition *partition, uint64_t lba, uint16_t sectors, uint16_t *readCountOutput, void *dataOutput) {
    return DISK_ReadSectors(partition->disk, partition->partitionLBA + lba, sectors, readCountOutput, dataOutput);
}
