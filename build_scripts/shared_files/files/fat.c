#include "fat.h"
#include "visual/stdio.h"
#include <lib/algorithm/ascii.h>
#include <lib/algorithm/math.h>
#include <lib/algorithm/string.h>
#include <lib/disk/mbr.h>
#include <lib/errors/errors.h>
#include <lib/memory/allocator.h>
#include <lib/memory/memdefs.h>
#include <lib/memory/memory.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ROOT_DIRECTORY_HANDLE -1
#define UNUSED_HANDLE -2
const char *FAT_ALLOWED_ASCII_CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&'()-@^_`{}~ ";

int FAT_ReadBootSector(FAT_Filesystem *filesystem) {
    if (filesystem == NULL)
        return NULL_ERROR;

    return Partition_ReadSectors(filesystem->partition, 0, 1, NULL, filesystem->fatData.bootSector.bootSectorBytes);
}

int FAT_ReadFat(FAT_Filesystem *filesystem, uint64_t lbaIndex) {
    if (filesystem == NULL)
        return NULL_ERROR;

    int status = Partition_ReadSectors(filesystem->partition, filesystem->fatData.bootSector.info.reservedSectors + lbaIndex, FAT_CACHE_SIZE_SECTORS, NULL, filesystem->fatData.fatCache);

    if (status == NO_ERROR)
        filesystem->fatData.fatCachePosition = lbaIndex;

    return status;
}

int FAT_IsFat32(FAT_Filesystem *filesystem, bool *fatTypeOutput) {
    if (filesystem == NULL)
        return NULL_ERROR;

    *fatTypeOutput = filesystem->fatData.bootSector.info.sectorsPerFat == 0;

    return NO_ERROR;
}

int FAT_DetectFatType(FAT_Filesystem *filesystem, uint8_t *fatTypeOutput) {
    if (filesystem == NULL)
        return NULL_ERROR;

    uint32_t dataClusters = (filesystem->partition->partitionSize - filesystem->fatData.dataSectionLba) / filesystem->fatData.bootSector.info.sectorsPerCluster;

    if (dataClusters < 0xFF5)
        *fatTypeOutput = 12;
    else if (filesystem->fatData.bootSector.info.sectorsPerFat != 0) // in fat32 this is always 0
        *fatTypeOutput = 16;
    else
        *fatTypeOutput = 32;

    return NO_ERROR;
}

int FAT_Initialize(FAT_Filesystem *filesystem) {
    if (filesystem == NULL)
        return NULL_ERROR;

    memset(&filesystem->fatData, 0, sizeof(FAT_Data));

    // read boot sector
    int status;

    if ((status = FAT_ReadBootSector(filesystem)) != NO_ERROR)
        return status;

    // fat32 stores sectors per fat elsewhere than fat12 and fat16
    bool isFat32;
    if ((status = FAT_IsFat32(filesystem, &isFat32)) != NO_ERROR)
        return status;
    if (isFat32)
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

    // read root directory
    if ((status = Partition_ReadSectors(filesystem->partition, rootDirectoryLba, 1, NULL, filesystem->fatData.rootDirectory.buffer)) != NO_ERROR)
        return status;

    // calculate data section
    uint32_t rootDirectorySectors = (rootDirectorySize + filesystem->fatData.bootSector.info.bytesPerSector - 1) / filesystem->fatData.bootSector.info.bytesPerSector;
    filesystem->fatData.dataSectionLba = rootDirectoryLba + rootDirectorySectors;

    // reset open files
    for (uint32_t i = 0; i < MAX_FILE_HANDLES; ++i)
        filesystem->fatData.openFiles[i].open = false;

    if ((status = FAT_DetectFatType(filesystem, &filesystem->fatData.fatType)) != NO_ERROR)
        return status;

    return NO_ERROR;
}

int FAT_ClusterToLba(FAT_Filesystem *filesystem, uint32_t cluster, uint32_t *lbaOutput) {
    if (filesystem == NULL)
        return NULL_ERROR;

    *lbaOutput = filesystem->fatData.dataSectionLba + (cluster - 2) * filesystem->fatData.bootSector.info.sectorsPerCluster;

    return NO_ERROR;
}

int FAT_OpenEntry(FAT_Filesystem *filesystem, FAT_DirectoryEntry *entry, FAT_File **fileOutput) {
    if (filesystem == NULL || entry == NULL)
        return NULL_ERROR;

    int32_t handle = UNUSED_HANDLE;
    for (size_t i = 0; i < MAX_FILE_HANDLES && handle < 0; ++i) {
        if (!filesystem->fatData.openFiles[i].open)
            handle = i;
    }

    if (handle == UNUSED_HANDLE)
        return FILESYSTEM_OUT_OF_HANDLES_ERROR;

    // set up variables
    FAT_FileData *fd = &filesystem->fatData.openFiles[handle];
    fd->public.handle = handle;
    fd->public.isDirectory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->size;
    fd->firstCluster = entry->firstClusterLow + ((uint32_t)entry->firstClusterHigh << 16);
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    uint32_t newEntryLba;
    int status;
    if ((status = FAT_ClusterToLba(filesystem, fd->firstCluster, &newEntryLba)) != NO_ERROR)
        return status;
    if ((status = Partition_ReadSectors(filesystem->partition, newEntryLba, 1, NULL, fd->buffer)) != NO_ERROR)
        return status;

    fd->open = true;
    *fileOutput = &fd->public;

    return NO_ERROR;
}

