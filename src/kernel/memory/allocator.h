#pragma once

#include "memdetect.h"
#include <stddef.h>

// DO NOT MODIFY THE REGIONS AFTER CALLING THIS FUNCTION
void ALLOCATOR_Initialize(const MEMDETECT_MemoryRegion *memoryRegions, uint32_t memoryRegionsCount);
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *ptr);