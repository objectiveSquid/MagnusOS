#include "allocator.h"
#include "memdefs.h"
#include "util/memory.h"
#include "util/other.h"
#include <stddef.h>

#define CHUNK_HEADER_TYPE size_t
#define CHUNK_HEADER_SIZE sizeof(CHUNK_HEADER_TYPE)

static uint8_t *g_InUseBits = (uint8_t *)MEMORY_ALLOCATOR_IN_USE_BITS;
static size_t g_LowestUsableAddress;
static uint64_t g_InUseBitsSize; // on 64 bit systems this might actually exceeed the 64 bit unsigned integer limit
static const MEMDETECT_MemoryRegion *g_MemoryRegions;
static size_t g_MemoryRegionsCount;

void ALLOCATOR_Initialize(const MEMDETECT_MemoryRegion *memoryRegions, uint32_t memoryRegionsCount) {
    const MEMDETECT_MemoryRegion *highestRegion = &memoryRegions[0];
    for (uint64_t index = 0; index < memoryRegionsCount; ++index)
        if (memoryRegions[index].baseAddress > highestRegion->baseAddress)
            highestRegion = &memoryRegions[index];

    g_InUseBitsSize = (highestRegion->baseAddress + highestRegion->size) / MEMORY_ALLOCATOR_CHUNK_SIZE;

    // addresses below this aren't for ram
    g_LowestUsableAddress = ((size_t)MEMORY_ALLOCATOR_IN_USE_BITS) + BITS2BYTES(g_InUseBitsSize);

    // clear in use bits
    memset(g_InUseBits, 0x00, BITS2BYTES(g_InUseBitsSize));
    // do not use low memory
    memset(g_InUseBits, 0xFF, BITS2BYTES(DIV_ROUND_UP(g_LowestUsableAddress, MEMORY_ALLOCATOR_CHUNK_SIZE)));
    // do not use unavailable memory regions
    for (uint32_t index = 0; index < memoryRegionsCount; ++index)
        if (memoryRegions[index].type != MEMORY_TYPE_AVAILABLE)
            memset(g_InUseBits + BITS2BYTES(DIV_ROUND_UP(memoryRegions[index].baseAddress, MEMORY_ALLOCATOR_CHUNK_SIZE)), 0xFF, BITS2BYTES(DIV_ROUND_UP(memoryRegions[index].size, MEMORY_ALLOCATOR_CHUNK_SIZE)));

    // set globals
    g_MemoryRegions = memoryRegions;
    g_MemoryRegionsCount = memoryRegionsCount;
}

void *malloc(size_t size) {
    if (size == 0)
        return NULL;

    size_t totalSize = size + CHUNK_HEADER_SIZE;

    size_t holeSize = 0;
    size_t bitIndex = 0;
    for (; bitIndex < g_InUseBitsSize && (holeSize * MEMORY_ALLOCATOR_CHUNK_SIZE) < totalSize; ++bitIndex)
        if (GET_BIT(g_InUseBits, bitIndex))
            holeSize = 0;
        else
            ++holeSize;

    if (holeSize == 0)
        return NULL;

    bitIndex -= holeSize;

    for (size_t i = 0; i < holeSize; ++i) {
        printf("Setting bit %d\n", bitIndex + i);
        SET_BIT(g_InUseBits, bitIndex + i);
    }

    void *holeSizePtr = (void *)(bitIndex * MEMORY_ALLOCATOR_CHUNK_SIZE);
    *((size_t *)holeSizePtr) = holeSize;

    return holeSizePtr + CHUNK_HEADER_SIZE;
}

void free(void *ptr) {
    CHUNK_HEADER_TYPE *holeSizePtr = ptr - 4;
    size_t ptrBitIndex = (size_t)holeSizePtr / MEMORY_ALLOCATOR_CHUNK_SIZE;

    for (size_t bitIndex = ptrBitIndex; bitIndex < (ptrBitIndex + (*holeSizePtr)); ++bitIndex)
        UNSET_BIT(g_InUseBits, bitIndex);
}
