#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

#define MEMORY_MIN 0x00000500
#define MEMORY_MAX 0x00080000

#define MEMORY_FAT_ADDR ((void __far* )0x00500000) // segment:offset (0xSSSSOOOO)
#define MEMORY_FAT_SIZE 0x00010000 // 64 kilobytes

// 0x00020000 - 0x00030000 - bootloader stage 2

// 0x00030000 - 0x00080000 - free

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - video
// 0x000C8000 - 0x000FFFFF - BIOS
