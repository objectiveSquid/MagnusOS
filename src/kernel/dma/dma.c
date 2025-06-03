#include "dma.h"
#include "util/errors.h"
#include "util/x86.h"
#include "visual/stdio.h"

#define DMA_COMMAND_MASK_CHANNEL_2 0x06
#define DMA_COMMAND_UNMASK_CHANNEL_2 0x02
#define DMA_COMMAND_SET_MODE_READ 0x46
#define DMA_COMMAND_SET_MODE_WRITE 0x4A

#define DMA_CHANNEL_2 0x02
#define DMA_STATUS_REGISTER 0x08
#define DMA_ERROR_MASK 0x10
#define DMA_CLEAR_PORT 0x0C

#define DMA_MODE_REGISTER 0x0B
#define DMA_ADDRESS_REGISTER_LOW 0x04
#define DMA_ADDRESS_REGISTER_HIGH 0x05
#define DMA_PAGE_REGISTER 0x81
#define DMA_COUNT_REGISTER_LOW 0x05
#define DMA_COUNT_REGISTER_HIGH 0x06
#define DMA_MASK_REGISTER 0x0A

#define DMA_STATUS_REGISTER 0x08
#define DMA_ERROR_MASK 0x10

bool checkDMAStatus() {
    if (x86_InByte(DMA_STATUS_REGISTER) & DMA_ERROR_MASK)
        return false;
    return true;
}

void clearFlipFlop() {
    x86_OutByte(DMA_CLEAR_PORT, 0xFF);
}

int DMA_Setup(uint8_t *buffer, uint16_t length) {
    clearFlipFlop();
    x86_OutByte(DMA_MASK_REGISTER, DMA_COMMAND_UNMASK_CHANNEL_2);

    if (!checkDMAStatus())
        return DMA_CHANNEL_IN_USE_ERROR;

    uint32_t address = (uint32_t)buffer;
    clearFlipFlop();
    x86_OutByte(DMA_ADDRESS_REGISTER_LOW, address & 0xFF);         // Low byte
    x86_OutByte(DMA_ADDRESS_REGISTER_HIGH, (address >> 8) & 0xFF); // High byte
    x86_OutByte(DMA_PAGE_REGISTER, (address >> 16) & 0xFF);        // Page register

    uint16_t count = length - 1;
    clearFlipFlop();
    x86_OutByte(DMA_COUNT_REGISTER_LOW, count & 0xFF);         // Low byte
    x86_OutByte(DMA_COUNT_REGISTER_HIGH, (count >> 8) & 0xFF); // High byte

    x86_OutByte(DMA_MODE_REGISTER, DMA_COMMAND_SET_MODE_READ); // Channel 2, Read mode, Single transfer
    clearFlipFlop();
    x86_OutByte(DMA_MASK_REGISTER, DMA_COMMAND_MASK_CHANNEL_2); // Unmask channel 2

    if (!checkDMAStatus())
        return DMA_SETUP_FAILED_ERROR;

    return NO_ERROR;
}
