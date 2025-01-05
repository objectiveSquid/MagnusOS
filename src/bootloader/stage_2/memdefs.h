#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

#define MEMORY_MIN 0x00000500
#define MEMORY_MAX 0x00080000

#define MEMORY_FAT_ADDRESS ((void *)0x00020000)
#define MEMORY_FAT_SIZE 0x10000 // 64 kilobytes

// 0x00020000 - 0x0002FFFF - bootloader stage 2

#define MEMORY_VESA_INFO ((void *)0x00030000)      // this is 0x200 bytes in hex
#define MEMORY_VESA_MODE_INFO ((void *)0x00030200) // this is 0x200 bytes in hex

// 0x00030000 - 0x00080000 - free

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDRESS ((void *)0x00100000)

// 0x00100000 - 0x001FFFFF - kernel

#define MEMORY_SCREEN_CHARACTER_BUFFER ((void *)0x00200000)     // this is up to 0x7E900 bytes in hex
#define MEMORY_RASTERFONT_BITS_LOAD_BUFFER ((void *)0x0027E900) // this is 0x100 bytes in hex
#define MEMORY_RASTERFONT_BITS ((void *)0x0027EA00)             // this is up to 0xB500 bytes in hex
#define MEMORY_FAT12_DISK_BUFFER ((void *)0x00289F00)           // this is only 0x7 (0x8 cause of alignment) bytes in hex, but i want a constant address for it
