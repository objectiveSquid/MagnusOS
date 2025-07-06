#pragma once

#include <lib/memory/memdetect.h>
#include <stdbool.h>
#include <stddef.h>

#define PAGE_SIZE 0x1000

int ALLOCATOR_Initialize(const MEMDETECT_MemoryRegion *memoryRegions, uint32_t memoryRegionsCount, bool skipInUseBits);

// do not expose this to userspace
void *ALLOCATOR_Malloc(size_t size, bool lower, bool pageAligned);

void *mallocPageAligned(size_t size);
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void free(void *ptr);
