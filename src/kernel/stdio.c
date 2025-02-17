#include "stdio.h"
#include "arch/i686/io.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define DEFAULT_COLOR 0x07

#define TAB_SIZE 4

char *g_ScreenBuffer = (char *)0xB8000;
uint16_t g_ScreenX = 0, g_ScreenY = 0;

char getCharacter(uint16_t x, uint16_t y) {
    return g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x)];
}

uint8_t getColor(uint16_t x, uint16_t y) {
    return g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x) + 1];
}

void setCursurPosition(uint16_t x, uint16_t y) {
    uint16_t relativePosition = (y * SCREEN_WIDTH) + x;

    i686_OutByte(0x3D4, 0x0F);
    i686_OutByte(0x3D5, (uint8_t)(relativePosition & 0xFF)); // lower position byte
    i686_OutByte(0x3D4, 0x0E);
    i686_OutByte(0x3D5, (uint8_t)((relativePosition >> 8) & 0xFF)); // upper position byte
}

void scrollBack(uint16_t lineCount) {
    for (uint16_t y = lineCount; y < SCREEN_HEIGHT; ++y)
        for (uint16_t x = 0; x < SCREEN_WIDTH; ++x) {
            putCharacter(x, y - lineCount, getCharacter(x, y));
            putColor(x, y - lineCount, getColor(x, y));
        }

    for (uint16_t y = SCREEN_HEIGHT - lineCount; y < SCREEN_HEIGHT; ++y)
        for (uint16_t x = 0; x < SCREEN_WIDTH; ++x) {
            putCharacter(x, y, '\0');
            putColor(x, y, DEFAULT_COLOR);
        }

    g_ScreenY -= lineCount;
}

void putCharacter(uint16_t x, uint16_t y, char c) {
    g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x)] = c;
}

void putColor(uint16_t x, uint16_t y, uint8_t color) {
    g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x) + 1] = color;
}

void clearScreen() {
    for (uint8_t x = 0; x < SCREEN_WIDTH; ++x)
        for (uint8_t y = 0; y < SCREEN_HEIGHT; ++y) {
            putCharacter(x, y, '\0');
            putColor(x, y, DEFAULT_COLOR);
        }

    g_ScreenX = g_ScreenY = 0;
    setCursurPosition(0, 0);
}

void putc(char c) {
    switch (c) {
    case '\n':
        g_ScreenX = 0; // '\r'
        ++g_ScreenY;   // '\n'
        break;
    case '\r':
        g_ScreenX = 0; // '\r'
        break;
    case '\t':
        for (uint8_t i = 0; i < TAB_SIZE - (g_ScreenX % TAB_SIZE); ++i)
            putc(' ');
        break;
    case '\b':
        if (g_ScreenX == 0) {
            g_ScreenX = SCREEN_WIDTH - 1;
            --g_ScreenY;
            break;
        }
        --g_ScreenX;
        break;
    default:
        putCharacter(g_ScreenX, g_ScreenY, c);
        ++g_ScreenX;
        break;
    }

    if (g_ScreenX >= SCREEN_WIDTH) {
        g_ScreenX = 0;
        ++g_ScreenY;
    }
    if (g_ScreenY >= SCREEN_HEIGHT) {
        scrollBack(1);
    }

    setCursurPosition(g_ScreenX, g_ScreenY);
}

void puts(const char *buf) {
    while (*buf) {
        putc(*buf);
        ++buf;
    }
}

const char g_HexChars[] = "0123456789abcdef";

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
        putc(g_HexChars[charBuffer[i] >> 4]);
        putc(g_HexChars[charBuffer[i] & 0xF]);
    }
}
