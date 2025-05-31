#pragma once

#include "mbr.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// everything except for FAT_File i dont like exposing, but for now, i'll put them here because FAT_Data needs them
#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 32
#define FAT_CACHE_SIZE_SECTORS 5

typedef struct {
    uint32_t handle;
    bool isDirectory;
    uint32_t position;
    uint32_t size;
} FAT_File;

typedef enum {
    FAT_ATTRIBUTE_READ_ONLY = 0x01,
    FAT_ATTRIBUTE_HIDDEN = 0x02,
    FAT_ATTRIBUTE_SYSTEM = 0x04,
    FAT_ATTRIBUTE_VOLUME_ID = 0x08,
    FAT_ATTRIBUTE_DIRECTORY = 0x10,
    FAT_ATTRIBUTE_ARCHIVE = 0x20,
    FAT_ATTRIBUTE_LFN = FAT_ATTRIBUTE_READ_ONLY | FAT_ATTRIBUTE_HIDDEN | FAT_ATTRIBUTE_SYSTEM | FAT_ATTRIBUTE_VOLUME_ID
} FAT_Attributes;

typedef enum {
    FAT_WHENCE_SET = 0,
    FAT_WHENCE_CURSOR = 1, // aka CUR
    FAT_WHENCE_END = 2,
} FAT_Whence;

typedef struct {
    // extended boot record (marked with ebr_ in boot.asm)
    uint8_t driveNumber;
    uint8_t _reserved;
    uint8_t signature;
    uint32_t volumeId;       // serial number, value doesn't matter
    uint8_t volumeLabel[11]; // 11 bytes, padded with spaces
    uint8_t systemId[8];
} __attribute__((packed)) FAT_ExtendedBootRecord;

typedef struct {
    // fat32 extended boot record (marked with fat32_ in boot.asm)
    uint32_t sectorsPerFat;
    uint16_t flags;
    uint16_t fatVersion;
    uint32_t rootDirectoryCluster;
    uint16_t FSInfoSector;
    uint16_t backupBootSector;
    uint8_t _reserved[12];
    // normal extended boot record (marked with ebr_ in boot.asm)
    FAT_ExtendedBootRecord EBR;

} __attribute((packed)) FAT32_ExtendedBootRecord;

typedef struct {
    uint8_t bootJumpInstruction[3];
    uint8_t oemIdentifier[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectors;
    uint8_t fatCount;
    uint16_t dirEntryCount;
    uint16_t totalSectors;
    uint8_t mediaDescriptorType;
    uint16_t sectorsPerFat;
    uint16_t sectorsPerTrack;
    uint16_t heads;
    uint32_t hiddenSectors;
    uint32_t largeSectorCount;

    union {
        FAT_ExtendedBootRecord ebr12and16;
        FAT32_ExtendedBootRecord ebr32;
    };
} __attribute__((packed)) FAT_BootSector;

typedef struct {
    char buffer[SECTOR_SIZE];
    FAT_File public;
    bool open;
    uint32_t firstCluster;
    uint32_t currentCluster;
    uint32_t currentSectorInCluster;
} FAT_FileData;

typedef struct {
    union {
        FAT_BootSector info;
        uint8_t bootSectorBytes[SECTOR_SIZE];
    } bootSector;

    FAT_FileData rootDirectory;

    FAT_FileData openFiles[MAX_FILE_HANDLES];

    uint8_t fatCache[FAT_CACHE_SIZE_SECTORS * SECTOR_SIZE];
    uint32_t fatCachePosition;

    uint32_t dataSectionLba;
    uint8_t fatType;
    uint32_t sectorsPerFat;
} FAT_Data;

typedef struct {
    Partition *partition;
    FAT_Data fatData;
} FAT_Filesystem;

bool FAT_Initialize(FAT_Filesystem *filesystem);
FAT_File *FAT_Open(FAT_Filesystem *filesystem, const char *path);
uint32_t FAT_Seek(FAT_Filesystem *filesystem, FAT_File *file, int64_t targetPosition, uint8_t whence);
uint32_t FAT_Read(FAT_Filesystem *filesystem, FAT_File *file, uint32_t byteCount, void *dataOutput);
void FAT_Close(FAT_Filesystem *filesystem, FAT_File *file);
