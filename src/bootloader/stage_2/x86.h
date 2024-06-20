#include "stdint.h"

void _cdecl x86_div64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOutput, uint32_t *remainderOutput);

void _cdecl x86_Video_WriteCharTeletype(char c, uint8_t page);

void _cdecl x86_halt();
