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

#define ROOT_DIRECTORY_HANDLE -1
#define UNUSED_HANDLE -2

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

bool FAT_ReadBootSector(FAT_Filesystem *filesystem) {
    return Partition_ReadSectors(filesystem->partition, 0, 1, filesystem->fatData.bootSector.bootSectorBytes) == 1;
}

bool FAT_ReadFat(FAT_Filesystem *filesystem, uint64_t lbaIndex) {
    bool success = Partition_ReadSectors(filesystem->partition, filesystem->fatData.bootSector.info.reservedSectors + lbaIndex, FAT_CACHE_SIZE_SECTORS, filesystem->fatData.fatCache) == FAT_CACHE_SIZE_SECTORS;

    if (success)
        filesystem->fatData.fatCachePosition = lbaIndex;

    return success;
}

bool FAT_IsFat32(FAT_Filesystem *filesystem) {
    return filesystem->fatData.bootSector.info.sectorsPerFat == 0;
}

uint8_t FAT_DetectFatType(FAT_Filesystem *filesystem) {
    uint32_t dataClusters = (filesystem->partition->partitionSize - filesystem->fatData.dataSectionLba) / filesystem->fatData.bootSector.info.sectorsPerCluster;

    if (dataClusters < 0xFF5)
        return 12;
    else if (filesystem->fatData.bootSector.info.sectorsPerFat != 0) // in fat32 this is always 0
        return 16;
    else
        return 32;
}

