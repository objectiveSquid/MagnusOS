#include "fat.h"
#include "mbr.h"
#include "memdefs.h"
#include "memory/allocator.h"
#include "util/memory.h"
#include "util/other.h"
#include "util/string.h"
#include "visual/stdio.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 32
#define FAT_CACHE_SIZE_SECTORS 5 // 5 sectors

#define ROOT_DIRECTORY_HANDLE -1
#define UNUSED_HANDLE -2

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
        FAT_BootSector bootSector;
        char bootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData rootDirectory;

    FAT_FileData openFiles[MAX_FILE_HANDLES];

    uint8_t fatCache[FAT_CACHE_SIZE_SECTORS * SECTOR_SIZE];
    uint32_t fatCachePosition;
} FAT_Data;

static FAT_Data *g_Data;
static uint32_t g_DataSectionLba;
static uint8_t g_FatType;
static uint32_t g_SectorsPerFat;

bool FAT_ReadBootSector(Partition *partition) {
    return Partition_ReadSectors(partition, 0, 1, g_Data->BS.bootSectorBytes) == 1;
}

bool FAT_ReadFat(Partition *partition, uint64_t lbaIndex) {
    bool success = Partition_ReadSectors(partition, g_Data->BS.bootSector.reservedSectors + lbaIndex, FAT_CACHE_SIZE_SECTORS, g_Data->fatCache) == FAT_CACHE_SIZE_SECTORS;

    if (success)
        g_Data->fatCachePosition = lbaIndex;

    return success;
}

bool FAT_IsFat32() {
    return g_Data->BS.bootSector.sectorsPerFat == 0;
}

uint8_t FAT_DetectFatType(Partition *partition) {
    uint32_t dataClusters = (partition->partitionSize - g_DataSectionLba) / g_Data->BS.bootSector.sectorsPerCluster;

    if (dataClusters < 0xFF5)
        return 12;
    else if (g_Data->BS.bootSector.sectorsPerFat != 0) // in fat32 this is always 0
        return 16;
    else
        return 32;
}

void FAT_DeInitialize() {
    free(g_Data);
    g_Data = NULL;
}

