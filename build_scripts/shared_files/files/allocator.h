#pragma once

#include "memdetect.h"
#include <stddef.h>

int ALLOCATOR_Initialize(const MEMDETECT_MemoryRegion *memoryRegions, uint32_t memoryRegionsCount, bool skipInUseBits);

// do not expose this to userspace
void *ALLOCATOR_Malloc(size_t size, bool lower);

void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *ptr);
