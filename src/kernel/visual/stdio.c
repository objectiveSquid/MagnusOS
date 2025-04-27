#include "stdio.h"
#include "font.h"
#include "memdefs.h"
#include "util/x86.h"
#include "vbe.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define TAB_SIZE 4

static VbeModeInfo *g_VbeModeInfo = (VbeModeInfo *)MEMORY_VESA_MODE_INFO;
static uint16_t g_CursorPosition[2] = {0, 0};

void setCursorPosition(uint16_t x, uint16_t y) {
    g_CursorPosition[0] = x;
    g_CursorPosition[1] = y;
}

void putc(char character) {
    FONT_Character fontCharacter = EMPTY_CHARACTER;
    uint16_t screenWidth, screenHeight;

    fontCharacter.typed.character = character;
    screenWidth = FONT_ScreenCharacterWidth();
    screenHeight = FONT_ScreenCharacterHeight();

    switch (character) {
    case '\n':
        g_CursorPosition[0] = 0; // '\r'
        ++g_CursorPosition[1];   // '\n'
        break;
    case '\r':
        g_CursorPosition[0] = 0; // '\r'
        break;
    case '\t':
        for (uint8_t i = 0; i < TAB_SIZE - (g_CursorPosition[0] % TAB_SIZE); ++i)
            putc(' ');
        break;
    case '\b':
        if (g_CursorPosition[0] == 0) {
            g_CursorPosition[0] = screenWidth - 1;
            --g_CursorPosition[1];
            break;
        }
        --g_CursorPosition[0];
        break;
    default:
        FONT_PutCharacter(g_CursorPosition[0], g_CursorPosition[1], fontCharacter);
        ++g_CursorPosition[0];
        break;
    }

    if (g_CursorPosition[0] >= screenWidth) {
        g_CursorPosition[0] = 0;
        ++g_CursorPosition[1];
    }
    if (g_CursorPosition[1] >= screenHeight) {
        FONT_ScrollBack(1);
    }
}

void puts(const char *buf) {
    while (*buf) {
        putc(*buf);
        ++buf;
    }
}

void clearScreen() {
    for (uint8_t x = 0; x < FONT_ScreenCharacterWidth(); ++x)
        for (uint8_t y = 0; y < FONT_ScreenCharacterHeight(); ++y)
            putc('\0');

    g_CursorPosition[0] = 0;
    g_CursorPosition[1] = 0;
}

static const char g_HexChars[] = "0123456789abcdef";

void printf_number_unsigned(uint64_t number, uint8_t radix) {
    char outputBuffer[22]; // 20-22 characters number, and 1 for the null terminator
    int8_t bufferPosition = 0;

    do {
        uint64_t remainder = number % radix;
        number /= radix;
        outputBuffer[bufferPosition++] = g_HexChars[remainder];
    } while (number > 0);

    while (--bufferPosition >= 0)
        putc(outputBuffer[bufferPosition]);
}

void printf_number_signed(int64_t number, uint8_t radix) {
    if (number < 0) {
        putc('-');
        printf_number_unsigned(-number, radix);
    } else {
        printf_number_unsigned(number, radix);
    }
}

#define PRINTF_STATE_NORMAL 0
#define PRINTF_STATE_LENGTH 1
#define PRINTF_STATE_LENGTH_SHORT 2
#define PRINTF_STATE_LENGTH_LONG 3
#define PRINTF_STATE_SPECIFIER 4

#define PRINTF_LENGTH_DEFAULT 0
#define PRINTF_LENGTH_SHORT_SHORT 1
#define PRINTF_LENGTH_SHORT 2
#define PRINTF_LENGTH_LONG 3
#define PRINTF_LENGTH_LONG_LONG 4

#define PRINTF_RADIX_DEFAULT 10
#define PRINTF_SIGN_DEFAULT false

