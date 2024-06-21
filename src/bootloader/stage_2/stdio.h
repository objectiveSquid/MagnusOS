#include "stdbool.h"
#include "stdint.h"

void putc(char c);
void puts(const char *buf);
void _cdecl printf(const char *format, ...);
char getch();
size_t input(char *outputBuffer, uint64_t maxInput, char stopChar, bool nullTerm);
