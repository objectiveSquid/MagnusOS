#include "ata.h"
#include "disk.h"
#include "pit/pit.h"
#include "util/io.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include <lib/algorithm/math.h>
#include <lib/errors/errors.h>
#include <lib/memory/memdefs.h>
#include <stdbool.h>
#include <stddef.h>

// for restricting the size of the lba
#define MAX_48_BIT_UNSIGNED_INTEGER 0x1000000000000
#define MAX_28_BIT_UNSIGNED_INTEGER 0x10000000

#define ATA_PORT_DATA 0x1F0
#define ATA_PORT_ERROR 0x1F1
#define ATA_PORT_SECTOR_COUNT 0x1F2
#define ATA_PORT_LBA_LOW 0x1F3
#define ATA_PORT_LBA_MID 0x1F4
#define ATA_PORT_LBA_HIGH 0x1F5
#define ATA_PORT_DRIVE_SELECT 0x1F6
#define ATA_PORT_STATUS_COMMAND 0x1F7

#define ATA_PORT_ALTERNATE_STATUS 0x3F6
#define ATA_PORT_CONTROL 0x3F6

#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_READ_SECTORS_EXTENDED 0x24  // for 48 bit lba
#define ATA_CMD_WRITE_SECTORS_EXTENDED 0x34 // for 48 bit lba
#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_CACHE_FLUSH 0xE7

// these are the values if doing stuff with the master drive, for most commands the value to use for the slave drive is simply 1 higher than these values
#define ATA_SELECT_IDENTIFY 0xA0
#define ATA_SELECT_READWRITE 0xE0
#define ATA_SELECT_READWRITE_EXTENDED 0x40

// control register stuff
#define ATA_CONTROL_NIEN 0x02 // dont send interrupts
#define ATA_CONTROL_SRST 0x04 // software reset

// the status register stuff
#define ATA_STATUS_REGISTER_BSY 0x80
#define ATA_STATUS_REGISTER_DF 0x20
#define ATA_STATUS_REGISTER_ERR 0x01
#define ATA_STATUS_REGISTER_DRQ 0x08
#define ATA_STATUS_REGISTER_SRV 0x10

const char *ataErrorMessages[] = {
    "Address mark not found",
    "Track zero not found",
    "Command aborted",
    "Media change request",
    "ID not found",
    "Media changed",
    "Uncorrectable data error",
    "Bad block detected",
};

static uint8_t g_ControlPortByte = 0x00;

void waitForBSYClear() {
    while (x86_InByte(ATA_PORT_ALTERNATE_STATUS) & ATA_STATUS_REGISTER_BSY)
        ;
}

void waitForDRQOrERRSet() {
    while (!(x86_InByte(ATA_PORT_ALTERNATE_STATUS) & (ATA_STATUS_REGISTER_DRQ | ATA_STATUS_REGISTER_ERR)))
        ;
}

void waitForDRQAndBSYClear() {
    while (x86_InByte(ATA_PORT_ALTERNATE_STATUS) & (ATA_STATUS_REGISTER_BSY | ATA_STATUS_REGISTER_DRQ))
        ;
}

// waits for bsy to clear and drq to set
void poll() {
    uint8_t status;

    while (true) {
        status = x86_InByte(ATA_PORT_ALTERNATE_STATUS);

        if (status & ATA_STATUS_REGISTER_BSY)
            continue;
        if (!(status & ATA_STATUS_REGISTER_DRQ || status & ATA_STATUS_REGISTER_ERR || status & ATA_STATUS_REGISTER_DF))
            continue;

        break;
    }
}

bool identify(bool master, ATA_IdentifyData *outputBuffer, uint8_t *errorCodeOutput) {
    uint16_t *dataBuffer = (uint16_t *)outputBuffer;

    uint8_t slaveBit;
    if (master)
        slaveBit = 0;
    else
        slaveBit = 0x10;

    x86_OutByte(ATA_PORT_DRIVE_SELECT, ATA_SELECT_IDENTIFY + slaveBit);
    x86_OutByte(ATA_PORT_SECTOR_COUNT, 0);
    x86_OutByte(ATA_PORT_LBA_LOW, 0);
    x86_OutByte(ATA_PORT_LBA_MID, 0);
    x86_OutByte(ATA_PORT_LBA_HIGH, 0);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t identifyReturn = x86_InByte(ATA_PORT_ALTERNATE_STATUS);

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
    waitForDRQOrERRSet();

    // error check
    if (x86_InByte(ATA_PORT_ALTERNATE_STATUS) & ATA_STATUS_REGISTER_ERR) {
        if (errorCodeOutput != NULL)
            *errorCodeOutput = x86_InByte(ATA_PORT_ERROR);
        return false;
    }

    for (uint16_t i = 0; i < 256; ++i)
        dataBuffer[i] = x86_InWord(ATA_PORT_DATA);

    return true;
}

