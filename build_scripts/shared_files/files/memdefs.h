#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

// 0x00000500 - 0x0000FFFF - bootloader stage 2

#define MEMORY_DMA_BUFFER ((void *)0x00000500) // this is 0x200 bytes in hex, also needs to be aligned to 16 bytes, and should be under 1mb (maybe 16mb) in addressing

// 0x00000500 - 0x0000FFFF - bootloader stage 2

#define MEMORY_FAT_ADDRESS ((void *)0x00010000)
#define MEMORY_FAT_SIZE 0x20000 // 128 kilobytes

// 0x00010000 - 0x0002FFFF - fat memory

#define MEMORY_VESA_INFO ((void *)0x00030000)      // this is 0x200 bytes in hex
#define MEMORY_VESA_MODE_INFO ((void *)0x00030200) // this is 0x200 bytes in hex

// 0x00030400 - 0x00080000 - free

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDRESS ((void *)0x00100000)

// 0x00100000 - 0x001FFFFF - kernel

#define MEMORY_SCREEN_CHARACTER_BUFFER ((void *)0x00200000)     // this is up to 0x7E900 bytes in hex
#define MEMORY_RASTERFONT_BITS_LOAD_BUFFER ((void *)0x0027E900) // this is 0x100 bytes in hex
#define MEMORY_RASTERFONT_BITS ((void *)0x0027EA00)             // this is up to 0xB500 bytes in hex
#define MEMORY_FAT12_DISK_BUFFER ((void *)0x00289F00)           // this is only 24 bytes, but i want a constant address for it