#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    F9 = 0x01,
    F5 = 0x03,
    F3 = 0x04,
    F1 = 0x05,
    F2 = 0x06,
    F12 = 0x07,
    F10 = 0x09,
    F8 = 0x0A,
    F6 = 0x0B,
    F4 = 0x0C,
    TAB = 0x0D,
    BACKTICK = 0x0E,
    LEFT_ALT = 0x11,
    LEFT_SHIFT = 0x12,
    LEFT_CONTROL = 0x14,
    Q = 0x15,
    NUM_1 = 0x16,
    Z = 0x1A,
    S = 0x1B,
    A = 0x1C,
    W = 0x1D,
    NUM_2 = 0x1E,
    C = 0x21,
    X = 0x22,
    D = 0x23,
    E = 0x24,
    NUM_4 = 0x25,
    NUM_3 = 0x26,
    space = 0x29,
    V = 0x2A,
    F = 0x2B,
    T = 0x2C,
    R = 0x2D,
    NUM_5 = 0x2E,
    N = 0x31,
    B = 0x32,
    H = 0x33,
    G = 0x34,
    Y = 0x35,
    NUM_6 = 0x36,
    M = 0x3A,
    J = 0x3B,
    U = 0x3C,
    NUM_7 = 0x3D,
    NUM_8 = 0x3E,
    COMMA = 0x41,
    K = 0x42,
    I = 0x43,
    O = 0x44,
    NUM_0 = 0x45,
    NUM_9 = 0x46,
    PERIOD = 0x49,
    SLASH = 0x4A,
    L = 0x4B,
    SEMICOLON = 0x4C,
    P = 0x4D,
    MINUS = 0x4E,
    SINGLE_QUOTE = 0x52,
    OPEN_BRACKET = 0x54,
    EQUALS = 0x55,
    CAPSLOCK = 0x58,
    RIGHT_SHIFT = 0x59,
    ENTER = 0x5A,
    CLOSE_BRACKET = 0x5B,
    BACKSLASH = 0x5D,
    BACKSPACE = 0x66,
    KEYPAD_1 = 0x69,
    KEYPAD_4 = 0x6B,
    KEYPAD_7 = 0x6C,
    KEYPAD_0 = 0x70,
    KEYPAD_PERIOD = 0x71,
    KEYPAD_2 = 0x72,
    KEYPAD_5 = 0x73,
    KEYPAD_6 = 0x74,
    KEYPAD_8 = 0x75,
    ESCAPE = 0x76,
    NUMLOCK = 0x77,
    F11 = 0x78,
    KEYPAD_PLUS = 0x79,
    KEYPAD_3 = 0x7A,
    KEYPAD_MINUS = 0x7B,
    KEYPAD_MULTIPLY = 0x7C,
    KEYPAD_9 = 0x7D,
    SCROLLLOCK = 0x7E,
    F7 = 0x83,
    MULTI_WWW_SEARCH = 0xE010,
    RIGHT_ALT = 0xE011,
    RIGHT_CONTROL = 0xE014,
    MULTI_PREVIOUS_TRACK = 0xE015,
    MULTI_WWW_FAVOURITES = 0xE018,
    LEFT_GUI = 0xE01F,
    MULTI_WWW_REFRESH = 0xE020,
    MULTI_VOLUME_DOWN = 0xE021,
    MULTI_MUTE = 0xE023,
    RIGHT_GUI = 0xE027,
    MULTI_WWW_STOP = 0xE028,
    MULTI_CALCULATOR = 0xE02B,
    APPS = 0xE02F,
    MULTI_WWW_FORWARD = 0xE030,
    MULTI_VOLUME_UP = 0xE032,
    MULTI_PLAY_PAUSE = 0xE034,
    ACPI_POWER = 0xE037,
    MULTI_WWW_BACK = 0xE038,
    MULTI_WWW_HOME = 0xE03A,
    MULTI_stop = 0xE03B,
    ACPI_SLEET = 0xE03F,
    MULTI_MY_COMPUTER = 0xE040,
    MULTI_email = 0xE048,
    KEYPAD_DIVISION = 0xE04A,
    MULTI_NEXT_TRACK = 0xE04D,
    MULTI_MEDIA_SELECT = 0xE050,
    KEYPAD_ENTER = 0xE05A,
    ACPI_WAKE = 0xE05E,
    END = 0xE069,
    CURSOR_LEFT = 0xE06B,
    HOME = 0xE06C,
    INSERT = 0xE070,
    DELETE = 0xE071,
    CURSOR_DOWN = 0xE072,
    CURSOR_RIGHT = 0xE074,
    CURSOR_UP = 0xE075,
    PAGE_DOWN = 0xE07A,
    PAGE_UP = 0xE07D,
    PRINT_SCREEN_PRESSED = 0xE012E07C,
    PRINT_SCREEN_RELEASED = 0xE0F07CE0F012,
    PAUSE = 0xE11477E1F014F077,
} PS2_ScanCodeSet2;
