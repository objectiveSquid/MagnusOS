#include "ps2.h"
#include "scancode.h"
#include "visual/stdio.h"
#include <lib/algorithm/arrays.h>
#include <lib/algorithm/bits.h>
#include <lib/errors/errors.h>
#include <lib/interrupt/irq/irq.h>
#include <lib/memory/memory.h>
#include <lib/time/pit.h>
#include <lib/x86/general.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// irqs
#define PS2_PORT_1_IRQ 1
#define PS2_PORT_2_IRQ 12

// ps2 ports
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

// ps2 controller response codes
#define PS2_SELF_TEST_SUCCESS 0x55

// ps2 keyboard response codes
#define PS2_KEY_DETECTION_ERROR 0x00
#define PS2_RESET_SUCCESS 0xAA
#define PS2_ECHO_RESPONSE 0xEE
#define PS2_ACK 0xFA
#define PS2_RESEND 0xFE
#define PS2_KEY_DETECTION_ERROR_2 0xFF

// status register bits
#define PS2_STATUS_OUTPUT_BUFFER_FULL 0x01
#define PS2_STATUS_INPUT_BUFFER_FULL 0x02

// other signals
#define PS2_NEW_KEYBOARD 0x61

// other
#define PS2_MAX_EMPTY_BUFFER_COUNT 100
#define DEFAULT_PS2_TIMEOUT_MS 20

typedef enum {
    LED_SET_IGNORE = 0,
    LED_SET_ON = 1,
    LED_SET_OFF = 2,
    LED_SET_TOGGLE = 3,
} SetLEDNumber;

static uint8_t g_LEDState = 0;
static uint8_t g_SkipPS2Interrupts = 0;
static const PICDriver *g_PicDriver = NULL;
uint8_t g_ScancodesHeld[((ARRAY_SIZE(SCANCODE_SET_2_INDEXES) + 7) / 8)]; // add 7 then divide by 8 to round up

// returns false if the timeout is reached
bool clearPS2Buffer() {
    uint16_t readBytes = 0;

    while ((x86_InByte(PS2_CMD_PORT) & PS2_STATUS_OUTPUT_BUFFER_FULL) && (readBytes++ < PS2_MAX_EMPTY_BUFFER_COUNT))
        x86_InByte(PS2_DATA_PORT);

    return readBytes < PS2_MAX_EMPTY_BUFFER_COUNT;
}

// i am NOT checking this 22 times!
// returns false if the timeout is reached
bool waitForPS2Controller() {
    uint64_t endTimeMs = PIT_GetTimeMs() + DEFAULT_PS2_TIMEOUT_MS;

    while ((x86_InByte(PS2_CMD_PORT) & PS2_STATUS_INPUT_BUFFER_FULL) && (PIT_GetTimeMs() < endTimeMs))
        ;

    return PIT_GetTimeMs() < endTimeMs;
}

bool getPS2Success() {
    return x86_InByte(PS2_DATA_PORT) == PS2_ACK;
}

void writePS2Port1(uint8_t byte) {
    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, byte);
}

void writePS2Port2(uint8_t byte) {
    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, PS2_CMD_WRITE_TO_PORT_2);

    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, byte);
}

// error probably not critical, so not handling it
void resetPS2Keyboard(bool port_1, bool port_2) {
    if (port_1) {
        waitForPS2Controller();
        writePS2Port1(PS2_CMD_RESET);
    }

    if (port_2) {
        waitForPS2Controller();
        writePS2Port2(PS2_CMD_RESET);
    }
}

void disablePS2ControllerPorts(bool port_1, bool port_2) {
    if (port_1) {
        waitForPS2Controller();
        x86_OutByte(PS2_CMD_PORT, PS2_CMD_DISABLE_PORT_1);
    }

    if (port_2) {
        waitForPS2Controller();
        x86_OutByte(PS2_CMD_PORT, PS2_CMD_DISABLE_PORT_2);
    }
}