void ASMCALL printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    uint8_t state = PRINTF_STATE_NORMAL;
    uint8_t length = PRINTF_LENGTH_DEFAULT;
    uint8_t radix = PRINTF_RADIX_DEFAULT;
    bool sign = PRINTF_SIGN_DEFAULT;
    bool number = false;

    while (*format) {
        switch (state) {
        case PRINTF_STATE_NORMAL:
            switch (*format) {
            case '%':
                state = PRINTF_STATE_LENGTH;
                break;
            default:
                putc(*format);
                break;
            }
            break;

        case PRINTF_STATE_LENGTH:
            switch (*format) {
            case 'h':
                length = PRINTF_LENGTH_SHORT;
                state = PRINTF_STATE_LENGTH_SHORT;
                break;
            case 'l':
                length = PRINTF_LENGTH_LONG;
                state = PRINTF_STATE_LENGTH_LONG;
                break;
            default:
                goto PRINTF_STATE_SPECIFIER_;
            }
            break;

        case PRINTF_STATE_LENGTH_SHORT:
            if (*format == 'h') {
                length = PRINTF_LENGTH_SHORT_SHORT;
                state = PRINTF_STATE_SPECIFIER;
            } else {
                goto PRINTF_STATE_SPECIFIER_;
            }
            break;

        case PRINTF_STATE_LENGTH_LONG:
            if (*format == 'l') {
                length = PRINTF_LENGTH_LONG_LONG;
                state = PRINTF_STATE_SPECIFIER;
            } else {
                goto PRINTF_STATE_SPECIFIER_;
            }
            break;

        case PRINTF_STATE_SPECIFIER:
        PRINTF_STATE_SPECIFIER_:
            switch (*format) {
            case 'c':
                putc((char)va_arg(args, int));
                break;
            case 's':
                puts(va_arg(args, const char *));
                break;
            case '%':
                putc('%');
                break;
            case 'd':
            case 'i':
            case 'u':
                radix = 10;
                sign = *format != 'u';
                number = true;
                break;
            case 'X':
            case 'x':
            case 'p':
                radix = 16;
                sign = false;
                number = true;
                break;
            case 'o':
                radix = 8;
                sign = false;
                number = true;
                break;

            default:
                break;
            }

            if (number)
                if (sign)
                    switch (length) {
                    case PRINTF_LENGTH_SHORT_SHORT:
                    case PRINTF_LENGTH_SHORT:
                    case PRINTF_LENGTH_DEFAULT:
                        printf_number_signed(va_arg(args, int), radix);
                        break;

                    case PRINTF_LENGTH_LONG:
                        printf_number_signed(va_arg(args, long), radix);
                        break;

                    case PRINTF_LENGTH_LONG_LONG:
                        printf_number_signed(va_arg(args, long long), radix);
                        break;
                    }
                else
                    switch (length) {
                    case PRINTF_LENGTH_SHORT_SHORT:
                    case PRINTF_LENGTH_SHORT:
                    case PRINTF_LENGTH_DEFAULT:
                        printf_number_unsigned(va_arg(args, unsigned), radix);
                        break;

                    case PRINTF_LENGTH_LONG:
                        printf_number_unsigned(va_arg(args, unsigned long), radix);
                        break;

                    case PRINTF_LENGTH_LONG_LONG:
                        printf_number_unsigned(va_arg(args, unsigned long long), radix);
                        break;
                    }

            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            radix = PRINTF_RADIX_DEFAULT;
            sign = PRINTF_SIGN_DEFAULT;
            number = false;
            break;
        }

        ++format;
    }

    va_end(args);
}

void printBuffer(const void *buffer, uint32_t count) {
    const char *charBuffer = (const char *)buffer;

    for (uint32_t i = 0; i < count; i++) {
        printf("%hhx", (uint8_t)(charBuffer[i] >> 4));
        printf("%hhx", (uint8_t)(charBuffer[i] & 0xF));
    }
}
