#pragma once

// other contstants
#define MEMORY_HIGHEST_BIOS_ADDRESS ((void *)0xA0000)

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

// 0x00007C00 - 0x00007DFF - bootloader stage 1

// 0x00000500 - 0x0000BFFF - c stack (a little over ~38kb of space)

// 0x0000C000 - 0x0002FFFF - bootloader stage 2 (starts under 0xFFFF to make space for 16 bit code)

// memdetect stuff
#define MEMORY_MEMDETECT_MAX_REGIONS 256
#define MEMORY_MEMDETECT_REGIONS_BUFFER ((void *)0x00030000)
#define MEMORY_MEMDETECT_REGIONS_BUFFER_SIZE (MEMORY_MEMDETECT_MAX_REGIONS * 24)

#define MEMORY_LOWEST_LOW_MEMORY_ADDRESS ((void *)MEMORY_MEMDETECT_REGIONS_BUFFER + MEMORY_MEMDETECT_REGIONS_BUFFER_SIZE)
// lower ram
#define MEMORY_HIGHEST_LOW_MEMORY_ADDRESS ((void *)0x0007FFFF)

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - vga video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDRESS ((void *)0x00100000)
#define MEMORY_MAX_KERNEL_SIZE 0x100000

// 0x00100000 - 0x001FFFFF - kernel

#define MEMORY_ALLOCATOR_IN_USE_BITS ((void *)0x00200000)
#define MEMORY_ALLOCATOR_CHUNK_SIZE 512

// everything after here is upper ram
