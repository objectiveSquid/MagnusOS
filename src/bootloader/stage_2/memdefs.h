#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

// 0x00000500 - 0x0000FFFF - bootloader stage 2

#define MEMORY_FAT_ADDRESS ((void *)0x00010000) // this must be under 1mb (because the bios has to be able to write to it from the disk)
#define MEMORY_FAT_SIZE 0x20000                 // 128 kilobytes

// 0x00010000 - 0x0002FFFF - fat memory

#define MEMORY_VESA_INFO ((void *)0x00030000)      // this is 0x200 bytes in hex and must be under 1mb (because the bios has to be able to write to it)
#define MEMORY_VESA_MODE_INFO ((void *)0x00030200) // this is 0x200 bytes in hex and must be under 1mb (because the bios has to be able to write to it)

// 0x00030000 - 0x000303FF - vesa (vbe) stuff

#define MEMORY_MEMDETECT_MAX_REGIONS 256
#define MEMORY_MEMDETECT_REGIONS_BUFFER ((void *)0x00030400) // this is (MEMORY_MEMDETECT_MAX_REGIONS * 24) (a region entry is 24 bytes) bytes in hex, maybe needs to be aligned to 4 bytes and must be under 1mb (because the bios has to be able to write to it)
#define MEMORY_MEMDETECT_REGIONS_BUFFER_SIZE (MEMORY_MEMDETECT_MAX_REGIONS * 24)

// 0x00030400 - 0x00031BFF - memdetect stuff

// 0x00031C00 - 0x0007FFFF - free

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - vga video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDRESS ((void *)0x00100000)
#define MEMORY_MAX_KERNEL_SIZE 0x100000

// 0x00100000 - 0x001FFFFF - kernel

// everything after this point must be the same in the kernels memdefs.h
#define MEMORY_ALLOCATOR_CHUNK_SIZE 512 // 4 kilobyte chunks
