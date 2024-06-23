#include "stdbool.h"
#include "stdint.h"

void _cdecl x86_Math_Div_64_32(uint64_t dividend, uint32_t divisor, uint64_t *quotientOutput, uint32_t *remainderOutput);

void _cdecl x86_Video_WriteCharTeletype(char c, uint8_t page);

bool _cdecl x86_Disk_Reset(uint8_t drive);
bool _cdecl x86_Disk_Read(uint8_t drive, uint16_t cylinder, uint16_t head, uint16_t sector, uint8_t count, void __far *dataOutput);
bool _cdecl x86_Disk_GetDriveParams(uint8_t drive, uint8_t *driveTypeOutput, uint16_t *cylindersOutput, uint16_t *headsOutput, uint16_t *sectorsOutput);

char _cdecl x86_Keyboard_ReadChar();

void _cdecl x86_Misc_Halt();
