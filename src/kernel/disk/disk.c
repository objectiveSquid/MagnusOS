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

    if (ataOutput.masterDriveExists && ataOutput.masterDriveData->Capabilities.LbaSupported) {
        masterDisk->isMaster = true;
        masterDisk->cylinders = ataOutput.masterDriveData->NumberOfCurrentCylinders;
        masterDisk->sectors = ataOutput.masterDriveData->CurrentSectorsPerTrack;
        masterDisk->heads = ataOutput.masterDriveData->NumberOfCurrentHeads;
        masterDisk->supports48BitLba = ataOutput.masterDriveData->CommandSetSupport.BigLba && ataOutput.masterDriveData->CommandSetActive.BigLba;
        masterDisk->ataData = (struct ATA_IdentifyData *)ataOutput.masterDriveData;
    }

    if (ataOutput.slaveDriveExists && ataOutput.slaveDriveData->Capabilities.LbaSupported) {
        slaveDisk->isMaster = false;
        slaveDisk->cylinders = ataOutput.slaveDriveData->NumberOfCurrentCylinders;
        slaveDisk->sectors = ataOutput.slaveDriveData->CurrentSectorsPerTrack;
        slaveDisk->heads = ataOutput.slaveDriveData->NumberOfCurrentHeads;
        slaveDisk->supports48BitLba = ataOutput.slaveDriveData->CommandSetSupport.BigLba && ataOutput.slaveDriveData->CommandSetActive.BigLba;
        slaveDisk->ataData = (struct ATA_IdentifyData *)ataOutput.slaveDriveData;
    }
}

bool DISK_ReadSectors(DISK *disk, uint64_t lba, uint16_t count, void *dataOutput) {
    return ATA_ReadSectors(lba, dataOutput, count, disk);
}