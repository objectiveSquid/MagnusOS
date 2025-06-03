#pragma once

#include <stdbool.h>
#include <stdint.h>

// forward declaration because of fucky wucky circular dependancy
struct ATA_IdentifyData;

typedef struct {
    bool isMaster; // if false its the slave drive
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
    bool supports48BitLba;
    struct ATA_IdentifyData *ataData;
} DISK;

typedef struct {
    bool initializedMasterDisk;
    bool initializedSlaveDisk;
} DISK_InitializeResult;

int DISK_Initialize(DISK_InitializeResult *resultOutput, DISK *masterDisk, DISK *slaveDisk);
void DISK_DeInitialize(DISK *disk);
int DISK_ReadSectors(DISK *disk, uint64_t lba, uint16_t count, uint16_t *readCountOutput, void *dataOutput);
int DISK_WriteSectors(DISK *disk, uint64_t lba, uint16_t count, void *buffer);
