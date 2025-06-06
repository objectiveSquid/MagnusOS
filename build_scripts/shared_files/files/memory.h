#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEMORY_SEGMENT(segment_offset) (segment_offset >> 16)
#define MEMORY_OFFSET(segment_offset) (segment_offset & 0xFF)
#define MEMORY_SEGMENT_OFFSET_TO_LINEAR(segment_offset) ((MEMORY_SEGMENT(segment_offset) << 4) + MEMORY_OFFSET(segment_offset))

void memcpy(void *destination, const void *source, size_t amount);
void memset(void *pointer, char value, size_t amount);
int memcmp(const void *pointer1, const void *pointer2, size_t amount);
