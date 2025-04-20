#include "fdc.h"
#include "arch/i686/irq.h"
#include "disk.h"
#include "dma/dma.h"
#include "memdefs.h"
#include "pit/pit.h"
#include "stdbool.h"
#include "util/binary.h"
#include "util/io.h"
#include "util/memory.h"
#include "util/x86.h"
#include "visual/stdio.h"
#include <stdbool.h>
#include <stddef.h>

#define MAX_RETRY_COUNT 3

#define DOR_PORT 0x3F2
#define MSR_PORT 0x3F4
#define DATA_PORT 0x3F5
#define CCR_PORT 0x3F7

#define FDC_IRQ 6

#define FDC_COMMAND_RECALIBRATE 0x07
#define FDC_COMMAND_SEEK 0x0F
#define FDC_COMMAND_SENSE_DRIVE_STATUS 0x04
#define FDC_COMMAND_SPECIFY 0x03
#define FDC_COMMAND_CONFIGURE 0x13
#define FDC_COMMAND_READ_DATA 0x40 | 0x80 | 0x06
#define FDC_COMMAND_DISABLE 0x00
#define FDC_COMMAND_ENABLE 0x0C
#define FDC_COMMAND_SENSE_INTERRUPT_STATUS 0x08
#define FDC_COMMAND_GET_VERSION 0x10

#define MSR_RQM 0x80
#define MSR_DIO 0x40
#define MSR_NDMA 0x20

#define DEFAULT_TIMEOUT 100

static volatile bool irqHappened = false;

void irq6Handler(Registers *registers) {
    irqHappened = true;
}

bool waitForIRQ(uint32_t timeoutMilliseconds) {
    for (uint32_t i = 0; i < timeoutMilliseconds; i++) {
        if (irqHappened) {
            printf("IRQ happened at %lu milliseconds\n", i);
            irqHappened = false;
            return true;
        }
        PIT_Delay(1);
    }

    printf("Timeout hit waiting for IRQ\n");
    return false;
}

bool waitReadyForCommand(uint32_t timeoutMilliseconds) {
    uint32_t timeout = timeoutMilliseconds;
    uint8_t msr = x86_InByte(MSR_PORT);

    while (!(msr & MSR_RQM) && (msr & MSR_DIO)) {
        if (--timeout == 0) {
            printf("Timeout hit waiting for MSR ready\n");
            return false;
        }
        PIT_Delay(1);
        msr = x86_InByte(MSR_PORT);
    }

    return true;
}

bool writeData(uint8_t data) {
    if (!waitReadyForCommand(DEFAULT_TIMEOUT)) {
        printf("Failed to write data: timeout\n");
        return false;
    }
    x86_OutByte(DATA_PORT, data);
    return true;
}

uint8_t readData() {
    while (!(x86_InByte(MSR_PORT) & MSR_RQM))
        ;

    return x86_InByte(DATA_PORT);
}

bool senseDriveState(uint8_t driveId, uint8_t *st3Output) {
    if (!writeData(FDC_COMMAND_SENSE_DRIVE_STATUS)) { // Sense Drive Status command
        printf("Failed to send sense drive command\n");
        return false;
    }

    if (!writeData(driveId)) {
        printf("Failed to send drive ID\n");
        return false;
    }

    if (st3Output != NULL)
        *st3Output = readData();
    else
        readData();

    return true;
}

bool checkInterrupt(uint8_t *st0Output, uint8_t *cylinderOutput) {
    if (!writeData(FDC_COMMAND_SENSE_INTERRUPT_STATUS)) {
        printf("Failed to send sense interrupt status command\n");
        return false;
    }

    if (st0Output != NULL)
        *st0Output = readData();
    if (cylinderOutput != NULL)
        *cylinderOutput = readData();

    return true;
}

bool reset(bool doCheckInterrupt, uint8_t diskId) {
    x86_OutByte(DOR_PORT, FDC_COMMAND_DISABLE);
    PIT_Delay(100); // MAYBE REMOVE DEBUG
    x86_OutByte(DOR_PORT, FDC_COMMAND_ENABLE);

    if (!doCheckInterrupt)
        return true;

    if (!waitForIRQ(DEFAULT_TIMEOUT)) {
        printf("Failed to reset floppy: timeout\n");
        return false;
    }

    for (int i = 0; i < 4; i++) {
        if (!checkInterrupt(NULL, NULL)) {
            printf("Failed to reset floppy: check interrupt %d\n", i);
            if (i == 0)
                return false; // only fail if first check fails
        }
    }

    senseDriveState(diskId, NULL);

    return true;
}

