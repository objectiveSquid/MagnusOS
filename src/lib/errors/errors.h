#pragma once

/*
- All errors must fit into an `int` type
- Error codes are seperated into ranges, so that you can easily bitwise AND to get the error type.
- No errors shall be 0, that is reserved for `NO_ERROR`
- No errors shall be equal.

Which functions don't need to return errors?
- If a function returns a pointer and there is only 1 possible error (null parameters don't count), then it is allowed to return NULL on error.
- If a function only has 1 possible error, then it is allowed to return a boolean to indicate status, but it must list why is would fail in an above comment.
    - But this is not reccomended.
*/

#define NO_ERROR 0

// disk errors
#define DISK_ERROR 0x100
#define DISK_READ_ERROR 0x101
#define DISK_READ_RETRIES_EXHAUSTED_ERROR 0x102
#define DISK_WRITE_ERROR 0x103
#define DISK_WRITE_RETRIES_EXHAUSTED_ERROR 0x104
#define DISK_NOT_ENOUGH_SECTORS_WARNING 0x105
#define ATA_ERROR 0x110
#define ATA_DRIVE_FAULT_ERROR 0x111
#define ATA_LBA_TOO_LARGE_28BIT_ERROR 0x112
#define ATA_LBA_TOO_LARGE_48BIT_ERROR 0x113
#define ATA_ADDRESS_MARK_NOT_FOUND_ERROR 0x114
#define ATA_TRACK_ZERO_NOT_FOUND_ERROR 0x115
#define ATA_COMMAND_ABORTED_ERROR 0x116
#define ATA_MEDIA_CHANGE_REQUEST_ERROR 0x117
#define ATA_ID_NOT_FOUND_ERROR 0x118
#define ATA_MEDIA_CHANGED_ERROR 0x119
#define ATA_UNCORRECTABLE_DATA_ERROR 0x11A
#define ATA_BAD_BLOCK_DETECTED_ERROR 0x11B
#define ATA_TIMEOUT_ERROR 0x11C
#define ATA_DRIVE_DOESNT_EXIST 0x11D
#define ATA_UNSUPPORTED_DRIVE 0x11E

// filesystem errors
#define FILESYSTEM_ERROR 0x200
#define FILESYSTEM_NOT_FOUND_ERROR 0x201
#define FILESYSTEM_READ_ERROR 0x202
#define FILESYSTEM_WRITE_ERROR 0x203
#define FILESYSTEM_OUT_OF_HANDLES_ERROR 0x204
#define FILESYSTEM_INTERNAL_ERROR 0x205
#define FILESYSTEM_NOT_OPEN_ERROR 0x206
#define FILESYSTEM_SEEK_ERROR 0x207
#define FILESYSTEM_EOF_WARNING 0x208

// memory errors
#define MEMORY_ERROR 0x400
#define NULL_ERROR 0x401
#define OUT_OF_BOUNDS_ERROR 0x402
#define OVERFLOW_ERROR 0x403
#define UNDERFLOW_ERROR 0x404
#define ALLOCATION_ERROR 0x405
#define ALLOCATED_MEMORY_TOO_HIGH_ERROR 0x406 // only used by bootloader because it calls some bios functions that need low memory
#define FAILED_TO_ALLOCATE_MEMORY_ERROR 0x407
#define FAILED_TO_DETECT_MEMORY_ERROR 0x408
#define DMA_CHANNEL_IN_USE_ERROR 0x409
#define DMA_SETUP_FAILED_ERROR 0x40A

// elf errors
#define ELF_ERROR 0x800
#define ELF_INVALID_HEADER 0x801
#define ELF_FILE_TOO_SMALL_ERROR 0x802
#define ELF_NOT_AN_ELF_FILE 0x803
#define ELF_UNSUPPORTED_HEADER_VERSION 0x804
#define ELF_UNSUPPORTED_ELF_VERSION 0x805
#define ELF_UNSUPPORTED_BITNESS 0x806
#define ELF_UNSUPPORTED_ENDIANNESS 0x807
#define ELF_UNSUPPORTED_INSTRUCTION_SET 0x808
#define ELF_UNSUPPORTED_ELF_TYPE 0x809

// logic errors
#define LOGIC_ERROR 0x1000
#define NOT_ENOUGH_INPUT_DATA_ERROR 0x1001

// graphics errors (font, vbe etc...)
#define GRAPHICS_ERROR 0x2000
#define VBE_ERROR 0x2001
#define VBE_FAILED_TO_GET_MODE_INFO_ERROR 0x2002
#define VBE_FAILED_TO_SET_MODE_ERROR 0x2003
#define VBE_FAILED_TO_GET_CONTROLLER_INFO_ERROR 0x2004
#define VBE_FAILED_TO_FIND_SUITABLE_MODE_ERROR 0x2005
#define FONT_ERROR 0x2006
#define FONT_NOT_FOUND_ERROR 0x2007

// periphiral errors (usb, ps2 etc...)
#define PERIPHIRAL_ERROR 0x4000
#define PS2_ERROR 0x4001
#define PS2_SELF_TEST_FAILED 0x4002
#define PS2_INTERFACE_TESTS_FAILED 0x4003
#define NO_PIC_DRIVER_FOUND 0x4004
