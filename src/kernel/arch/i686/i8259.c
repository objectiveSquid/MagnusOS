#include "i8259.h"
#include "io.h"
#include "pic.h"
#include <stdbool.h>

#define PIC_1_COMMAND_PORT 0x20
#define PIC_1_DATA_PORT 0x21
#define PIC_2_COMMAND_PORT 0xA0
#define PIC_2_DATA_PORT 0xA1

#define PROBE_TEST_MASK 0x1337 // leet

enum {
    PIC_ICW1_ICW4 = 0x01,
    PIC_ICW1_SINGLE = 0x02,
    PIC_ICW1_INTERVAL4 = 0x04,
    PIC_ICW1_LEVEL = 0x08,
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

static picmask_t g_PicMask = PICMASK_ALL;

static bool g_AutoEndOfInterrupt = false;

void i8259_SetMask(picmask_t newMask) {
    g_PicMask = newMask;

    i686_OutByte(PIC_1_DATA_PORT, PICMASK_PIC1(g_PicMask));
    i686_IOWait();
    i686_OutByte(PIC_2_DATA_PORT, PICMASK_PIC2(g_PicMask));
    i686_IOWait();
}

picmask_t i8259_GetMask() {
    return i686_InByte(PIC_1_DATA_PORT) | (i686_InByte(PIC_2_DATA_PORT) << 8);
}

void i8259_Disable() {
    i8259_SetMask(PICMASK_ALL);
}

void i8259_Configure(uint8_t offsetPic_1, uint8_t offsetPic_2, bool autoEndOfInterrupt) {
    // mask all interrupts
    i8259_Disable();

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
    uint8_t icw4 = PIC_ICW4_8086;
    if (autoEndOfInterrupt)
        icw4 |= PIC_ICW4_AUTO_EOI;

    i686_OutByte(PIC_1_DATA_PORT, icw4);
    i686_IOWait();
    i686_OutByte(PIC_2_DATA_PORT, icw4);
    i686_IOWait();

    // mask all interrupts
    i8259_Disable();
}

void i8259_SendEndOfInterrupt(irq_t irq) {
    if (irq >= 8)
        i686_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i8259_Mask(irq_t irq) {
    i8259_SetMask(g_PicMask | (1 << irq));
}

void i8259_Unmask(irq_t irq) {
    i8259_SetMask(g_PicMask & ~(1 << irq));
}

// irr = in request register
// this also means that they are pending (awaiting the cpu)
IRQRequestRegisters i8259_GetIRQRequestRegister() {
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_READ_IRR);
    i686_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_READ_IRR);

    IRQRequestRegisters output = {
        .first = i686_InByte(PIC_1_DATA_PORT),
        .second = i686_InByte(PIC_2_DATA_PORT)};
    return output;
}

// isr = in service register
// meaning that they are currently being handled by the cpu
InServiceRegisters i8259_GetInServiceRegister() {
    i686_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_READ_ISR);
    i686_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_READ_ISR);

    InServiceRegisters output = {
        .first = i686_InByte(PIC_1_DATA_PORT),
        .second = i686_InByte(PIC_2_DATA_PORT),
    };
    return output;
}

bool i8259_Probe() {
    i8259_Disable();
    i8259_SetMask(PROBE_TEST_MASK);
    return i8259_GetMask() == PROBE_TEST_MASK;
}

static const PICDriver g_PicDriver = {
    .name = "i8259 PIC",
    .sendEndOfInterrupt = i8259_SendEndOfInterrupt,
    .probe = i8259_Probe,
    .disable = i8259_Disable,
    .initialize = i8259_Configure,
    .unmask = i8259_Unmask,
    .mask = i8259_Mask,
};

const PICDriver *i8259_GetDriver() {
    return &g_PicDriver;
}