void enablePS2ControllerPorts(bool port_1, bool port_2) {
    if (port_1) {
        waitForPS2Controller();
        x86_OutByte(PS2_CMD_PORT, PS2_CMD_ENABLE_PORT_1);
    }

    if (port_2) {
        waitForPS2Controller();
        x86_OutByte(PS2_CMD_PORT, PS2_CMD_ENABLE_PORT_2);
    }
}

void setPS2ControllerConfiguration(bool setIRQ1, bool setIRQ2) {
    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_READ_CONFIG); // read config

    uint8_t config = x86_InByte(PS2_DATA_PORT);
    if (setIRQ1)
        FLAG_SET(config, 0b1);
    else
        FLAG_UNSET(config, 0b1);

    if (setIRQ2)
        FLAG_SET(config, 0b10);
    else
        FLAG_UNSET(config, 0b10);

    FLAG_UNSET(config, 0b1000000); // translation
    FLAG_SET(config, 0b10000);     // enable clock signal

    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_WRITE_CONFIG); // write config

    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, config);
}

bool setPS2LEDState(uint8_t ledState) {
    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, PS2_CMD_SET_LEDS);

    if (!getPS2Success()) {
        puts("Could not send command to write LED state to PS2 keyboard\n");
        return false;
    }

    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, ledState);

    if (!getPS2Success()) {
        puts("Could not write LED state to PS2 keyboard\n");
        return false;
    }

    g_LEDState = ledState;

    return true;
}

bool addPS2LEDState(SetLEDNumber scrollLock, SetLEDNumber numLock, SetLEDNumber capsLock) {
    uint8_t oldLEDState = g_LEDState;

    if (scrollLock == LED_SET_ON)
        BIT_SET(g_LEDState, 0);
    if (scrollLock == LED_SET_OFF)
        BIT_UNSET(g_LEDState, 0);
    if (scrollLock == LED_SET_TOGGLE)
        BIT_TOGGLE(g_LEDState, 0);

    if (numLock == LED_SET_ON)
        BIT_SET(g_LEDState, 1);
    if (numLock == LED_SET_OFF)
        BIT_UNSET(g_LEDState, 1);
    if (numLock == LED_SET_TOGGLE)
        BIT_TOGGLE(g_LEDState, 1);

    if (capsLock == LED_SET_ON)
        BIT_SET(g_LEDState, 2);
    if (capsLock == LED_SET_OFF)
        BIT_UNSET(g_LEDState, 2);
    if (capsLock == LED_SET_TOGGLE)
        BIT_TOGGLE(g_LEDState, 2);

    if (!setPS2LEDState(g_LEDState)) {
        g_LEDState = oldLEDState; // revert on error
        return false;
    }

    return true;
}

bool set2CheckPS2LEDState(uint8_t scancode, bool press) {
    bool value = LED_SET_IGNORE;
    if (press)
        value = LED_SET_TOGGLE;

    switch (scancode) {
    case SET_2_SCROLLLOCK:
        return addPS2LEDState(value, LED_SET_IGNORE, LED_SET_IGNORE);
    case SET_2_NUMLOCK:
        return addPS2LEDState(LED_SET_IGNORE, value, LED_SET_IGNORE);
    case SET_2_CAPSLOCK:
        return addPS2LEDState(LED_SET_IGNORE, LED_SET_IGNORE, value);
    }

    return true;
}

bool setPS2ScancodeSet(uint8_t scancodeSet) {
    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, PS2_CMD_SET_SCANCODE_SET);

    if (!getPS2Success()) {
        puts("Could not send command to write scancode set to PS2 keyboard\n");
        return false;
    }

    waitForPS2Controller();
    x86_OutByte(PS2_DATA_PORT, scancodeSet);

    if (!getPS2Success()) {
        puts("Could not write scancode set to PS2 keyboard\n");
        return false;
    }

    return true;
}

