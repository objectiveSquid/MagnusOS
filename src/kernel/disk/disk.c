#include "disk.h"
#include "ata.h"
#include "memory/allocator.h"
#include "visual/stdio.h"
#include <lib/errors/errors.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// !!! YOU ARE RESPONSIBLE FOR FREEING `masterDisk->ataData` AND `slaveDisk->ataData` with `DISK_DeInitialize` !!!
int DISK_Initialize(DISK_InitializeResult *resultOutput, DISK *masterDisk, DISK *slaveDisk) {
    ATA_InitializeDriveOutput masterOutput, slaveOutput;

    masterOutput.driveData = (ATA_IdentifyData *)malloc(sizeof(ATA_IdentifyData));
    slaveOutput.driveData = (ATA_IdentifyData *)malloc(sizeof(ATA_IdentifyData));

    if (masterOutput.driveData == NULL)
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;
    if (slaveOutput.driveData == NULL) {
        free(masterOutput.driveData); // for once, we will do it for the caller
        return FAILED_TO_ALLOCATE_MEMORY_ERROR;
    }

    ATA_Initialize(&masterOutput, &slaveOutput);

    resultOutput->initializedMasterDisk = masterOutput.driveExists;
    resultOutput->initializedSlaveDisk = slaveOutput.driveExists;

    // spaghetti, maybe de duplicate code?
    if (masterOutput.driveExists && masterOutput.driveData->Capabilities.LbaSupported) {
        masterDisk->isMaster = true;
        masterDisk->cylinders = masterOutput.driveData->NumberOfCurrentCylinders;
        masterDisk->sectors = masterOutput.driveData->CurrentSectorsPerTrack;
        masterDisk->heads = masterOutput.driveData->NumberOfCurrentHeads;
        masterDisk->supports48BitLba = masterOutput.driveData->CommandSetSupport.BigLba && masterOutput.driveData->CommandSetActive.BigLba;
        masterDisk->ataData = (struct ATA_IdentifyData *)masterOutput.driveData;
    }

    if (slaveOutput.driveExists && slaveOutput.driveData->Capabilities.LbaSupported) {
        slaveDisk->isMaster = false;
        slaveDisk->cylinders = slaveOutput.driveData->NumberOfCurrentCylinders;
        slaveDisk->sectors = slaveOutput.driveData->CurrentSectorsPerTrack;
        slaveDisk->heads = slaveOutput.driveData->NumberOfCurrentHeads;
        slaveDisk->supports48BitLba = slaveOutput.driveData->CommandSetSupport.BigLba && slaveOutput.driveData->CommandSetActive.BigLba;
        slaveDisk->ataData = (struct ATA_IdentifyData *)slaveOutput.driveData;
    }

    return NO_ERROR;
}

void DISK_DeInitialize(DISK *disk) {
    if (disk->ataData == NULL)
        return;

    free(disk->ataData);
    disk->ataData = NULL;
}

int DISK_ReadSectors(DISK *disk, uint64_t lba, uint16_t count, uint16_t *readCountOutput, void *dataOutput) {
    return ATA_ReadSectors(lba, dataOutput, count, disk);
}

int DISK_WriteSectors(DISK *disk, uint64_t lba, uint16_t count, void *buffer) {
    return ATA_WriteSectors(lba, buffer, count, disk);
}
