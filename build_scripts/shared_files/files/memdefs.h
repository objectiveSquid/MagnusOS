#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

// i know this data will overwrite bootloader stage 2, but it doesnt matter since it will only be done once we are in the kernel
#define MEMORY_DMA_BUFFER ((void *)0x00000500) // this is 0x200 bytes in hex, needs to be aligned to 16 bytes, should be under 1mb (maybe 16mb) in addressing and must not cross a 64k boundary

#define MEMORY_ATA_MASTER_IDENTIFY_BUFFER ((void *)0x00000700) // this is 0x200 bytes in hex, needs to be aligned to 4 bytes, must not cross a 64k boundary and must be addressable with 32 bits
#define MEMORY_ATA_SLAVE_IDENTIFY_BUFFER ((void *)0x00000900)  // this is 0x200 bytes in hex, needs to be aligned to 4 bytes, must not cross a 64k boundary and must be addressable with 32 bits

// 0x00000700 - 0x00000AFF - ata stuff

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

// 0x00030400 - 0x00031C00 - memdetect stuff

// 0x00031800 - 0x0007FFFF - free

// 0x00080000 - 0x0009FFFF - extended BIOS data area
// 0x000A0000 - 0x000C7FFF - vga video
// 0x000C8000 - 0x000FFFFF - BIOS

#define MEMORY_KERNEL_ADDRESS ((void *)0x00100000)
#define MEMORY_MAX_KERNEL_SIZE 0x100000

// 0x00100000 - 0x001FFFFF - kernel

#define MEMORY_SCREEN_CHARACTER_BUFFER ((void *)0x00200000)     // this is up to 0x7E900 bytes in hex
#define MEMORY_RASTERFONT_BITS_LOAD_BUFFER ((void *)0x0027E900) // this is 0x100 bytes in hex
#define MEMORY_RASTERFONT_BITS ((void *)0x0027EA00)             // this is up to 0xB500 bytes in hex

// 0x00200000 - 0x00289EFF - rasterfont stuff

#define MEMORY_ALLOCATOR_CHUNK_SIZE 0x1000 // 4 kilobyte chunks
#define MEMORY_ALLOCATOR_IN_USE_BITS ((void *)0x00289F00)

// 0x00289F00 - (unknown, depends on the amount of ram and allocator chunk size) - allocator in use bits

// from here on out its just ram