void setSelectedDrive(uint8_t driveId) {
    if (driveId > 3) {
        printf("Invalid drive id, must be less than or equal to 3: %d\n", driveId);
        return;
    }

    uint8_t dor = x86_InByte(DOR_PORT);
    dor |= driveId;
    x86_OutByte(DOR_PORT, dor);
}

void setDMAOn() {
    uint8_t dor = x86_InByte(DOR_PORT);
    BIT_SET(dor, 3);
    x86_OutByte(DOR_PORT, dor);
}

void motorOn() {
    uint8_t dor = x86_InByte(DOR_PORT);
    BIT_SET(dor, 4);
    x86_OutByte(DOR_PORT, dor);

    PIT_Delay(500);
}

void motorOff() {
    uint8_t dor = x86_InByte(DOR_PORT);
    BIT_UNSET(dor, 4);
    x86_OutByte(DOR_PORT, dor);
}

bool calibrate(uint8_t driveId) {
    uint8_t st0, cylinder;

    senseDriveState(driveId, NULL);

    for (uint8_t i = 0; i < MAX_RETRY_COUNT; i++) {
        if (!writeData(FDC_COMMAND_RECALIBRATE)) {
            printf("Failed to send recalibrate floppy command: timeout\n");
            continue;
        }
        if (!writeData(driveId)) { // argument is drive, we only support 0
            printf("Failed to recalibrate floppy (1st byte): timeout\n");
            continue;
        }

        if (!waitForIRQ(3000)) {
            printf("Failed to calibrate floppy: timeout\n");
            continue;
        }
        if (!checkInterrupt(&st0, &cylinder)) {
            printf("Failed to calibrate floppy: check interrupt\n");
            continue;
        }

        if (st0 & 0xC0) {
            static const char *status[] =
                {0, "error", "invalid", "drive"};
            printf("Failed to calibrate floppy: status = %s\n", status[st0 >> 6]);
            continue;
        }

        if (cylinder != 0) { // found cylinder 0 ?
            printf("Failed to calibrate floppy: cylinder 0 not returned\n");
            continue;
        }

        return true;
    }

    printf("Failed to calibrate floppy: %hhu retries exhausted\n", MAX_RETRY_COUNT);
    return false;
}

void LBAToCHS(DISK *disk, uint32_t lba, uint16_t *cylinderOutput, uint16_t *headOutput, uint16_t *sectorOutput) {
    // cylinder = (lba / sectors per track) / heads
    *cylinderOutput = (lba / disk->sectors) / disk->heads;

    // head = (lba / sectors per track) % heads
    *headOutput = (lba / disk->sectors) % disk->heads;

    // sector = (lba % sectors per track) + 1
    *sectorOutput = (lba % disk->sectors) + 1;
}

void setTransferSpeed() {
    x86_OutByte(CCR_PORT, 0x00); // 0x00 = 500kbps
}

bool specify() {
    if (!writeData(FDC_COMMAND_SPECIFY)) {
        printf("Failed to send specify command: timeout\n");
        return false;
    }
    if (!writeData(0xDF)) { /* steprate = 3ms, unload time = 240ms */
        printf("Failed to specify (1st byte): timeout\n");
        return false;
    }
    if (!writeData(0x02)) { /* load time = 16ms, no-DMA = 0 */
        printf("Failed to specify (2nd byte): timeout\n");
        return false;
    }

    return true;
}

