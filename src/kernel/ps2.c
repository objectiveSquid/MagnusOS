/* NOTE: who actually cares about error handling lmao */

#include "ps2.h"
#include "arch/i686/io.h"
#include "arch/i686/irq.h"
#include "stdbool.h"
#include "stdio.h"

#define PS2_CMD_PORT 0x64
#define PS2_DATA_PORT 0x60

// ps2 commands
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_SET_SCANCODE_SET 0xF0
#define PS2_CMD_SET_LEDS 0xED

// other
#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_NEW_KEYBOARD 0x61

typedef struct {
    const uint8_t scancode;
    const uint8_t extension;
    bool held;
} PS2_Set1Scancode;

static bool g_ExtendedScancode = false;
static bool g_KeyboardDetected = false;
static uint8_t g_SkipPS2Interrupts = 0;
static uint8_t g_ScancodesHeld[(256 / 8) * 2] = {0}; // scancodes set 1 (1 byte and extended (up to 2 bytes index))

void clearPS2Buffer() {
    while (i686_InByte(PS2_CMD_PORT) & 1)
        i686_InByte(PS2_DATA_PORT);
}

uint8_t makePS2LedState(bool numLock, bool capsLock, bool scrollLock) {
    uint8_t output = 0;

    if (scrollLock)
        output |= 0b1;
    if (numLock)
        output |= 0b10;
    if (capsLock)
        output |= 0b100;

    return output;
}

void waitForPS2Controller() {
    while (i686_InByte(PS2_CMD_PORT) & 0x02)
        ;
}

bool getPS2Success() {
    return i686_InByte(PS2_DATA_PORT) == PS2_ACK;
}

void enablePS2ControllerInterrupts() {
    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_READ_CONFIG); // read config

    uint8_t config = i686_InByte(PS2_DATA_PORT);
    config |= 0x01;

    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_WRITE_CONFIG); // write config

    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, config);
}

bool setPS2LEDState(uint8_t ledState) {
    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, PS2_CMD_SET_LEDS);

    if (!getPS2Success()) {
        puts("Could not send command to write LED state to PS2 controller\n");
        return false;
    }

    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, ledState);

    if (!getPS2Success()) {
        puts("Could not write LED state to PS2 controller\n");
        return false;
    }

    return true;
}

bool isScancodeHeld(uint16_t scancode) {
    return g_ScancodesHeld[scancode / 8] & (1 << (scancode % 8));
}

void setScancodeHeld(uint16_t scancode, bool held) {
    if (scancode & 0xE000) { // extended scancode
    }
    if (held) {
        g_ScancodesHeld[scancode / 8] |= (1 << (scancode % 8));
    } else {
        g_ScancodesHeld[scancode / 8] &= ~(1 << (scancode % 8));
    }
}

void PS2Set1Handler(Registers *registers) {
    if (g_SkipPS2Interrupts) {
        i686_InByte(PS2_DATA_PORT); // ignore
        --g_SkipPS2Interrupts;
        return;
    }

    uint16_t scancode = (uint16_t)i686_InByte(PS2_DATA_PORT);

    switch (scancode) {
    case PS2_NEW_KEYBOARD:
        puts("PS/2 Keyboard detected\n");
        g_KeyboardDetected = true;
        return;
    case PS2_ACK:
        puts("Spillover PS2 command acknowledge response\n");
        return;
    case PS2_RESEND:
        puts("Spillover PS2 command resend response\n");
        return;
    }

    // extended scancode check
    if (g_ExtendedScancode) { // extended scancode now
        scancode |= (uint16_t)0xE000;
        g_ExtendedScancode = false;
    } else if (scancode == 0xE0) { // extended scancode incoming
        g_ExtendedScancode = true;
        return;
    } else if (scancode == 0xE1) {     // always "pause pressed"
        setScancodeHeld(0xE145, true); // there is no "pause released" scancode, the os will deal with it
        printf("Pressed: %x\n", 0xE145);
        g_SkipPS2Interrupts = 5;
        return;
    } else {
        goto after_extended_scancode_check;
    }

    // special extended codes (print screen released/pressed)
    switch (scancode) {
    case 0xE02A:
        setScancodeHeld(0xE02A, true);
        g_SkipPS2Interrupts = 2;
        return;
    case 0xE145:
        setScancodeHeld(0xE02A, false);
        g_SkipPS2Interrupts = 2;
        return;
    }

after_extended_scancode_check:
    if (isScancodeHeld(scancode - 0x80) && scancode > 0x80) {
        setScancodeHeld(scancode - 0x80, false);
    } else {
        setScancodeHeld(scancode, true);
    }
}

bool PS2_Initialize() {
    i686_IRQ_GetDriver()->mask(PS2_IRQ); // just in case

    enablePS2ControllerInterrupts();
    i686_IRQ_RegisterHandler(PS2_IRQ, PS2Set1Handler); // i for some reason couldnt set the scancode set to 2, so we are always using set 1
    clearPS2Buffer();
    setPS2LEDState(0); // error not critical, so not handling it

    i686_IRQ_GetDriver()->unmask(PS2_IRQ);

    return true;
}