void softwareReset() {
    g_ControlPortByte |= ATA_CONTROL_SRST;
    x86_OutByte(ATA_PORT_CONTROL, g_ControlPortByte);
    g_ControlPortByte &= ~ATA_CONTROL_SRST;
}

void selectDrive28Bit(uint32_t lba, uint8_t slaveBit) {
    x86_OutByte(ATA_PORT_DRIVE_SELECT, (ATA_SELECT_READWRITE + slaveBit) | ((lba >> 24) & 0x0F));
}

void selectDrive48Bit(uint8_t slaveBit) {
    x86_OutByte(ATA_PORT_DRIVE_SELECT, ATA_SELECT_READWRITE_EXTENDED + slaveBit);
}

void readSectors28BitLba(uint32_t lba, uint8_t count, uint8_t slaveBit) {
    selectDrive28Bit(lba, slaveBit);

    x86_OutByte(ATA_PORT_ERROR, 0); // optional, i think
    x86_OutByte(ATA_PORT_SECTOR_COUNT, count);
    x86_OutByte(ATA_PORT_LBA_LOW, lba & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_READ_SECTORS);
}

void readSectors48BitLba(uint64_t lba, uint16_t count, uint8_t slaveBit) {
    selectDrive48Bit(slaveBit);

    x86_OutByte(ATA_PORT_SECTOR_COUNT, (count >> 8) & 0xFF); // high sector count byte
    x86_OutByte(ATA_PORT_LBA_LOW, (lba >> 24) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 32) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 40) & 0xFF);
    x86_OutByte(ATA_PORT_SECTOR_COUNT, count & 0xFF); // low sector count byte
    x86_OutByte(ATA_PORT_LBA_LOW, lba & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_READ_SECTORS_EXTENDED);
}

void cacheFlush() {
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_CACHE_FLUSH);
}

int checkErrors() {
    uint8_t status = x86_InByte(ATA_PORT_ALTERNATE_STATUS);
    uint8_t errorCode = x86_InByte(ATA_PORT_ERROR);

    if (status & ATA_STATUS_REGISTER_DF)
        return ATA_DRIVE_FAULT_ERROR;

    if (status & ATA_STATUS_REGISTER_ERR || errorCode != 0)
        switch (findLowestSetBit(errorCode)) {
        case 0:
            return ATA_ADDRESS_MARK_NOT_FOUND_ERROR;
        case 1:
            return ATA_TRACK_ZERO_NOT_FOUND_ERROR;
        case 2:
            return ATA_COMMAND_ABORTED_ERROR;
        case 3:
            return ATA_MEDIA_CHANGE_REQUEST_ERROR;
        case 4:
            return ATA_ID_NOT_FOUND_ERROR;
        case 5:
            return ATA_MEDIA_CHANGED_ERROR;
        case 6:
            return ATA_UNCORRECTABLE_DATA_ERROR;
        case 7:
            return ATA_BAD_BLOCK_DETECTED_ERROR;
        }

    return NO_ERROR;
}

