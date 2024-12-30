/* NOTE: who actually cares about error handling lmao */

#include "ps2.h"
#include "arch/i686/io.h"
#include "arch/i686/irq.h"
#include "stdbool.h"
#include "stdio.h"
#include "util/binary.h"

#define PS2_CMD_PORT 0x64
#define PS2_DATA_PORT 0x60

// ps2 keyboard commands
#define PS2_CMD_SET_SCANCODE_SET 0xF0
#define PS2_CMD_SET_LEDS 0xED
#define PS2_CMD_RESET 0xFF

// ps2 controller commands
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60
#define PS2_CMD_DISABLE_PORT_1 0xAD
#define PS2_CMD_ENABLE_PORT_1 0xAE
#define PS2_CMD_DISABLE_PORT_2 0xA7
#define PS2_CMD_ENABLE_PORT_2 0xA8
#define PS2_CMD_CONTROLLER_SELF_TEST 0xAA
#define PS2_CMD_ENABLE_SECOND_PORT 0xA8
#define PS2_CMD_DISABLE_SECOND_PORT 0xA7
#define PS2_CMD_INTERFACE_TEST_PORT_1 0xAB
#define PS2_CMD_INTERFACE_TEST_PORT_2 0xA9
#define PS2_CMD_WRITE_TO_PORT_2 0xD4

// other
#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_NEW_KEYBOARD 0x61
#define PS2_RESET_SUCCESS 0xAA

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

void writePS2Port1(uint8_t byte) {
    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, byte);
}

void writePS2Port2(uint8_t byte) {
    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, PS2_CMD_WRITE_TO_PORT_2);

    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, byte);
}

uint8_t resetPS2Keyboard(bool port_1, bool port_2) {
    uint8_t output = 0;

    waitForPS2Controller();
    writePS2Port1(PS2_CMD_RESET);

    if (!getPS2Success()) {
        puts("Could not send command to reset PS2 keyboard on port 1\n");
        FLAG_UNSET(output, 0b1);
    } else if (i686_InByte(PS2_DATA_PORT) != PS2_RESET_SUCCESS) {
        puts("Could not reset PS2 keyboard on port 1\n");
        FLAG_UNSET(output, 0b1);
    }

    waitForPS2Controller();
    writePS2Port2(PS2_CMD_RESET);

    if (!getPS2Success()) {
        puts("Could not send command to reset PS2 keyboard on port 2\n");
        FLAG_UNSET(output, 0b10);
    } else if (i686_InByte(PS2_DATA_PORT) != PS2_RESET_SUCCESS) {
        puts("Could not reset PS2 keyboard on port 2\n");
        FLAG_UNSET(output, 0b10);
    }
    return output;
}

void disablePS2ControllerPorts(bool port_1, bool port_2) {
    if (port_1) {
        waitForPS2Controller();
        i686_OutByte(PS2_CMD_PORT, PS2_CMD_DISABLE_PORT_1);
    }

    if (port_2) {
        waitForPS2Controller();
        i686_OutByte(PS2_CMD_PORT, PS2_CMD_DISABLE_PORT_2);
    }
}

void enablePS2ControllerPorts(bool port_1, bool port_2) {
    if (port_1) {
        waitForPS2Controller();
        i686_OutByte(PS2_CMD_PORT, PS2_CMD_ENABLE_PORT_1);
    }

    if (port_2) {
        waitForPS2Controller();
        i686_OutByte(PS2_CMD_PORT, PS2_CMD_ENABLE_PORT_2);
    }
}

void setPS2ControllerConfiguration(bool setIRQ1, bool setIRQ2) {
    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_READ_CONFIG); // read config

    uint8_t config = i686_InByte(PS2_DATA_PORT);
    if (setIRQ1)
        FLAG_SET(config, 0b1);
    else
        FLAG_UNSET(config, 0b1);

    if (false)
        if (setIRQ2)
            FLAG_SET(config, 0b10);
        else
            FLAG_UNSET(config, 0b10);

    FLAG_UNSET(config, 0b1000000); // translation
    FLAG_SET(config, 0b10000);     // enable clock signal

    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_WRITE_CONFIG); // write config

    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, config);
}

bool setPS2LEDState(uint8_t ledState) {
    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, PS2_CMD_SET_LEDS);

    if (!getPS2Success()) {
        puts("Could not send command to write LED state to PS2 keyboard\n");
        return false;
    }

    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, ledState);

    if (!getPS2Success()) {
        puts("Could not write LED state to PS2 keyboard\n");
        return false;
    }

    return true;
}

