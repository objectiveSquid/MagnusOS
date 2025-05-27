#pragma once

#include "mbr.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char name[11];
    uint8_t attributes;
    uint8_t _Reserved;
    uint8_t createdTimeTenths;
    uint16_t createdTime;
    uint16_t createdDate;
    uint16_t accessedDate;
    uint16_t firstClusterHigh;
    uint16_t modifiedTime;
    uint16_t modifiedDate;
    uint16_t firstClusterLow;
    uint32_t size;
} __attribute__((packed)) FAT_DirectoryEntry;

typedef struct {
    uint32_t handle;
    bool isDirectory;
    uint32_t position;
    uint32_t size;
} FAT_File;

enum FAT_Attributes {
    FAT_ATTRIBUTE_READ_ONLY = 0x01,
    FAT_ATTRIBUTE_HIDDEN = 0x02,
    FAT_ATTRIBUTE_SYSTEM = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID = 0x08,
    FAT_ATTRIBUTE_DIRECTORY = 0x10,
    FAT_ATTRIBUTE_ARCHIVE = 0x20,
    FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
};

bool FAT_Initialize(Partition *partition);
FAT_File *FAT_Open(Partition *partition, const char *path);
uint32_t FAT_Read(Partition *partition, FAT_File *file, uint32_t byteCount, void *dataOutput);
bool FAT_ReadEntry(Partition *partition, FAT_File *file, FAT_DirectoryEntry *directoryEntryOutput);
void FAT_Close(FAT_File *file);
