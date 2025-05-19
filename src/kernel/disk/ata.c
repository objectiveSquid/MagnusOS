#include "ata.h"
#include "disk.h"
#include "memdefs.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include <stdbool.h>

#define ATA_PORT_DATA 0x1F0
#define ATA_PORT_ERROR 0x1F1
#define ATA_PORT_SECTOR_COUNT 0x1F2
#define ATA_PORT_LBA_LOW 0x1F3
#define ATA_PORT_LBA_MID 0x1F4
#define ATA_PORT_LBA_HIGH 0x1F5
#define ATA_PORT_DRIVE_SELECT 0x1F6
#define ATA_PORT_STATUS_COMMAND 0x1F7

#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_IDENTIFY 0xEC

// these are the values if doing stuff with the master drive, for most commands the value to use for the slave drive is simply 1 higher than these values
#define ATA_SELECT_IDENTIFY 0xA0
#define ATA_SELECT_READ 0xE0

#define ATA_STATUS_REGISTER_BSY 0x80
#define ATA_STATUS_REGISTER_ERR 0x01
#define ATA_STATUS_REGISTER_DRQ 0x08

bool g_MasterDriveExists = false;
bool g_SlaveDriveExists = false;

void waitForBSYClear() {
    while (x86_InByte(ATA_PORT_STATUS_COMMAND) & ATA_STATUS_REGISTER_BSY)
        ;
}

void waitForDRQOrERRSet() {
    while (!(x86_InByte(ATA_PORT_STATUS_COMMAND) & (ATA_STATUS_REGISTER_DRQ | ATA_STATUS_REGISTER_ERR)))
        ;
}

// waits for bsy to clear and drq to set
void poll() {
    uint8_t status;

    while (true) {
        status = x86_InByte(ATA_PORT_STATUS_COMMAND);

        if (status & ATA_STATUS_REGISTER_BSY)
            continue;
        if (!(status & ATA_STATUS_REGISTER_DRQ))
            continue;

        break;
    }
}

bool identify(bool master) {
    uint8_t slaveBit;
    uint16_t *dataBuffer;
    if (master) {
        slaveBit = 0;
        dataBuffer = (uint16_t *)MEMORY_ATA_MASTER_IDENTIFY_BUFFER;
    } else {
        slaveBit = 0x10;
        dataBuffer = (uint16_t *)MEMORY_ATA_SLAVE_IDENTIFY_BUFFER;
    }

    x86_OutByte(ATA_PORT_DRIVE_SELECT, ATA_SELECT_IDENTIFY + slaveBit);
    x86_OutByte(ATA_PORT_SECTOR_COUNT, 0);
    x86_OutByte(ATA_PORT_LBA_LOW, 0);
    x86_OutByte(ATA_PORT_LBA_MID, 0);
    x86_OutByte(ATA_PORT_LBA_HIGH, 0);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t identifyReturn = x86_InByte(ATA_PORT_STATUS_COMMAND);

    // drive does not exist
    if (identifyReturn == 0)
        return false;

    waitForBSYClear();

    // some atapi drives dont follow the specification, so as programmers we need to handle this because drive manufacturers can do whatever the fuck they want, and they just expect us to make shit happen anyway
    // this is a check to see if the drive is actually ata
    uint8_t lbaMid = x86_InByte(ATA_PORT_LBA_MID);
    uint8_t lbaHigh = x86_InByte(ATA_PORT_LBA_HIGH);
    if (lbaMid != 0x00 || lbaHigh != 0x00)
        return false;

    // wait for data to be ready
    puts("aaa\n");
    waitForDRQOrERRSet();
    puts("bbb\n");

    // error check
    if (x86_InByte(ATA_PORT_STATUS_COMMAND) & ATA_STATUS_REGISTER_ERR) {
        printf("ATA error: %X\n", x86_InByte(ATA_PORT_ERROR));
        return false;
    }

    for (uint16_t i = 0; i < 256; ++i)
        ((uint16_t *)MEMORY_ATA_MASTER_IDENTIFY_BUFFER)[i] = x86_InWord(ATA_PORT_DATA);

    return true;
}

void ATA_Initialize(ATA_InitializeOutput *output) {
    g_MasterDriveExists = identify(true);
    g_SlaveDriveExists = identify(false);

    output->masterDriveExists = g_MasterDriveExists;
    output->slaveDriveExists = g_SlaveDriveExists;
    output->masterDriveData = (ATA_IdentifyData *)MEMORY_ATA_MASTER_IDENTIFY_BUFFER;
    output->slaveDriveData = (ATA_IdentifyData *)MEMORY_ATA_SLAVE_IDENTIFY_BUFFER;
}

// TODO: implement dma instead of polling
bool ATA_ReadSectors(uint32_t lba, void *buffer, uint8_t count, bool master) {
    uint8_t slaveBit;
    if (master)
        slaveBit = 0;
    else
        slaveBit = 0x10;

    x86_OutByte(ATA_PORT_DRIVE_SELECT, (ATA_SELECT_READ + slaveBit) | ((lba >> 24) & 0x0F));
    x86_OutByte(ATA_PORT_ERROR, 0); // optional, i think
    x86_OutByte(ATA_PORT_SECTOR_COUNT, 1);
    x86_OutByte(ATA_PORT_LBA_LOW, lba & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_READ_SECTORS);
    poll();

    if (x86_InByte(ATA_PORT_STATUS_COMMAND) & ATA_STATUS_REGISTER_ERR) {
        printf("ATA error: %X\n", x86_InByte(ATA_PORT_ERROR));
        return false;
    }

    for (uint16_t i = 0; i < 256; ++i)
        ((uint16_t *)buffer)[i] = x86_InWord(ATA_PORT_DATA);

    return true;
}

// TODO: implement writing