int8_t getIndexFromScancode(uint16_t scancode) {
    return (int8_t)findElementInArray(SCANCODE_SET_2_INDEXES, ARRAY_SIZE(SCANCODE_SET_2_INDEXES), scancode);
}

bool isScancodeHeld(uint16_t scancode) {
    int8_t bitIndex = getIndexFromScancode(scancode);
    if (bitIndex == -1) {
        printf("Unknown scancode %x (on check)\n", scancode);
        return false;
    }

    return g_ScancodesHeld[bitIndex] & (1 << (bitIndex % 8));
}

// ugly ternaries
void setScancodeHeld(uint16_t scancode, bool held) {
    int8_t bitIndex = getIndexFromScancode(scancode);
    if (bitIndex == -1) {
        printf("Unknown scancode %x (on set %s)\n", scancode, held ? "held" : "release");
        return;
    }

    if (held)
        g_ScancodesHeld[bitIndex] |= (1 << (bitIndex % 8));
    else
        g_ScancodesHeld[bitIndex / 8] &= ~(1 << (bitIndex % 8));

    printf("Scancode 0x%hx is %s\n", scancode, held ? "held" : "released");
    return;
}

bool runPS2ControllerSelfTest() {
    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_CONTROLLER_SELF_TEST);

    if (x86_InByte(PS2_DATA_PORT) != PS2_SELF_TEST_SUCCESS)
        return false;

    return true;
}

bool checkPS2ControllerDualChannel() {
    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_ENABLE_SECOND_PORT);

    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_READ_CONFIG);

    uint8_t config = x86_InByte(PS2_DATA_PORT);
    if (BIT_IS_SET(config, 5))
        return false; // not a dual channel

    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_DISABLE_SECOND_PORT);

    BIT_UNSET(config, 1); // disable irq for port 2
    BIT_UNSET(config, 5); // enable clock for port 2

    return true;
}

void runPS2InterfaceTests(bool isDualChannel, bool *usingPort1, bool *usingPort2) {
    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_INTERFACE_TEST_PORT_1);

    if (x86_InByte(PS2_DATA_PORT) == 0x00)
        *usingPort1 = true;

    if (!isDualChannel)
        return;

    waitForPS2Controller();
    x86_OutByte(PS2_CMD_PORT, PS2_CMD_INTERFACE_TEST_PORT_2);

    if (x86_InByte(PS2_DATA_PORT) == 0x00)
        *usingPort2 = true;
}