bool setPS2ScancodeSet(uint8_t scancodeSet) {
    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, PS2_CMD_SET_SCANCODE_SET);

    if (!getPS2Success()) {
        puts("Could not send command to write scancode set to PS2 keyboard\n");
        return false;
    }

    waitForPS2Controller();
    i686_OutByte(PS2_DATA_PORT, scancodeSet);

    if (!getPS2Success()) {
        puts("Could not write scancode set to PS2 keyboard\n");
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

bool runPS2ControllerSelfTest() {
    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_CONTROLLER_SELF_TEST);

    if (i686_InByte(PS2_DATA_PORT) != 0x55) {
        puts("PS2 controller self test failed\n");
        return false;
    }

    return true;
}

bool checkPS2ControllerDualChannel() {
    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_ENABLE_SECOND_PORT);

    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_READ_CONFIG);

    uint8_t config = i686_InByte(PS2_DATA_PORT);
    if (config & 0b100000)
        return false; // not a dual channel

    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_DISABLE_SECOND_PORT);

    FLAG_UNSET(config, 0b10);     // disable irq for port 2
    FLAG_UNSET(config, 0b100000); // enable clock for port 2

    return true;
}

uint8_t runPS2InterfaceTests(bool isDualChannel) {
    uint8_t output = 0;

    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_INTERFACE_TEST_PORT_1);

    if (!i686_InByte(PS2_DATA_PORT) == 0x00)
        FLAG_UNSET(output, 0b1);
    else
        FLAG_SET(output, 0b1);

    if (!isDualChannel) {
        if (!output)
            puts("PS2 interface test failed (0 / 1)\n");
        return output;
    }

    waitForPS2Controller();
    i686_OutByte(PS2_CMD_PORT, PS2_CMD_INTERFACE_TEST_PORT_2);

    if (!i686_InByte(PS2_DATA_PORT) == 0x00)
        FLAG_UNSET(output, 0b10);
    else
        FLAG_SET(output, 0b10);

    if (!output)
        puts("PS2 interface test failed (0 / 2)\n");
    return output;
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
        puts("PS2 Keyboard detected\n");
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
        // printf("Pressed: %x\n", 0xE145);
        g_SkipPS2Interrupts = 5;
        return;
    } else {
        goto after_extended_scancode_check;
    }

    // special extended codes (print screen released/pressed)
    switch (scancode) {
    case 0xE02A:
        setScancodeHeld(0xE02A, true);
        // printf("Pressed: %x\n", 0xE12A);
        g_SkipPS2Interrupts = 2;
        return;
    case 0xE145:
        setScancodeHeld(0xE02A, false);
        // printf("Released: %x\n", 0xE12A);
        g_SkipPS2Interrupts = 2;
        return;
    }

after_extended_scancode_check:
    if (isScancodeHeld(scancode - 0x80) && scancode > 0x80) {
        // printf("Released: %x\n", scancode - 0x80);
        setScancodeHeld(scancode - 0x80, false);
    } else {
        // printf("Pressed: %x\n", scancode);
        setScancodeHeld(scancode, true);
    }
}

bool PS2_Initialize() {
    disablePS2ControllerPorts(true, true); // true, true = port 1 disabled, port 2 disabled
    clearPS2Buffer();
    setPS2ControllerConfiguration(false, false); // false, false = disable irq1, disable irq2
    if (!runPS2ControllerSelfTest())
        return false;
    setPS2ControllerConfiguration(false, false); // should do this after self testing
    bool isDualChannel = checkPS2ControllerDualChannel();
    uint8_t interfaceTestResult = runPS2InterfaceTests(isDualChannel);
    if (!interfaceTestResult)
        return false;
    bool usingPort1 = interfaceTestResult & 0b1;
    bool usingPort2 = interfaceTestResult & 0b10;
    enablePS2ControllerPorts(usingPort1, usingPort2); // enable working ports
    setPS2ControllerConfiguration(usingPort1, usingPort2);
    resetPS2Keyboard(usingPort1, usingPort2); // error probably not critical, so not handling it

    setPS2ScancodeSet(1);
    if (usingPort1) {
        i686_IRQ_RegisterHandler(PS2_PORT_1_IRQ, PS2Set1Handler);
        i686_IRQ_GetDriver()->unmask(PS2_PORT_1_IRQ);
    }
    if (usingPort2) {
        i686_IRQ_RegisterHandler(PS2_PORT_2_IRQ, PS2Set1Handler);
        i686_IRQ_GetDriver()->unmask(PS2_PORT_2_IRQ);
    }

    setPS2LEDState(0); // error not critical, so not handling it

    return true;
}
