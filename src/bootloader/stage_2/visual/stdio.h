#include <stdint.h>

void setCursorPosition(uint16_t x, uint16_t y);

void putc(char c);
void puts(const char *buf);
void printf(const char *format, ...);
void printBuffer(const void *buffer, uint32_t count);
