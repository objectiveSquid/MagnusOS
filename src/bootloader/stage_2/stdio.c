#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "x86.h"

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

const char g_hexChars[] = "0123456789abcdef";

int *printf_number(int *arg_pointer, int length, bool sign, uint_t radix) {
    char outputBuffer[23]; // 20-22 characters number, and 1 for the null terminator
    uint64_t number;
    int number_sign = 1;
    int bufferPosition = 0;

    // process length and sign
    switch (length) {
    case PRINTF_LENGTH_SHORT_SHORT:
    case PRINTF_LENGTH_SHORT:
    case PRINTF_LENGTH_DEFAULT:
        if (sign) {
            int n = *arg_pointer;
            if (n < 0) {
                n = -n;
                number_sign = -1;
            }
            number = (uint64_t)n;
        } else {
            number = *(uint_t *)arg_pointer;
        }
        ++arg_pointer;
        break;
    case PRINTF_LENGTH_LONG:
        if (sign) {
            lint_t n = *(lint_t *)arg_pointer;
            if (n < 0) {
                n = -n;
                number_sign = -1;
            }
            number = (uint64_t)n;
        } else {
            number = *(ulint_t *)arg_pointer;
        }
        arg_pointer += 2;
        break;
    case PRINTF_LENGTH_LONG_LONG:
        if (sign) {
            llint_t n = *(llint_t *)arg_pointer;
            if (n < 0) {
                n = -n;
                number_sign = -1;
            }
            number = (uint64_t)n;
        } else {
            number = *(ullint_t *)arg_pointer;
        }
        arg_pointer += 4;
        break;
    }

    do {
        uint32_t remainder;
        x86_Math_Div_64_32(number, radix, &number, &remainder);
        outputBuffer[bufferPosition++] = g_hexChars[remainder];
    } while (number > 0);

    if (sign && number_sign < 0)
        outputBuffer[bufferPosition++] = '-';

    while (--bufferPosition >= 0)
        putc(outputBuffer[bufferPosition]);

    return arg_pointer;
}

void _cdecl printf(const char *format, ...) {
    int state = PRINTF_STATE_NORMAL;
    int length = PRINTF_LENGTH_DEFAULT;
    int radix = PRINTF_RADIX_DEFAULT;
    bool sign = PRINTF_SIGN_DEFAULT;

    // if these 2 lines are placed in the start of the function, the compilation fails, I DONT KNOW WHY!
    // my suspicion is that there is a parsing bug in the open watcom v2 compiler
    int *arg_pointer = (int *)&format;
    arg_pointer++;

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
                putc((char)*arg_pointer);
                arg_pointer++;
                break;
            case 's':
                puts(*(char **)arg_pointer);
                arg_pointer++;
                break;
            case '%':
                putc('%');
                break;
            case 'd':
            case 'i':
            case 'u':
                radix = 10;
                sign = *format != 'u';
                arg_pointer = printf_number(arg_pointer, length, sign, radix);
                break;
            case 'X':
            case 'x':
            case 'p':
                radix = 16;
                sign = false;
                arg_pointer = printf_number(arg_pointer, length, sign, radix);
                break;
            case 'o':
                radix = 8;
                sign = false;
                arg_pointer = printf_number(arg_pointer, length, sign, radix);
                break;

            default:
                break;
            }
            state = PRINTF_STATE_NORMAL;
            length = PRINTF_LENGTH_DEFAULT;
            radix = PRINTF_RADIX_DEFAULT;
            sign = PRINTF_SIGN_DEFAULT;
            break;
        }
        ++format;
    }
}

void putc(char c) {
    x86_Video_WriteCharTeletype(c, 0);
}

void puts(const char *buf) {
    for (; *buf; ++buf)
        putc(*buf);
}

char getch() {
    return x86_Keyboard_ReadChar();
}

size_t input(char *outputBuffer, uint64_t maxInput, char stopChar, bool nullTerm) {
    char currentChar;
    size_t i = 0;
    for (; i < maxInput; ++i) {
        currentChar = getch();
        if (currentChar == stopChar)
            break;
        outputBuffer[i] = currentChar;
        putc(currentChar);
    }
    if (nullTerm)
        outputBuffer[i + 1] = '\0';

    return i;
}
