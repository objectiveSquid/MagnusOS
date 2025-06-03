#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MEMORY_TYPE_AVAILABLE = 1,
    MEMORY_TYPE_RESERVED = 2,
    MEMORY_TYPE_ACPI_RECLAIM = 3,
    MEMORY_TYPE_NVS = 4,
} MEMDETECT_MemoryType;

typedef struct {
    uint64_t baseAddress;
    uint64_t size;
    uint32_t type;
    uint32_t ACPIInfo;
} __attribute__((packed)) MEMDETECT_MemoryRegion;

// probably dont use in kernel
typedef enum {
    MEMDETECT_ERROR_CARRY_SET = 1,
    MEMDETECT_ERROR_UNSUPPORTED = 2,
    MEMDETECT_ERROR_OTHER = 3,
} MEMDETECT_GetMemoryRegionsErrorCode;

// also probably dont use in kernel
extern const char *MEMDETECT_ErrorCodeStrings[];

// do not use in kernel, only defined in bootloader
int MEMDETECT_GetMemoryRegions(MEMDETECT_MemoryRegion *regionsOutput, uint32_t maxRegions, uint32_t *regionCountOutput, uint8_t *errorCodeOutput);
