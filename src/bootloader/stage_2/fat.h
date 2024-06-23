#pragma once
#include "disk.h"
#include "stdbool.h"
#include "stdint.h"

#pragma pack(push, 1)
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
} FAT_DirectoryEntry;
#pragma pack(pop)

typedef struct
{
    ssize_t handle;
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

bool FAT_Initialize(DISK *disk);
FAT_File __far *FAT_Open(DISK *disk, const char *path);
uint32_t FAT_Read(DISK *disk, FAT_File __far *file, uint32_t byteCount, void *dataOut);
bool FAT_ReadEntry(DISK *disk, FAT_File __far *file, FAT_DirectoryEntry *directoryEntryOutput);
void FAT_Close(FAT_File __far *file);
