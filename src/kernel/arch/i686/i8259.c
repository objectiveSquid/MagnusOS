#include "i8259.h"
#include "pic.h"
#include "util/io.h"
#include "util/x86.h"
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

void i8259_SetMask(picmask_t newMask) {
    g_PicMask = newMask;

    x86_OutByte(PIC_1_DATA_PORT, PICMASK_PIC1(g_PicMask));
    IOWait();
    x86_OutByte(PIC_2_DATA_PORT, PICMASK_PIC2(g_PicMask));
    IOWait();
}

picmask_t i8259_GetMask() {
    return x86_InByte(PIC_1_DATA_PORT) | (x86_InByte(PIC_2_DATA_PORT) << 8);
}

void i8259_Disable() {
    i8259_SetMask(PICMASK_ALL);
}

void i8259_Configure(uint8_t offsetPic_1, uint8_t offsetPic_2, bool autoEndOfInterrupt) {
    // mask all interrupts
    i8259_Disable();

    // init control word 1
    x86_OutByte(PIC_1_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    IOWait();
    x86_OutByte(PIC_2_COMMAND_PORT, PIC_ICW1_ICW4 | PIC_ICW1_INITIALIZE);
    IOWait();

    // init control word 2
    x86_OutByte(PIC_1_DATA_PORT, offsetPic_1);
    IOWait();
    x86_OutByte(PIC_2_DATA_PORT, offsetPic_2);
    IOWait();

    // init control word 3
    x86_OutByte(PIC_1_DATA_PORT, 0x4); // slave at IRQ2 (0000 0100)
    IOWait();
    x86_OutByte(PIC_2_DATA_PORT, 0x2); // cascade identity (0000 0010)
    IOWait();

    // init control word 4
    uint8_t icw4 = PIC_ICW4_8086;
    if (autoEndOfInterrupt)
        icw4 |= PIC_ICW4_AUTO_EOI;

    x86_OutByte(PIC_1_DATA_PORT, icw4);
    IOWait();
    x86_OutByte(PIC_2_DATA_PORT, icw4);
    IOWait();

    // mask all interrupts
    i8259_Disable();
}

void i8259_SendEndOfInterrupt(int irq) {
    if (irq >= 8)
        x86_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    x86_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
}

void i8259_Mask(int irq) {
    i8259_SetMask(g_PicMask | (1 << irq));
}

void i8259_Unmask(int irq) {
    i8259_SetMask(g_PicMask & ~(1 << irq));
}

// irr = in request register
// this also means that they are pending (awaiting the cpu)
uint16_t i8259_GetIRQRequestRegister() {
    x86_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_READ_IRR);
    x86_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_READ_IRR);

    return ((uint16_t)x86_InByte(PIC_2_COMMAND_PORT)) | (((uint16_t)x86_InByte(PIC_2_COMMAND_PORT)) << 8);
}

// isr = in service register
// meaning that they are currently being handled by the cpu
uint16_t i8259_GetInServiceRegister() {
    x86_OutByte(PIC_1_COMMAND_PORT, PIC_CMD_READ_ISR);
    x86_OutByte(PIC_2_COMMAND_PORT, PIC_CMD_READ_ISR);

    return ((uint16_t)x86_InByte(PIC_2_COMMAND_PORT)) | (((uint16_t)x86_InByte(PIC_2_COMMAND_PORT)) << 8);
}

bool i8259_Probe() {
    i8259_Disable();
    i8259_SetMask(PROBE_TEST_MASK);
    return i8259_GetMask() == PROBE_TEST_MASK;
}

static const PICDriver g_PicDriver = {
    .name = "i8259 PIC",
    .sendEndOfInterrupt = &i8259_SendEndOfInterrupt,
    .probe = &i8259_Probe,
    .disable = &i8259_Disable,
    .initialize = &i8259_Configure,
    .unmask = &i8259_Unmask,
    .mask = &i8259_Mask,
};

const PICDriver *i8259_GetDriver() {
    return &g_PicDriver;
}