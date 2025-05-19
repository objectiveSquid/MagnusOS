#include "disk.h"
#include "ata.h"
#include "visual/stdio.h"
#include <stdbool.h>
#include <stdint.h>

void DISK_Initialize(DISK_InitializeResult *resultOutput, DISK *masterDisk, DISK *slaveDisk) {
    ATA_InitializeOutput ataOutput;
    ATA_Initialize(&ataOutput);

    resultOutput->initializedMasterDisk = ataOutput.masterDriveExists;
    resultOutput->initializedSlaveDisk = ataOutput.slaveDriveExists;

    if (ataOutput.masterDriveExists) {
        masterDisk->isMaster = true;
        masterDisk->cylinders = ataOutput.masterDriveData->NumberOfCurrentCylinders;
        masterDisk->sectors = ataOutput.masterDriveData->CurrentSectorsPerTrack;
        masterDisk->heads = ataOutput.masterDriveData->NumberOfCurrentHeads;
        masterDisk->ataData = ataOutput.masterDriveData;
        printf("master drive cylinders: %d\n", masterDisk->cylinders);
        printf("master drive sectors: %d\n", masterDisk->sectors);
        printf("master drive heads: %d\n", masterDisk->heads);
    }

    if (ataOutput.slaveDriveExists) {
        slaveDisk->isMaster = false;
        slaveDisk->cylinders = ataOutput.slaveDriveData->NumberOfCurrentCylinders;
        slaveDisk->sectors = ataOutput.slaveDriveData->CurrentSectorsPerTrack;
        slaveDisk->heads = ataOutput.slaveDriveData->NumberOfCurrentHeads;
        slaveDisk->ataData = ataOutput.slaveDriveData;
    }
}

bool DISK_ReadSectors(DISK *disk, uint32_t lba, uint8_t count, void *dataOutput) {
    return ATA_ReadSectors(lba, dataOutput, count, disk->isMaster);
}