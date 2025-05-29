#include "allocator.h"
#include "memdefs.h"
#include "util/memory.h"
#include "util/other.h"
#include <stddef.h>

#define CHUNK_HEADER_TYPE size_t
#define CHUNK_HEADER_SIZE sizeof(CHUNK_HEADER_TYPE)

static uint8_t *g_InUseBits = (uint8_t *)MEMORY_ALLOCATOR_IN_USE_BITS;
static size_t g_LowestUpperUsableAddress;
static uint64_t g_InUseBitsSize; // on 64 bit systems this might actually exceeed the 64 bit unsigned integer limit
static const MEMDETECT_MemoryRegion *g_MemoryRegions;
static size_t g_MemoryRegionsCount;

void ALLOCATOR_Initialize(const MEMDETECT_MemoryRegion *memoryRegions, uint32_t memoryRegionsCount, bool skipInUseBits) {
    // set globals
    g_MemoryRegions = memoryRegions;
    g_MemoryRegionsCount = memoryRegionsCount;

    const MEMDETECT_MemoryRegion *highestRegion = &g_MemoryRegions[0];
    for (uint64_t index = 0; index < g_MemoryRegionsCount; ++index)
        if (g_MemoryRegions[index].baseAddress > highestRegion->baseAddress)
            highestRegion = &g_MemoryRegions[index];

    g_InUseBitsSize = (highestRegion->baseAddress + highestRegion->size) / MEMORY_ALLOCATOR_CHUNK_SIZE;

    // addresses below this aren't for upper ram
    g_LowestUpperUsableAddress = ((size_t)MEMORY_ALLOCATOR_IN_USE_BITS) + BITS2BYTES(g_InUseBitsSize);

    if (skipInUseBits)
        return;

    // clear in use bits
    memset(g_InUseBits, 0x00, BITS2BYTES(g_InUseBitsSize));

    // do not overwrite stack, bootloader stage 2, ivt, etc...
    memset(g_InUseBits, 0xFF, BITS2BYTES(DIV_ROUND_UP((size_t)MEMORY_LOWEST_LOW_MEMORY_ADDRESS, MEMORY_ALLOCATOR_CHUNK_SIZE)));

    // do not overwrite extended bios data area, bios code, kernel, etc...
    size_t biosExtendedDataAreaStart = ((size_t)g_InUseBits) + BITS2BYTES(DIV_ROUND_UP((size_t)MEMORY_HIGHEST_LOW_MEMORY_ADDRESS, MEMORY_ALLOCATOR_CHUNK_SIZE));
    memset((void *)biosExtendedDataAreaStart, 0xFF, BITS2BYTES(DIV_ROUND_UP(g_LowestUpperUsableAddress - biosExtendedDataAreaStart, MEMORY_ALLOCATOR_CHUNK_SIZE)));

    // do not use unavailable memory regions
    for (uint32_t index = 0; index < g_MemoryRegionsCount; ++index)
        if (g_MemoryRegions[index].type != MEMORY_TYPE_AVAILABLE)
            memset(g_InUseBits + BITS2BYTES(DIV_ROUND_UP(g_MemoryRegions[index].baseAddress, MEMORY_ALLOCATOR_CHUNK_SIZE)), 0xFF, BITS2BYTES(DIV_ROUND_UP(g_MemoryRegions[index].size, MEMORY_ALLOCATOR_CHUNK_SIZE)));
}

// set `lower` to `true` to request memory below 1MB (0x100000)
// - if none is found memory above 1MB might be returned
void *ALLOCATOR_Malloc(size_t size, bool lower) {
    if (size == 0)
        return NULL;

    size_t totalSize = size + CHUNK_HEADER_SIZE;

    size_t holeSize = 0;
    size_t bitIndex = 0;
    if (!lower)
        bitIndex = DIV_ROUND_UP((size_t)g_LowestUpperUsableAddress, MEMORY_ALLOCATOR_CHUNK_SIZE); // start searching in upper memory

    for (; bitIndex < g_InUseBitsSize && (holeSize * MEMORY_ALLOCATOR_CHUNK_SIZE) < totalSize; ++bitIndex)
        if (GET_BIT(g_InUseBits, bitIndex))
            holeSize = 0;
        else
            ++holeSize;

    if (holeSize == 0)
        return NULL;

    bitIndex -= holeSize;

    for (size_t i = 0; i < holeSize; ++i)
        SET_BIT(g_InUseBits, bitIndex + i);

    void *holeSizePtr = (void *)(bitIndex * MEMORY_ALLOCATOR_CHUNK_SIZE);
    *((size_t *)holeSizePtr) = holeSize;

    return holeSizePtr + CHUNK_HEADER_SIZE;
}

void *malloc(size_t size) {
    return ALLOCATOR_Malloc(size, false);
}

void *calloc(size_t count, size_t size) {
    if (count == 0 || size == 0)
        return NULL;

    void *ptr = malloc(count * size);
    if (ptr == NULL)
        return NULL;

    memset(ptr, 0, count * size);

    return ptr;
}

void free(void *ptr) {
    if (ptr == NULL)
        return;

    CHUNK_HEADER_TYPE *holeSizePtr = ptr - 4;
    size_t ptrBitIndex = (size_t)holeSizePtr / MEMORY_ALLOCATOR_CHUNK_SIZE;

    for (size_t bitIndex = ptrBitIndex; bitIndex < (ptrBitIndex + (*holeSizePtr)); ++bitIndex)
        UNSET_BIT(g_InUseBits, bitIndex);
}
