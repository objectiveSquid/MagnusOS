#include <stdint.h>

void putc(char c);
void puts(const char *buf);
void printf(const char *format, ...);
void printBuffer(const void *buffer, uint32_t count);

void VGA_PutColor(uint16_t x, uint16_t y, uint8_t color);
void VGA_PutCharacter(uint16_t x, uint16_t y, char c);
void VGA_ClearScreen();
