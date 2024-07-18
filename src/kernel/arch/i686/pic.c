#include "pic.h"
#include "io.h"

#define PIC_1_COMMAND_PORT 0x20
#define PIC_1_DATA_PORT 0x21
#define PIC_2_COMMAND_PORT 0xA0
#define PIC_2_DATA_PORT 0xA1

enum {
    PIC_ICW1_ICW4 = 0x01,
    PIC_ICW1_INITIALIZE = 0x10
} PIC_ICW1;

enum {
    PIC_ICW4_8086 = 0x1,
    PIC_ICW4_AUTO_EOI = 0x2,
    PIC_ICW4_BUFFER_MASTER = 0x4,
    PIC_ICW4_BUFFER_SLAVE = 0x0,
    PIC_ICW4_BUFFERED = 0x8,
    PIC_ICW4_SFNM = 0x10,
} PIC_ICW4;

enum {
    PIC_CMD_END_OF_INTERRUPT = 0x20,
    PIC_CMD_READ_IRR = 0x0A,
    PIC_CMD_READ_ISR = 0x0B,
} PIC_CMD;

void i686_PIC_Configure(uint8_t offsetPic_1, uint8_t offsetPic_2) {
    // init control word 1
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    i686_IOWait();
    i686_OutByte(PIC_2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    i686_IOWait();

    // init control word 2
    i686_OutByte(PIC_1_DATA_PORT, offsetPic_1);
    i686_IOWait();
    i686_OutByte(PIC_2_DATA_PORT, offsetPic_2);
    i686_IOWait();

    // init control word 3
    i686_OutByte(PIC_1_DATA_PORT, 0x4); // slave at IRQ2 (0000 0100)
    i686_IOWait();
    i686_OutByte(PIC_2_DATA_PORT, 0x2); // cascade identity (0000 0010)
    i686_IOWait();

    // init control word 4
    i686_OutByte(PIC_1_DATA_PORT, PIC_ICW4_8086);
    i686_IOWait();
    i686_OutByte(PIC_2_DATA_PORT, PIC_ICW4_8086);
    i686_IOWait();

    // clear data registers
    i686_OutByte(PIC_1_DATA_PORT, 0);
    i686_IOWait();
    i686_OutByte(PIC_1_DATA_PORT, 0);
    i686_IOWait();
}

void i686_PIC_SendEndOfInterrupt(irq_t irq) {
    if (irq >= 8)
        i686_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i686_PIC_Disable() {
    i686_OutByte(PIC_1_DATA_PORT, 0xFF); // mask all
    i686_IOWait();
    i686_OutByte(PIC_2_DATA_PORT, 0xFF); // mask all
    i686_IOWait();
}

void i686_PIC_Mask(irq_t irq) {
    uint8_t port;

    if (irq >= 8) {
        port = PIC_1_DATA_PORT;
    } else {
        irq -= 8;
        port = PIC_2_DATA_PORT;
    }

    uint8_t mask = i686_InByte(PIC_1_DATA_PORT);
    i686_OutByte(PIC_1_DATA_PORT, mask | (1 << irq));
}

void i686_PIC_Unmask(irq_t irq) {
    uint8_t port;

    if (irq >= 8) {
        port = PIC_1_DATA_PORT;
    } else {
        irq -= 8;
        port = PIC_2_DATA_PORT;
    }

    uint8_t mask = i686_InByte(PIC_1_DATA_PORT);
    i686_OutByte(PIC_1_DATA_PORT, mask & ~(1 << irq));
}

IRQRequestRegisters i686_PIC_GetIRQRequestRegister() {
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i686_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_READ_IRR);

    IRQRequestRegisters output;
    output.first = i686_InByte(PIC_1_DATA_PORT);
    output.second = i686_InByte(PIC_2_DATA_PORT) << 8;

    return output;
}

InServiceRegisters i686_PIC_GetInServiceRegister() {
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i686_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_READ_ISR);

    InServiceRegisters output;
    output.first = i686_InByte(PIC_1_DATA_PORT);
    output.second = i686_InByte(PIC_2_DATA_PORT) << 8;

    return output;
}