int FAT_NextCluster(FAT_Filesystem *filesystem, uint32_t currentCluster, uint32_t *nextClusterOutput) {
    if (filesystem == NULL)
        return NULL_ERROR;

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
        return FILESYSTEM_INTERNAL_ERROR;
    }

    int status;
    uint32_t fatIndexSector = fatIndex / SECTOR_SIZE;
    if (fatIndexSector < filesystem->fatData.fatCachePosition || fatIndexSector >= filesystem->fatData.fatCachePosition + FAT_CACHE_SIZE_SECTORS) {
        if ((status = FAT_ReadFat(filesystem, fatIndexSector)) != NO_ERROR) // cache miss
            return status;
    }

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
        return FILESYSTEM_INTERNAL_ERROR;
    }

    *nextClusterOutput = nextCluster;

    return NO_ERROR;
}

int FAT_Seek(FAT_Filesystem *filesystem, FAT_File *file, int64_t targetPosition, uint8_t whence) {
    if (filesystem == NULL || file == NULL)
        return NULL_ERROR;

    FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &filesystem->fatData.rootDirectory
                           : &filesystem->fatData.openFiles[file->handle];

    if (!fd->open) {
        puts("FAT: File is not open.\n");
        return FILESYSTEM_NOT_OPEN_ERROR;
    }

    if (whence == FAT_WHENCE_CURSOR)
        targetPosition += fd->public.position;
    if (whence == FAT_WHENCE_END)
        targetPosition += fd->public.size;

    // out of bounds
    if (targetPosition < 0) {
        puts("FAT: Failed to seek, position is negative.\n");
        return FILESYSTEM_SEEK_ERROR;
    }

    if (targetPosition > fd->public.size) {
        puts("FAT: Failed to seek, position is past EOF.\n");
        return FILESYSTEM_SEEK_ERROR;
    }

    // set file info to beggining
    fd->public.position = 0;
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    // if seeking to the beginning of the file
    int status;
    uint32_t newLba;
    if (targetPosition == 0) {
        // load the first sector into buffer
        status = FAT_ClusterToLba(filesystem, fd->currentCluster, &newLba);
        return Partition_ReadSectors(filesystem->partition, newLba + fd->currentSectorInCluster, 1, NULL, fd->buffer);
    }

    // calculate cluster and sector to reach position
    uint32_t clusterSize = SECTOR_SIZE * filesystem->fatData.bootSector.info.sectorsPerCluster;
    uint32_t clustersToAdvance = targetPosition / clusterSize;
    uint32_t remainingBytes = targetPosition % clusterSize;
    uint32_t sectorInCluster = remainingBytes / SECTOR_SIZE;

    // follow cluster chain
    while (clustersToAdvance-- > 0) {
        if ((status = FAT_NextCluster(filesystem, fd->currentCluster, &fd->currentCluster)) != NO_ERROR)
            return status;
    }

    fd->currentSectorInCluster = sectorInCluster;
    fd->public.position = targetPosition;

    // coad the sector into the buffer
    if ((status = FAT_ClusterToLba(filesystem, fd->currentCluster, &newLba)) != NO_ERROR)
        return status;
    if ((status = Partition_ReadSectors(filesystem->partition, newLba + fd->currentSectorInCluster, 1, NULL, fd->buffer)) != NO_ERROR)
        return status;

    return NO_ERROR;
}

int FAT_Read(FAT_Filesystem *filesystem, FAT_File *file, uint32_t byteCount, uint32_t *readCountOutput, void *dataOutput) {
    if (filesystem == NULL || file == NULL || dataOutput == NULL)
        return NULL_ERROR;

    FAT_FileData *fd = (file->handle == ROOT_DIRECTORY_HANDLE)
                           ? &filesystem->fatData.rootDirectory
                           : &filesystem->fatData.openFiles[file->handle];

    uint8_t *u8DataOutput = (uint8_t *)dataOutput;

    if (!fd->public.isDirectory || (fd->public.isDirectory && fd->public.size != 0))
        byteCount = min(byteCount, fd->public.size - fd->public.position);

    int status;
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

                if ((status = Partition_ReadSectors(filesystem->partition, fd->currentCluster, 1, NULL, fd->buffer)) != NO_ERROR)
                    break;
            } else {
                if (++fd->currentSectorInCluster >= filesystem->fatData.bootSector.info.sectorsPerCluster) {
                    fd->currentSectorInCluster = 0;
                    if (FAT_NextCluster(filesystem, fd->currentCluster, &fd->currentCluster) != NO_ERROR)
                        break;
                }

                // eof
                if (fd->currentCluster >= 0xFFFFFFF8) {
                    fd->public.size = fd->public.position;
                    break;
                }

                uint32_t lba;
                if ((status = FAT_ClusterToLba(filesystem, fd->currentCluster, &lba)) != NO_ERROR)
                    break;

                if ((status = Partition_ReadSectors(filesystem->partition, lba + fd->currentSectorInCluster, 1, NULL, fd->buffer)) != NO_ERROR)
                    break;
            }
        }
    }

    if (readCountOutput != NULL)
        *readCountOutput = u8DataOutput - (uint8_t *)dataOutput;

    return NO_ERROR;
}