bool seek(DISK *disk, uint32_t lba) {
    uint16_t cylinder, head, sector;
    LBAToCHS(disk, lba, &cylinder, &head, &sector);

    if (!writeData(FDC_COMMAND_SEEK)) {
        printf("Failed to send seek command: timeout\n");
        return false;
    }
    if (!writeData((head << 2) | 0)) {
        printf("Failed to seek (1st byte): timeout\n");
        return false;
    }
    if (!writeData(cylinder)) {
        printf("Failed to seek (2nd byte): timeout\n");
        return false;
    }

    if (!waitForIRQ(3000)) {
        printf("Failed to seek: timeout\n");
        return false;
    }

    uint8_t st0;
    if (!checkInterrupt(&st0, NULL)) {
        printf("Failed to seek: check interrupt\n");
        return false;
    }

    if (!(st0 & (0x20 | disk->id))) {
        printf("Failed to seek: status = 0x%hhx\n", st0);
        return false;
    }

    return true;
}

bool checkValidVersion() {
    if (!writeData(FDC_COMMAND_GET_VERSION)) {
        printf("Failed to send get version command: timeout\n");
        return false;
    }

    uint8_t version = readData();
    if (version != 0x90) {
        printf("Invalid version: 0x%hhx\n", version);
        return false;
    }

    return true;
}

bool configure() {
    if (!writeData(FDC_COMMAND_CONFIGURE)) {
        printf("Failed to send configure command: timeout\n");
        return false;
    }
    if (!writeData(0x00)) {
        printf("Failed to configure (1st byte): timeout\n");
        return false;
    }
    // Second parameter byte = (implied seek ENable << 6) | (fifo DISable << 5) | (drive polling mode DISable << 4) | thresh_val (= threshold - 1)
    if (!writeData(0x57)) { // imlpied seek enabled, fifo enabled, drive polling disabled, threshold = 8 (- 1 = 7)
        printf("Failed to configure (2nd byte): timeout\n");
        return false;
    }
    if (!writeData(0x00)) {
        printf("Failed to configure (3rd byte): timeout\n");
        return false;
    }

    return true;
}

void drainFDC() {
    uint8_t msr;
    uint8_t i;

    for (i = 0; i < 10; i++) {
        msr = x86_InByte(MSR_PORT);

        if ((msr & 0xC0) == 0xC0) {
            // Read the data to clear the FDC's buffer
            x86_InByte(DATA_PORT);
        } else {
            // FDC doesn't have data to send, we're done
            break;
        }
    }
}

FDC_ISSUE_COMMAND_RESULT issueCommand(uint8_t diskId, uint8_t *commandBytes, uint8_t numberOfCommandBytes, bool executionPhase, bool resultPhase, uint8_t *dataBuffer, uint32_t maxDataBufferLength, bool dataBufferWriteToFIFO) {
    for (uint8_t i = 0; i < numberOfCommandBytes; i++) {
        if (!writeData(commandBytes[i])) {
            reset(false, diskId);
            printf("Failed to issue command, on parameter byte #%hhu. #0 means on the command byte: timeout\n", i);
            return ISSUE_COMMAND_TIMEOUT;
        }
    }
    // if NDMA is not set (dma is on), go directly to result phase
    if (!executionPhase || !(x86_InByte(MSR_PORT) & MSR_NDMA))
        goto result_phase;

execution_phase:
    while ((x86_InByte(MSR_PORT) & 0xA0) == 0xA0 && maxDataBufferLength-- != 0) {
        if (!waitForIRQ(DEFAULT_TIMEOUT)) {
            reset(false, diskId);
            printf("Failed to issue command: execution phase IRQ6 timeout\n");
            return ISSUE_COMMAND_EXECUTION_IRQ6_TIMEOUT;
        }

        if (dataBufferWriteToFIFO)
            x86_OutByte(DATA_PORT, *dataBuffer++);
        else
            *dataBuffer++ = readData();
    }

result_phase:
    if (!resultPhase)
        return ISSUE_COMMAND_SUCCESS;
    if (!(x86_InByte(MSR_PORT) & MSR_NDMA)) { // DMA on
        if (!waitForIRQ(DEFAULT_TIMEOUT)) {
            reset(false, diskId);
            printf("Failed to issue command: result phase IRQ6 timeout\n");
            return ISSUE_COMMAND_RESULT_IRQ6_TIMEOUT;
        }
    } else { // NDMA enabled (DMA off)
        /* not using NDMA */

        // wait for RQM and DIO to be set
        // while ((x86_InByte(MSR_PORT) & 0xC0) != 0xC0)
        //     ;
        // while (true) {
        //     while
        // }
    }

    return ISSUE_COMMAND_SUCCESS;
}