// port can be 1 or 2
void PS2Set2Handler(uint8_t port) {
    if (g_SkipPS2Interrupts) {
        x86_InByte(PS2_DATA_PORT); // ignore
        --g_SkipPS2Interrupts;
        return;
    }

    uint16_t scancode = (uint16_t)x86_InByte(PS2_DATA_PORT);

    switch (scancode) {
    case PS2_NEW_KEYBOARD:
        puts("PS2 Keyboard detected\n");
        return;
    case PS2_ACK:
        puts("Spillover PS2 command acknowledge response\n");
        return;
    case PS2_RESEND:
        puts("Spillover PS2 command resend response\n");
        return;
    case PS2_KEY_DETECTION_ERROR:
    case PS2_KEY_DETECTION_ERROR_2:
        puts("Spillover PS2 key detection error or internal buffer overrun response.\n");
        return;
    case PS2_ECHO_RESPONSE:
        puts("Spillover PS2 echo response\n");
        return;
    case PS2_RESET_SUCCESS:
        puts("Spillover PS2 reset success response\n");
        return;
    }

    switch (scancode) {
    case 0xE1:                                                 // always "pause pressed"
        setScancodeHeld(PS2_SCANCODE_SET_2_SHORT_PAUSE, true); // there is no "pause released" scancode, the os will deal with it
        g_SkipPS2Interrupts = 7;                               // skip next 7 interrupts
        return;
    case PS2_SCANCODE_SET_2_EXTENDED:                           // extended scancode
        scancode = (scancode << 8) | x86_InByte(PS2_DATA_PORT); // read next byte
        g_SkipPS2Interrupts = 1;

        if ((uint8_t)scancode == PS2_SCANCODE_SET_2_KEY_RELEASE) { // extended scancode release
            scancode &= ~PS2_SCANCODE_SET_2_KEY_RELEASE;
            scancode |= x86_InByte(PS2_DATA_PORT);
            if ((uint8_t)scancode == 0x7C) { // always "print screen released"
                g_SkipPS2Interrupts = 3;
                setScancodeHeld(PS2_SCANCODE_SET_2_SHORT_PRINT_SCREEN, false);
            } else {
                setScancodeHeld(scancode, false);
            }
        } else {
            if ((uint8_t)scancode == 0x12) { // always "print screen pressed"
                g_SkipPS2Interrupts = 2;
                setScancodeHeld(PS2_SCANCODE_SET_2_SHORT_PRINT_SCREEN, true);
            } else {
                setScancodeHeld(scancode, true);
            }
        }
        return;
    case PS2_SCANCODE_SET_2_KEY_RELEASE: // single byte scancode release
        scancode = x86_InByte(PS2_DATA_PORT);
        set2CheckPS2LEDState(scancode, false);
        setScancodeHeld(scancode, false);
        g_SkipPS2Interrupts = 1;
        return;

    default: // single byte scancode press
        set2CheckPS2LEDState(scancode, true);
        setScancodeHeld(scancode, true);
        return;
    }
}

// maybe mask the interrupts while handling, maybe some kind of race condition? not sure
void PS2Set2HandlerPort1(Registers *registers) {
    // g_PicDriver->mask(PS2_PORT_1_IRQ);
    PS2Set2Handler(1);
    // g_PicDriver->unmask(PS2_PORT_1_IRQ);
}

void PS2Set2HandlerPort2(Registers *registers) {
    // g_PicDriver->mask(PS2_PORT_2_IRQ);
    PS2Set2Handler(2);
    // g_PicDriver->unmask(PS2_PORT_2_IRQ);
}

int PS2_Initialize() {
    // controller initialization
    disablePS2ControllerPorts(true, true); // true, true = port 1 disabled, port 2 disabled
    if (!clearPS2Buffer())
        return TIMEOUT_ERROR;
    setPS2ControllerConfiguration(false, false); // false, false = disable irq1, disable irq2
    if (!runPS2ControllerSelfTest())
        return PS2_SELF_TEST_FAILED;
    setPS2ControllerConfiguration(false, false); // should do this after self testing
    bool isDualChannel = checkPS2ControllerDualChannel();
    bool usingPort1, usingPort2;
    runPS2InterfaceTests(isDualChannel, &usingPort1, &usingPort2);
    if (!usingPort1 && !usingPort2) // both ports dont work/disabled
        return PS2_INTERFACE_TESTS_FAILED;
    enablePS2ControllerPorts(usingPort1, usingPort2); // enable working ports
    setPS2ControllerConfiguration(usingPort1, usingPort2);
    resetPS2Keyboard(usingPort1, usingPort2);

    // other initialization
    memset(g_ScancodesHeld, 0, sizeof(g_ScancodesHeld));
    g_PicDriver = i686_IRQ_GetDriver();

    // keyboard initialization
    setPS2LEDState(0); // error not critical, so not handling it
    setPS2ScancodeSet(2);
    if (usingPort1) {
        i686_IRQ_RegisterHandler(PS2_PORT_1_IRQ, PS2Set2HandlerPort1);
        g_PicDriver->unmask(PS2_PORT_1_IRQ);
    }
    if (usingPort2) {
        i686_IRQ_RegisterHandler(PS2_PORT_2_IRQ, PS2Set2HandlerPort2);
        g_PicDriver->unmask(PS2_PORT_2_IRQ);
    }

    return NO_ERROR;
}
