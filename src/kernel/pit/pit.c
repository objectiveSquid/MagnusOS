#include "pit.h"
#include "arch/i686/irq.h"
#include "util/x86.h"
#include <stdint.h>

#define PIT_CHANNEL_0_PORT 0x40
#define PIT_CHANNEL_1_PORT 0x41
#define PIT_CHANNEL_2_PORT 0x42
#define PIT_COMMAND_PORT 0x43

#define PIT_IRQ 0

#define PIT_FREQUENZY_HZ 1193182

volatile uint64_t pitTicks = 0;

/*
Command for port 0x43:

Bits         Usage
6 and 7      Select channel :
                0 0 = Channel 0
                0 1 = Channel 1
                1 0 = Channel 2
                1 1 = Read-back command (8254 only)
4 and 5      Access mode :
                0 0 = Latch count value command
                0 1 = Access mode: lobyte only
                1 0 = Access mode: hibyte only
                1 1 = Access mode: lobyte/hibyte
1 to 3       Operating mode :
                0 0 0 = Mode 0 (interrupt on terminal count)
                0 0 1 = Mode 1 (hardware re-triggerable one-shot)
                0 1 0 = Mode 2 (rate generator)
                0 1 1 = Mode 3 (square wave generator)
                1 0 0 = Mode 4 (software triggered strobe)
                1 0 1 = Mode 5 (hardware triggered strobe)
                1 1 0 = Mode 2 (rate generator, same as 010b)
                1 1 1 = Mode 3 (square wave generator, same as 011b)
0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
*/
void setFrequency(uint16_t hz) {
    uint16_t divisor = (PIT_FREQUENZY_HZ / hz) - 1;
    uint8_t lsb = divisor & 0xFF;
    uint8_t msb = (divisor >> 8) & 0xFF;

    x86_OutByte(PIT_COMMAND_PORT, 0x36); // look above this function for explanation
    x86_OutByte(PIT_CHANNEL_0_PORT, lsb);
    x86_OutByte(PIT_CHANNEL_0_PORT, msb);
}

void PIT_Delay(uint32_t milliseconds) {
    uint32_t targetTicks = pitTicks + milliseconds;
    while (pitTicks < targetTicks)
        asm volatile("hlt"); // let the cpu chill until the next interrupt
}

void irq0Handler() {
    ++pitTicks;
}

void PIT_Initialize() {
    i686_IRQ_RegisterHandler(PIT_IRQ, irq0Handler);
    setFrequency(1000);
    i686_IRQ_GetDriver()->unmask(PIT_IRQ);
}