bool FAT_Initialize(Partition *partition, bool isKernel) {
    if (isKernel)
        g_Data = (FAT_Data *)malloc(sizeof(FAT_Data));
    else
        g_Data = (FAT_Data *)ALLOCATOR_Malloc(sizeof(FAT_Data), true);

    memset(g_Data, 0, sizeof(FAT_Data));

    // read boot sector
    if (!FAT_ReadBootSector(partition)) {
        puts("FAT: Failed to read boot sector.\n");
        return false;
    }

    if (FAT_IsFat32())
        g_SectorsPerFat = g_Data->BS.bootSector.ebr32.sectorsPerFat;
    else
        g_SectorsPerFat = g_Data->BS.bootSector.sectorsPerFat;

    // read root directory
    g_Data->fatCachePosition = UINT32_MAX;
    uint32_t rootDirectoryLba = g_Data->BS.bootSector.reservedSectors + (g_SectorsPerFat * g_Data->BS.bootSector.fatCount);
    uint32_t rootDirectorySize = sizeof(FAT_DirectoryEntry) * g_Data->BS.bootSector.dirEntryCount;

    // open root directory file
    g_Data->rootDirectory.open = true;
    g_Data->rootDirectory.public.handle = ROOT_DIRECTORY_HANDLE;
    g_Data->rootDirectory.public.isDirectory = true;
    g_Data->rootDirectory.public.position = 0;
    g_Data->rootDirectory.public.size = rootDirectorySize;
    g_Data->rootDirectory.firstCluster = rootDirectoryLba;
    g_Data->rootDirectory.currentCluster = rootDirectoryLba;
    g_Data->rootDirectory.currentSectorInCluster = 0;

    if (Partition_ReadSectors(partition, rootDirectoryLba, 1, g_Data->rootDirectory.buffer) != 1) {
        puts("FAT: Failed to read root directory.\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirectorySectors = (rootDirectorySize + g_Data->BS.bootSector.bytesPerSector - 1) / g_Data->BS.bootSector.bytesPerSector;
    g_DataSectionLba = rootDirectoryLba + rootDirectorySectors;

    // reset open files
    for (uint32_t i = 0; i < MAX_FILE_HANDLES; ++i)
        g_Data->openFiles[i].open = false;

    g_FatType = FAT_DetectFatType(partition);

    return true;
}

uint32_t FAT_ClusterToLba(uint32_t cluster) {
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.bootSector.sectorsPerCluster;
}

FAT_File *FAT_OpenEntry(Partition *partition, FAT_DirectoryEntry *entry) {
    int32_t handle = UNUSED_HANDLE;
    for (size_t i = 0; i < MAX_FILE_HANDLES && handle < 0; ++i) {
        if (!g_Data->openFiles[i].open)
            handle = i;
    }

    if (handle == UNUSED_HANDLE) {
        puts("FAT: Out of file handles.\n");
        return false;
    }

    // set up variables
    FAT_FileData *fd = &g_Data->openFiles[handle];
    fd->public.handle = handle;
    fd->public.isDirectory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->size;
    fd->firstCluster = entry->firstClusterLow + ((uint32_t)entry->firstClusterHigh << 16);
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    if (Partition_ReadSectors(partition, FAT_ClusterToLba(fd->currentCluster), 1, fd->buffer) != 1) {
        puts("FAT: Read error.\n");
        return false;
    }

    fd->open = true;
    return &fd->public;
}

// returns UINT32_MAX on error
uint32_t FAT_NextCluster(Partition *partition, uint32_t currentCluster) {
    uint32_t fatIndex;

    switch (g_FatType) {
    case 12:
        fatIndex = currentCluster * 3 / 2;
        break;
    case 16:
        fatIndex = currentCluster * 2;
        break;
    case 32:
        fatIndex = currentCluster * 4;
        break;
    default:
        return UINT32_MAX;
    }

    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if (fatIndexSector < g_Data->fatCachePosition || fatIndexSector >= g_Data->fatCachePosition + FAT_CACHE_SIZE_SECTORS)
        if (!FAT_ReadFat(partition, fatIndexSector)) // cache miss
            return UINT32_MAX;

    fatIndex -= g_Data->fatCachePosition * SECTOR_SIZE;

    uint32_t nextCluster;
    // comparisons with 0xFF...FF8 are eof checks
    switch (g_FatType) {
    case 12:
        if (currentCluster % 2 == 0)
            nextCluster = (*(uint16_t *)(g_Data->fatCache + fatIndex)) & 0x0FFF;
        else
            nextCluster = (*(uint16_t *)(g_Data->fatCache + fatIndex)) >> 4;

        if (nextCluster >= 0xFF8)
            nextCluster |= 0xFFFFF000;

        break;
    case 16:
        nextCluster = *(uint16_t *)(g_Data->fatCache + fatIndex);

        if (nextCluster >= 0xFFF8)
            nextCluster |= 0xFFFF0000;

        break;
    case 32:
        nextCluster = *(uint32_t *)(g_Data->fatCache + fatIndex);
        break;
    default:
        return UINT32_MAX;
    }

    return nextCluster;
}

// if dataOutput is NULL, it is ignored and this function is pretty much just a seek function
uint32_t FAT_Read(Partition *partition, FAT_File *file, uint32_t byteCount, void *dataOutput) {
    FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &g_Data->rootDirectory
                           : &g_Data->openFiles[file->handle];

    uint8_t *u8DataOutput = (uint8_t *)dataOutput;

    if (!fd->public.isDirectory || (fd->public.isDirectory && fd->public.size != 0))
        byteCount = min(byteCount, fd->public.size - fd->public.position);

    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        if (dataOutput != NULL)
            memcpy(u8DataOutput, fd->buffer + fd->public.position % SECTOR_SIZE, take);

        u8DataOutput += take;
        fd->public.position += take;
        byteCount -= take;

        if (leftInBuffer == take) {
            // special handling for root directory
            if (fd->public.handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->currentCluster;

                if (Partition_ReadSectors(partition, fd->currentCluster, 1, fd->buffer) != 1) {
                    puts("FAT: Failed to read next sector.\n");
                    break;
                }
            } else {
                if (++fd->currentSectorInCluster >= g_Data->BS.bootSector.sectorsPerCluster) {
                    fd->currentSectorInCluster = 0;
                    uint32_t nextCluster = FAT_NextCluster(partition, fd->currentCluster);
                    if (nextCluster == UINT32_MAX) {
                        puts("FAT: Failed to get next cluster.\n");
                        break;
                    }
                    fd->currentCluster = nextCluster;
                }

                // eof
                if (fd->currentCluster >= 0xFFFFFFF8) {
                    fd->public.size = fd->public.position;
                    break;
                }

                if (Partition_ReadSectors(partition, FAT_ClusterToLba(fd->currentCluster) + fd->currentSectorInCluster, 1, fd->buffer) != 1) {
                    puts("FAT: Failed to read next sector.\n");
                    break;
                }
            }
        }
    }

    return u8DataOutput - (uint8_t *)dataOutput;
}

bool FAT_ReadEntry(Partition *partition, FAT_File *file, FAT_DirectoryEntry *directoryEntryOutput) {
    return FAT_Read(partition, file, sizeof(FAT_DirectoryEntry), directoryEntryOutput) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File *file) {
    if (file->handle == ROOT_DIRECTORY_HANDLE) {
        file->position = 0;
        g_Data->rootDirectory.currentCluster = g_Data->rootDirectory.firstCluster;
    } else {
        g_Data->openFiles[file->handle].open = false;
    }
}

bool FAT_FindFile(Partition *partition, FAT_File *file, const char *name, FAT_DirectoryEntry *entryOutput) {
    char fatName[12];
    FAT_DirectoryEntry entry;

    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char *extension = strchr(name, '.');
    if (extension == NULL)
        extension = name + 11;

    for (uint8_t i = 0; i < 8 && name[i] && name + i < extension; ++i)
        fatName[i] = toUpper(name[i]);

    if (extension != name + 11)
        for (uint8_t i = 0; i < 3 && extension[i + 1]; ++i)
            fatName[i + 8] = toUpper(extension[i + 1]);

    while (FAT_ReadEntry(partition, file, &entry)) {
        if (memcmp(fatName, entry.name, 11) == 0) {
            *entryOutput = entry;
            return true;
        }
    }

    return false;
}

// returns NULL on error
FAT_File *FAT_Open(Partition *partition, const char *path) {
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        ++path;

    FAT_File *current = &g_Data->rootDirectory.public;

    while (*path) {
        bool isLast = false;

        const char *splitted = strchr(path, '/');
        if (splitted != NULL) {
            memcpy(name, path, splitted - path);
            name[splitted - path] = '\0';
            path = splitted + 1;
        } else {
            size_t length = strlen(path);
            memcpy(name, path, length);
            name[length] = '\0';
            path += length;
            isLast = true;
        }

        FAT_DirectoryEntry entry;
        if (FAT_FindFile(partition, current, name, &entry)) {
            FAT_Close(current);

            if (!isLast && ((entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)) {
                printf("FAT: %s not a directory\n", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_OpenEntry(partition, &entry);
        } else {
            FAT_Close(current);

            printf("FAT: %s not found.\n", name);
            return NULL;
        }
    }

    return current;
}
