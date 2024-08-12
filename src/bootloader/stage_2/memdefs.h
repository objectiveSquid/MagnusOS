#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

#define MEMORY_MIN 0x00000500
#define MEMORY_MAX 0x00080000

#define MEMORY_FAT_ADDRESS ((void *)0x20000)
#define MEMORY_FAT_SIZE 0x00010000 // 64 kilobytes

#define MEMORY_LOAD_KERNEL_BUFFER ((void *)0x30000)
#define MEMORY_LOAD_KERNEL_CHUNK_SIZE 0x10000 // the kernel load chunk-size

// 0x00020000 - 0x00030000 - bootloader stage 2

#define MEMORY_VESA_INFO ((void *)0x00030000)
#define MEMORY_VESA_MODE_INFO ((void *)0x00030200) // the previous struct is 512 bytes (0x200 in hex)

// 0x00030000 - 0x00080000 - free

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDRESS ((void *)0x100000)