int ATA_ReadSectors(uint64_t lba, void *buffer, uint16_t count, DISK *disk) {
    uint8_t slaveBit;
    if (disk->isMaster)
        slaveBit = 0;
    else
        slaveBit = 0x10;

    if (x86_InByte(ATA_PORT_ALTERNATE_STATUS) & ATA_STATUS_REGISTER_SRV)
        softwareReset();
    waitForBSYClear();
    if (disk->supports48BitLba && lba > MAX_28_BIT_UNSIGNED_INTEGER) { // 28 bit is faster
        if (lba > ((ATA_IdentifyData *)disk->ataData)->Max48BitLBA || lba > MAX_48_BIT_UNSIGNED_INTEGER)
            return ATA_LBA_TOO_LARGE_48BIT_ERROR;
        readSectors48BitLba(lba, count, slaveBit);
    } else {
        if (lba > ((ATA_IdentifyData *)disk->ataData)->Max28BitLBA || lba > MAX_28_BIT_UNSIGNED_INTEGER)
            return ATA_LBA_TOO_LARGE_28BIT_ERROR;
        readSectors28BitLba(lba, count, slaveBit);
    }
    poll();

    int status;
    if ((status = checkErrors()) != NO_ERROR)
        return status;

    for (uint16_t sectorIndex = 0; sectorIndex < count; ++sectorIndex) {
        poll();
        if ((status = checkErrors()) != NO_ERROR)
            return status;
        waitNsRough(400);

        for (uint16_t dataWordIndex = 0; dataWordIndex < 256; ++dataWordIndex)
            ((uint16_t *)buffer)[dataWordIndex] = x86_InWord(ATA_PORT_DATA);
        buffer += 512;
    }

    return NO_ERROR;
}

void writeSectors28BitLba(uint32_t lba, uint8_t count, uint8_t slaveBit) {
    selectDrive28Bit(lba, slaveBit);

    x86_OutByte(ATA_PORT_ERROR, 0); // optional, i think
    x86_OutByte(ATA_PORT_SECTOR_COUNT, count);
    x86_OutByte(ATA_PORT_LBA_LOW, lba & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_WRITE_SECTORS);
}

void writeSectors48BitLba(uint64_t lba, uint16_t count, uint8_t slaveBit) {
    selectDrive48Bit(slaveBit);

    x86_OutByte(ATA_PORT_SECTOR_COUNT, (count >> 8) & 0xFF); // high sector count byte
    x86_OutByte(ATA_PORT_LBA_LOW, (lba >> 24) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 32) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 40) & 0xFF);
    x86_OutByte(ATA_PORT_SECTOR_COUNT, count & 0xFF); // low sector count byte
    x86_OutByte(ATA_PORT_LBA_LOW, lba & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_WRITE_SECTORS_EXTENDED);
}

int ATA_WriteSectors(uint64_t lba, void *buffer, uint16_t count, DISK *disk) {
    uint8_t slaveBit;
    if (disk->isMaster)
        slaveBit = 0;
    else
        slaveBit = 0x10;

    if (x86_InByte(ATA_PORT_ALTERNATE_STATUS) & ATA_STATUS_REGISTER_SRV)
        softwareReset();
    waitForBSYClear();
    if (disk->supports48BitLba && lba > MAX_28_BIT_UNSIGNED_INTEGER) { // 28 bit is faster
        if (lba > ((ATA_IdentifyData *)disk->ataData)->Max48BitLBA || lba > MAX_48_BIT_UNSIGNED_INTEGER)
            return ATA_LBA_TOO_LARGE_48BIT_ERROR;
        writeSectors48BitLba(lba, count, slaveBit);
    } else {
        if (lba > ((ATA_IdentifyData *)disk->ataData)->Max28BitLBA || lba > MAX_28_BIT_UNSIGNED_INTEGER)
            return ATA_LBA_TOO_LARGE_28BIT_ERROR;
        writeSectors28BitLba(lba, count, slaveBit);
    }
    poll();

    int status;
    if ((status = checkErrors()) != NO_ERROR)
        return status;

    for (uint16_t sectorIndex = 0; sectorIndex < count; ++sectorIndex) {
        poll();
        if ((status = checkErrors()) != NO_ERROR)
            return status;
        waitNsRough(400);

        // volatile makes the compiler not optimize out the loop, because we need to wait "a jmp $+2 size of delay" between each outword
        for (volatile uint16_t dataWordIndex = 0; dataWordIndex < 256; ++dataWordIndex)
            x86_OutWord(ATA_PORT_DATA, ((uint16_t *)buffer)[dataWordIndex]);
        buffer += 512;
    }

    cacheFlush();

    return count;
}

void ATA_Initialize(ATA_InitializeDriveOutput *masterOutput, ATA_InitializeDriveOutput *slaveOutput) {
    g_ControlPortByte = 0x00;
    x86_OutByte(ATA_PORT_CONTROL, g_ControlPortByte); // clear all properties of the control port (recommandation from wiki.osdev.org)

    masterOutput->driveExists = identify(true, masterOutput->driveData, &masterOutput->errorCode);
    slaveOutput->driveExists = identify(false, slaveOutput->driveData, &slaveOutput->errorCode);
}