int FAT_ReadEntry(FAT_Filesystem *filesystem, FAT_File *file, FAT_DirectoryEntry *directoryEntryOutput) {
    if (filesystem == NULL || directoryEntryOutput == NULL || file == NULL)
        return NULL_ERROR;

    uint32_t readCount;
    int status = FAT_Read(filesystem, file, sizeof(FAT_DirectoryEntry), &readCount, directoryEntryOutput);

    if (readCount != sizeof(FAT_DirectoryEntry))
        status = FILESYSTEM_EOF_WARNING;

    return status;
}

int FAT_Close(FAT_Filesystem *filesystem, FAT_File *file) {
    if (filesystem == NULL || file == NULL)
        return NULL_ERROR;

    if (file->handle == ROOT_DIRECTORY_HANDLE) {
        file->position = 0;
        filesystem->fatData.rootDirectory.currentCluster = filesystem->fatData.rootDirectory.firstCluster;
    } else {
        filesystem->fatData.openFiles[file->handle].open = false;
    }

    return NO_ERROR;
}

int FAT_FindFile(FAT_Filesystem *filesystem, FAT_File *file, const char *name, FAT_DirectoryEntry *entryOutput) {
    if (entryOutput == NULL || file == NULL || name == NULL || filesystem == NULL)
        return NULL_ERROR;

    char fatName[12];

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

    FAT_DirectoryEntry entry;
    int status;
    int i = 0;
    while ((status = FAT_ReadEntry(filesystem, file, &entry)) == NO_ERROR) {
        if (entry.name[0] == '\0' && i > 5) { // no more entries
            status = FILESYSTEM_NOT_FOUND_ERROR;
            break;
        }
        if (memcmp(fatName, entry.name, 11) == 0) {
            *entryOutput = entry;
            return NO_ERROR;
        }
        i++;
    }

    return status;
}

bool FAT_AllowedFilename(const char *name, size_t length, bool caseSensitive) {
    while (*name && length--) {
        char currentCharacter = *name;
        if (!caseSensitive)
            currentCharacter = toUpper(currentCharacter);
        if (strchr(FAT_ALLOWED_ASCII_CHARACTERS, currentCharacter) == NULL && *name < 0x80) // 0x80 and above is allowed
            return false;

        ++name;
    }

    if (length + 1 != 0) // null before reaching the end, +1 because it will be underflowed by 1 by the end of loop, this resetting it to 0
        return false;

    return true;
}

int FAT_ListDirectory(FAT_Filesystem *filesystem, const char *path, FAT_DirectoryEntry *entries, size_t maxEntries, size_t *entriesCountOutput) {
    if (filesystem == NULL || path == NULL || entries == NULL)
        return NULL_ERROR;

    FAT_File *openPath;
    int status;
    if ((status = FAT_Open(filesystem, path, &openPath)) != NO_ERROR)
        return status;

    FAT_DirectoryEntry entry;
    while (*entriesCountOutput < maxEntries && FAT_ReadEntry(filesystem, openPath, &entry) && entry.name[0] != '\0')
        if (FAT_AllowedFilename(entry.name, 11, true)) {
            entries[*entriesCountOutput] = entry;
            ++*entriesCountOutput;
        }

    FAT_Close(filesystem, openPath); // here we will ignore error

    return NO_ERROR;
}

int FAT_Open(FAT_Filesystem *filesystem, const char *path, FAT_File **fileOutput) {
    if (filesystem == NULL || path == NULL || fileOutput == NULL)
        return NULL_ERROR;

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
        int status;
        if ((status = FAT_FindFile(filesystem, current, name, &entry)) == NO_ERROR) {
            FAT_Close(filesystem, current);

            if (!isLast && ((entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0))
                return FILESYSTEM_NOT_FOUND_ERROR;

            // open new directory entry
            if ((status = FAT_OpenEntry(filesystem, &entry, &current)) != NO_ERROR)
                return status;
        } else {
            FAT_Close(filesystem, current);

            return status;
        }
    }

    *fileOutput = current;
    return NO_ERROR;
}
