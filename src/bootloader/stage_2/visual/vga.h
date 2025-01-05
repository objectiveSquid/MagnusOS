#pragma once

#include <stdbool.h>
#include <stdint.h>

void VGA_ClearScreen();
void VGA_PutCharacter(uint16_t x, uint16_t y, char character);
void VGA_PutColor(uint16_t x, uint16_t y, uint8_t color);
void VGA_SetCursurPosition(uint16_t x, uint16_t y);
void VGA_ScrollBack(uint16_t lineCount);