bool FAT_Initialize(FAT_Filesystem *filesystem) {
    memset(&filesystem->fatData, 0, sizeof(FAT_Data));

    // read boot sector
    if (!FAT_ReadBootSector(filesystem)) {
        puts("FAT: Failed to read boot sector.\n");
        return false;
    }

    if (FAT_IsFat32(filesystem))
        filesystem->fatData.sectorsPerFat = filesystem->fatData.bootSector.info.ebr32.sectorsPerFat;
    else
        filesystem->fatData.sectorsPerFat = filesystem->fatData.bootSector.info.sectorsPerFat;

    // read root directory
    filesystem->fatData.fatCachePosition = UINT32_MAX;
    uint32_t rootDirectoryLba = filesystem->fatData.bootSector.info.reservedSectors + (filesystem->fatData.sectorsPerFat * filesystem->fatData.bootSector.info.fatCount);
    uint32_t rootDirectorySize = sizeof(FAT_DirectoryEntry) * filesystem->fatData.bootSector.info.dirEntryCount;

    // open root directory file
    filesystem->fatData.rootDirectory.open = true;
    filesystem->fatData.rootDirectory.public.handle = ROOT_DIRECTORY_HANDLE;
    filesystem->fatData.rootDirectory.public.isDirectory = true;
    filesystem->fatData.rootDirectory.public.position = 0;
    filesystem->fatData.rootDirectory.public.size = rootDirectorySize;
    filesystem->fatData.rootDirectory.firstCluster = rootDirectoryLba;
    filesystem->fatData.rootDirectory.currentCluster = rootDirectoryLba;
    filesystem->fatData.rootDirectory.currentSectorInCluster = 0;

    if (Partition_ReadSectors(filesystem->partition, rootDirectoryLba, 1, filesystem->fatData.rootDirectory.buffer) != 1) {
        puts("FAT: Failed to read root directory.\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirectorySectors = (rootDirectorySize + filesystem->fatData.bootSector.info.bytesPerSector - 1) / filesystem->fatData.bootSector.info.bytesPerSector;
    filesystem->fatData.dataSectionLba = rootDirectoryLba + rootDirectorySectors;

    // reset open files
    for (uint32_t i = 0; i < MAX_FILE_HANDLES; ++i)
        filesystem->fatData.openFiles[i].open = false;

    filesystem->fatData.fatType = FAT_DetectFatType(filesystem);

    return true;
}

uint32_t FAT_ClusterToLba(FAT_Filesystem *filesystem, uint32_t cluster) {
    return filesystem->fatData.dataSectionLba + (cluster - 2) * filesystem->fatData.bootSector.info.sectorsPerCluster;
}

FAT_File *FAT_OpenEntry(FAT_Filesystem *filesystem, FAT_DirectoryEntry *entry) {
    int32_t handle = UNUSED_HANDLE;
    for (size_t i = 0; i < MAX_FILE_HANDLES && handle < 0; ++i) {
        if (!filesystem->fatData.openFiles[i].open)
            handle = i;
    }

    if (handle == UNUSED_HANDLE) {
        puts("FAT: Out of file handles.\n");
        return false;
    }

    // set up variables
    FAT_FileData *fd = &filesystem->fatData.openFiles[handle];
    fd->public.handle = handle;
    fd->public.isDirectory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->size;
    fd->firstCluster = entry->firstClusterLow + ((uint32_t)entry->firstClusterHigh << 16);
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    if (Partition_ReadSectors(filesystem->partition, FAT_ClusterToLba(filesystem, fd->currentCluster), 1, fd->buffer) != 1) {
        puts("FAT: Read error.\n");
        return false;
    }

    fd->open = true;
    return &fd->public;
}

// returns UINT32_MAX on error
uint32_t FAT_NextCluster(FAT_Filesystem *filesystem, uint32_t currentCluster) {
    uint32_t fatIndex;

    switch (filesystem->fatData.fatType) {
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
    if (fatIndexSector < filesystem->fatData.fatCachePosition || fatIndexSector >= filesystem->fatData.fatCachePosition + FAT_CACHE_SIZE_SECTORS)
        if (!FAT_ReadFat(filesystem, fatIndexSector)) // cache miss
            return UINT32_MAX;

    fatIndex -= filesystem->fatData.fatCachePosition * SECTOR_SIZE;

    uint32_t nextCluster;
    // comparisons with 0xFF...FF8 are eof checks
    switch (filesystem->fatData.fatType) {
    case 12:
        if (currentCluster % 2 == 0)
            nextCluster = (*(uint16_t *)(filesystem->fatData.fatCache + fatIndex)) & 0x0FFF;
        else
            nextCluster = (*(uint16_t *)(filesystem->fatData.fatCache + fatIndex)) >> 4;

        if (nextCluster >= 0xFF8)
            nextCluster |= 0xFFFFF000;

        break;
    case 16:
        nextCluster = *(uint16_t *)(filesystem->fatData.fatCache + fatIndex);

        if (nextCluster >= 0xFFF8)
            nextCluster |= 0xFFFF0000;

        break;
    case 32:
        nextCluster = *(uint32_t *)(filesystem->fatData.fatCache + fatIndex);
        break;
    default:
        return UINT32_MAX;
    }

    return nextCluster;
}

// - returns `UINT32_MAX` on error
// - returns the new position on success
uint32_t FAT_Seek(FAT_Filesystem *filesystem, FAT_File *file, int64_t targetPosition, uint8_t whence) {
    FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &filesystem->fatData.rootDirectory
                           : &filesystem->fatData.openFiles[file->handle];

    if (whence == FAT_WHENCE_CURSOR)
        targetPosition += fd->public.position;
    if (whence == FAT_WHENCE_END)
        targetPosition += fd->public.size;

    // out of bounds
    if (targetPosition < 0) {
        puts("FAT: Failed to seek, position is negative.\n");
        return UINT32_MAX;
    }

    if (targetPosition > fd->public.size) {
        puts("FAT: Failed to seek, position is past EOF.\n");
        return UINT32_MAX;
    }

    // set file info to beggining
    fd->public.position = 0;
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    // short path if seeking to the beginning
    if (targetPosition == 0) {
        // load the first sector into buffer
        uint16_t sectorsRead = Partition_ReadSectors(filesystem->partition, FAT_ClusterToLba(filesystem, fd->currentCluster), 1, fd->buffer);
        if (sectorsRead != 1) {
            puts("FAT: Failed to seek, read error.\n");
            return UINT32_MAX;
        }
        return targetPosition;
    }

    // calculate cluster and sector to reach position
    uint32_t clusterSize = SECTOR_SIZE * filesystem->fatData.bootSector.info.sectorsPerCluster;
    uint32_t clustersToAdvance = targetPosition / clusterSize;
    uint32_t remainingBytes = targetPosition % clusterSize;
    uint32_t sectorInCluster = remainingBytes / SECTOR_SIZE;

    // follow cluster chain
    while (clustersToAdvance-- > 0) {
        uint32_t nextCluster = FAT_NextCluster(filesystem, fd->currentCluster);
        if (nextCluster == UINT32_MAX)
            return UINT32_MAX; // error
        fd->currentCluster = nextCluster;
    }

    fd->currentSectorInCluster = sectorInCluster;
    fd->public.position = targetPosition;

    // coad the sector into the buffer
    uint32_t lba = FAT_ClusterToLba(filesystem, fd->currentCluster) + fd->currentSectorInCluster;
    if (Partition_ReadSectors(filesystem->partition, lba, 1, fd->buffer) != 1) {
        puts("FAT: Failed to seek, read error.\n");
        return UINT32_MAX;
    }

    return targetPosition;
}

uint32_t FAT_Read(FAT_Filesystem *filesystem, FAT_File *file, uint32_t byteCount, void *dataOutput) {
    FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &filesystem->fatData.rootDirectory
                           : &filesystem->fatData.openFiles[file->handle];

    if (dataOutput == NULL)
        return 0;

    uint8_t *u8DataOutput = (uint8_t *)dataOutput;

    if (!fd->public.isDirectory || (fd->public.isDirectory && fd->public.size != 0))
        byteCount = min(byteCount, fd->public.size - fd->public.position);

    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(u8DataOutput, fd->buffer + fd->public.position % SECTOR_SIZE, take);

        u8DataOutput += take;
        fd->public.position += take;
        byteCount -= take;

        if (leftInBuffer == take) {
            // special handling for root directory
            if (fd->public.handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->currentCluster;

                if (Partition_ReadSectors(filesystem->partition, fd->currentCluster, 1, fd->buffer) != 1) {
                    puts("FAT: Failed to read next sector.\n");
                    break;
                }
            } else {
                if (++fd->currentSectorInCluster >= filesystem->fatData.bootSector.info.sectorsPerCluster) {
                    fd->currentSectorInCluster = 0;
                    uint32_t nextCluster = FAT_NextCluster(filesystem, fd->currentCluster);
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

                if (Partition_ReadSectors(filesystem->partition, FAT_ClusterToLba(filesystem, fd->currentCluster) + fd->currentSectorInCluster, 1, fd->buffer) != 1) {
                    puts("FAT: Failed to read next sector.\n");
                    break;
                }
            }
        }
    }

    return u8DataOutput - (uint8_t *)dataOutput;
}

bool FAT_ReadEntry(FAT_Filesystem *filesystem, FAT_File *file, FAT_DirectoryEntry *directoryEntryOutput) {
    return FAT_Read(filesystem, file, sizeof(FAT_DirectoryEntry), directoryEntryOutput) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_Filesystem *filesystem, FAT_File *file) {
    if (file->handle == ROOT_DIRECTORY_HANDLE) {
        file->position = 0;
        filesystem->fatData.rootDirectory.currentCluster = filesystem->fatData.rootDirectory.firstCluster;
    } else {
        filesystem->fatData.openFiles[file->handle].open = false;
    }
}

bool FAT_FindFile(FAT_Filesystem *filesystem, FAT_File *file, const char *name, FAT_DirectoryEntry *entryOutput) {
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

    while (FAT_ReadEntry(filesystem, file, &entry)) {
        if (memcmp(fatName, entry.name, 11) == 0) {
            *entryOutput = entry;
            return true;
        }
    }

    return false;
}

// returns NULL on error
FAT_File *FAT_Open(FAT_Filesystem *filesystem, const char *path) {
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        ++path;

    FAT_File *current = &filesystem->fatData.rootDirectory.public;

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
        if (FAT_FindFile(filesystem, current, name, &entry)) {
            FAT_Close(filesystem, current);

            if (!isLast && ((entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0)) {
                printf("FAT: %s not a directory\n", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_OpenEntry(filesystem, &entry);
        } else {
            FAT_Close(filesystem, current);

            printf("FAT: %s not found.\n", name);
            return NULL;
        }
    }

    return current;
}
