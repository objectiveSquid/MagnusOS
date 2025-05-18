#include "ata.h"
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

#define ATA_SELECT_MASTER_IDENTIFY 0xA0
#define ATA_SELECT_SLAVE_IDENTIFY 0xB0
#define ATA_SELECT_MASTER_READ 0xE0
#define ATA_SELECT_SLAVE_READ 0xF0

#define ATA_STATUS_REGISTER_BSY 0x80
#define ATA_STATUS_REGISTER_ERR 0x01

void waitForBSYClear() {
    while (x86_InByte(ATA_PORT_STATUS_COMMAND) & ATA_STATUS_REGISTER_BSY)
        ;
}

// TODO: implement dma instead of polling
bool ATA_ReadSectors(uint32_t lba, void *buffer, uint8_t count, bool master) {
    if (master)
        x86_OutByte(ATA_PORT_DRIVE_SELECT, ATA_SELECT_MASTER_READ | ((lba >> 24) & 0x0F));
    else
        x86_OutByte(ATA_PORT_DRIVE_SELECT, ATA_SELECT_SLAVE_READ | ((lba >> 24) & 0x0F));
    x86_OutByte(ATA_PORT_ERROR, 0); // optional, i think
    x86_OutByte(ATA_PORT_SECTOR_COUNT, 1);
    x86_OutByte(ATA_PORT_LBA_LOW, lba & 0xFF);
    x86_OutByte(ATA_PORT_LBA_MID, (lba >> 8) & 0xFF);
    x86_OutByte(ATA_PORT_LBA_HIGH, (lba >> 16) & 0xFF);
    x86_OutByte(ATA_PORT_STATUS_COMMAND, ATA_CMD_READ_SECTORS);
    waitForBSYClear();

    if (x86_InByte(ATA_PORT_STATUS_COMMAND) & ATA_STATUS_REGISTER_ERR) {
        printf("ATA error: %X\n", x86_InByte(ATA_PORT_ERROR));
        return false;
    }

    for (uint16_t i = 0; i < 256; ++i) {
        ((uint16_t *)buffer)[i] = x86_InWord(ATA_PORT_DATA);
    }

    return true;
}