FDC_READ_SECTOR_RESULT FDC_ReadSector(DISK *disk, uint32_t lba, void *dataOutput) {
    uint16_t cylinder, head, sector;
    LBAToCHS(disk, lba, &cylinder, &head, &sector);

    uint8_t st3;
    senseDriveState(disk->id, &st3);
    if (st3 & 0x80) {
        printf("Failed to read data: FDD fault\n");
        return false;
    }

    setSelectedDrive(disk->id);
    if (!calibrate(disk->id)) {
        printf("Failed to read data: calibrate\n");
        return false;
    }

    memset(MEMORY_DMA_BUFFER, 0, 512);
    DMA_Setup(MEMORY_DMA_BUFFER, 512); // subtract 1 from 512 maybe?

    uint8_t readSectorCommandBytes[9] = {FDC_COMMAND_READ_DATA, (head << 2) | disk->id, cylinder, head, sector, 0x02, 18, 0x1B, 0xFF};
    switch (issueCommand(disk->id, readSectorCommandBytes, sizeof(readSectorCommandBytes), true, true, NULL, 0, false)) {
    case ISSUE_COMMAND_TIMEOUT:
        printf("Failed to read data: timeout\n");
        return FDC_READ_SECTOR_SEND_DATA_TIMEOUT;
    case ISSUE_COMMAND_EXECUTION_IRQ6_TIMEOUT:
        printf("Failed to read data: execution phase IRQ6 timeout\n");
        return FDC_READ_SECTOR_EXECUTION_IRQ6_TIMEOUT;
    case ISSUE_COMMAND_RESULT_IRQ6_TIMEOUT:
        printf("Failed to read data: result phase IRQ6 timeout\n");
        return FDC_READ_SECTOR_RESULT_IRQ6_TIMEOUT;
    }

    // while (!(x86_InByte(MSR_PORT) & MSR_RQM))
    //     ;

    // receive output data
    uint8_t st0, st1, st2, end_head, end_sector;
    st0 = readData();
    st1 = readData();
    st2 = readData(); // useless? (according to osdev.wiki)
    readData();       // "cylinder number", idk what it is so not gonna use it
    end_head = readData();
    end_sector = readData();
    readData(); // always a 2 for some reason

    if (st0 & 0xC0) {
        printf("Failed to read sector: st0 = 0x%hhx, lba = %lu\n", st0, lba);
        return FDC_READ_SECTOR_ERROR;
    }

    if (st1 & 0x80) {
        printf("Failed to read sector, not enough sectors error: st1 = 0x%hhx, lba = %lu\n", st1, lba);
        return FDC_READ_SECTOR_NOT_ENOUGH_SECTORS;
    } else if (st1 & 0x2) {
        printf("Failed to read sector, write protected error: st1 = 0x%hhx, lba = %lu\n", st1, lba);
        return FDC_READ_SECTOR_WRITE_PROTECTED;
    }

    memcpy(dataOutput, MEMORY_DMA_BUFFER, 512);

    return true;
}

bool FDC_ReadSectors(DISK *disk, uint32_t lba, uint16_t sectorCount, void *dataOutput) {
    FDC_READ_SECTOR_RESULT readSectorResult;

    while (sectorCount-- > 0) {
        if ((readSectorResult = FDC_ReadSector(disk, lba, dataOutput)) != FDC_READ_SECTOR_SUCCESS) {
            printf("Failed to read lba %lu, result = %hhu\n", lba, readSectorResult);
            return false;
        }
        dataOutput += 512;
        ++lba;
    }

    return true;
}

bool FDC_Initialize(uint8_t diskId) {
    i686_IRQ_RegisterHandler(FDC_IRQ, irq6Handler);
    i686_IRQ_GetDriver()->unmask(FDC_IRQ);

    drainFDC();

    if (!checkValidVersion())
        return false;
    if (!reset(true, diskId))
        return false;
    if (!specify())
        return false;
    setTransferSpeed();
    if (!configure())
        return false;
    setDMAOn();
    setSelectedDrive(diskId);
    motorOn();

    if (!calibrate(diskId)) {
        motorOff();
        return false;
    }

    return true;
}
