#include "stdio.h"
#include "util/x86.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define DEFAULT_COLOR 0x07

#define TAB_SIZE 4

char *g_ScreenBuffer = (char *)0xB8000;

char getCharacter(uint16_t x, uint16_t y) {
    return g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x)];
}

uint8_t getColor(uint16_t x, uint16_t y) {
    return g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x) + 1];
}

void VGA_SetCursurPosition(uint16_t x, uint16_t y) {
    uint16_t relativePosition = (y * SCREEN_WIDTH) + x;

    x86_OutByte(0x3D4, 0x0F);
    x86_OutByte(0x3D5, (uint8_t)(relativePosition & 0xFF)); // lower position byte
    x86_OutByte(0x3D4, 0x0E);
    x86_OutByte(0x3D5, (uint8_t)((relativePosition >> 8) & 0xFF)); // upper position byte
}

void VGA_PutCharacter(uint16_t x, uint16_t y, char character) {
    g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x)] = character;
}

void VGA_PutColor(uint16_t x, uint16_t y, uint8_t color) {
    g_ScreenBuffer[2 * ((y * SCREEN_WIDTH) + x) + 1] = color;
}

void VGA_ScrollBack(uint16_t lineCount) {
    for (uint16_t y = lineCount; y < SCREEN_HEIGHT; ++y)
        for (uint16_t x = 0; x < SCREEN_WIDTH; ++x) {
            VGA_PutCharacter(x, y - lineCount, getCharacter(x, y));
            VGA_PutColor(x, y - lineCount, getColor(x, y));
        }

    for (uint16_t y = SCREEN_HEIGHT - lineCount; y < SCREEN_HEIGHT; ++y)
        for (uint16_t x = 0; x < SCREEN_WIDTH; ++x) {
            VGA_PutCharacter(x, y, '\0');
            VGA_PutColor(x, y, DEFAULT_COLOR);
        }
}
