#include "fat.h"
#include "memdefs.h"
#include "memory.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "utility.h"

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10

#define ROOT_DIRECTORY_HANDLE -1
#define UNUSED_HANDLE -2

#pragma pack(push, 1)
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

    // extended boot record
    uint8_t driveNumber;
    uint8_t _Reserved;
    uint8_t signature;
    uint32_t volumeId;       // serial number, value doesn't matter
    uint8_t volumeLabel[11]; // 11 bytes, padded with spaces
    uint8_t systemId[8];
} FAT_BootSector;
#pragma pack(pop)

typedef struct
{
    char buffer[SECTOR_SIZE];
    FAT_File public;
    bool open;
    uint32_t firstCluster;
    uint32_t currentCluster;
    uint32_t currentSectorInCluster;
} FAT_FileData;

typedef struct
{
    union {
        FAT_BootSector bootSector;
        char bootSectorBytes[SECTOR_SIZE];
    } BS;

    FAT_FileData rootDirectory;
    FAT_FileData openFiles[MAX_FILE_HANDLES];
} FAT_Data;

static FAT_Data __far *g_Data;
static char __far *g_Fat = NULL;
static uint32_t g_DataSectionLba;

bool FAT_ReadBootSector(DISK *disk) {
    return DISK_ReadSectors(disk, 0, 1, g_Data->BS.bootSectorBytes);
}

bool FAT_ReadFat(DISK *disk) {
    return DISK_ReadSectors(disk, g_Data->BS.bootSector.reservedSectors, g_Data->BS.bootSector.sectorsPerFat, g_Fat);
}

bool FAT_Initialize(DISK *disk) {
    g_Data = (FAT_Data __far *)MEMORY_FAT_ADDR;

    // read boot sector
    if (!FAT_ReadBootSector(disk)) {
        puts("FAT: Failed to read boot sector.\r\n");
        return false;
    }

    // read fat
    g_Fat = (char __far *)g_Data + sizeof(FAT_Data);
    uint32_t fatSize = g_Data->BS.bootSector.bytesPerSector * g_Data->BS.bootSector.sectorsPerFat;
    if (sizeof(FAT_Data) + fatSize >= MEMORY_FAT_SIZE) {
        printf("FAT: No enough memory to read FAT. (required: %lu, only have: %lu)\r\n", (ulint_t)(sizeof(FAT_Data) + fatSize), (ulint_t)MEMORY_FAT_SIZE);
        return false;
    }

    if (!FAT_ReadFat(disk)) {
        puts("FAT: Failed to read FAT.\r\n");
        return false;
    }

    // read root directory
    uint32_t rootDirectoryLba = g_Data->BS.bootSector.reservedSectors + (g_Data->BS.bootSector.sectorsPerFat * g_Data->BS.bootSector.fatCount);
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

    if (!DISK_ReadSectors(disk, rootDirectoryLba, 1, g_Data->rootDirectory.buffer)) {
        puts("FAT: Failed to read root directory.\r\n");
        return false;
    }

    // calculate data section
    uint32_t rootDirectorySectors = (rootDirectorySize + g_Data->BS.bootSector.bytesPerSector - 1) / g_Data->BS.bootSector.bytesPerSector;
    g_DataSectionLba = rootDirectoryLba + rootDirectorySectors;

    // reset open files
    for (ssize_t i = 0; i < MAX_FILE_HANDLES; ++i) {
        g_Data->openFiles[i].open = false;
    }

    return true;
}

uint32_t FAT_ClusterToLba(uint16_t cluster) {
    return g_DataSectionLba + (cluster - 2) * g_Data->BS.bootSector.sectorsPerCluster;
}

FAT_File __far *FAT_OpenEntry(DISK *disk, FAT_DirectoryEntry *entry) {
    ssize_t handle = UNUSED_HANDLE;
    for (size_t i = 0; i < MAX_FILE_HANDLES && handle < 0; ++i) {
        if (!g_Data->openFiles[i].open)
            handle = i;
    }

    // out of handles
    if (handle == UNUSED_HANDLE) {
        puts("FAT: Out of file handles.\r\n");
        return false;
    }

    // set up variables
    FAT_FileData __far *fd = &g_Data->openFiles[handle];
    fd->public.handle = handle;
    fd->public.isDirectory = (entry->attributes & FAT_ATTRIBUTE_DIRECTORY) != 0;
    fd->public.position = 0;
    fd->public.size = entry->size;
    fd->firstCluster = entry->firstClusterLow + ((uint32_t)entry->firstClusterHigh << 16);
    fd->currentCluster = fd->firstCluster;
    fd->currentSectorInCluster = 0;

    if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->currentCluster), 1, fd->buffer)) {
        puts("FAT: Read error.\r\n");
        return false;
    }

    fd->open = true;
    return &fd->public;
}

uint32_t FAT_NextCluster(uint32_t currentCluster) {
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t *)(g_Fat + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t *)(g_Fat + fatIndex)) >> 4;
}

uint32_t FAT_Read(DISK *disk, FAT_File __far *file, uint32_t byteCount, void *dataOutput) {
    FAT_FileData __far *fd;
    if (file->handle == ROOT_DIRECTORY_HANDLE)
        fd = &g_Data->rootDirectory;
    else
        fd = &g_Data->openFiles[file->handle];

    char *charDataOutput = (char *)dataOutput;

    byteCount = min(byteCount, fd->public.size - fd->public.position);
    while (byteCount > 0) {
        uint32_t leftInBuffer = SECTOR_SIZE - (fd->public.position % SECTOR_SIZE);
        uint32_t take = min(byteCount, leftInBuffer);

        memcpy(charDataOutput, fd->buffer + fd->public.position % SECTOR_SIZE, take);
        charDataOutput += take;
        fd->public.position += take;
        byteCount -= take;

        if (byteCount > 0) {
            // special handling for root directory
            if (fd->public.handle == ROOT_DIRECTORY_HANDLE) {
                ++fd->currentCluster;

                if (!DISK_ReadSectors(disk, fd->currentCluster, 1, fd->buffer)) {
                    puts("FAT: Failed to read next sector.\r\n");
                    break;
                }
            } else {
                if (++fd->currentSectorInCluster >= g_Data->BS.bootSector.sectorsPerCluster) {
                    fd->currentSectorInCluster = 0;
                    fd->currentCluster = FAT_NextCluster(fd->currentCluster);
                }

                if (fd->currentCluster >= 0xFF8) {
                    puts("FAT: Invalid next cluster.\r\n");
                    break;
                }

                if (!DISK_ReadSectors(disk, FAT_ClusterToLba(fd->currentCluster) + fd->currentSectorInCluster, 1, fd->buffer)) {
                    puts("FAT: Failed to read next sector.\r\n");
                    break;
                }
            }
        }
    }

    return charDataOutput - (uint8_t *)dataOutput;
}

bool FAT_ReadEntry(DISK *disk, FAT_File __far *file, FAT_DirectoryEntry *directoryEntryOutput) {
    return FAT_Read(disk, file, sizeof(FAT_DirectoryEntry), directoryEntryOutput) == sizeof(FAT_DirectoryEntry);
}

void FAT_Close(FAT_File __far *file) {
    if (file->handle == ROOT_DIRECTORY_HANDLE) {
        file->position = 0;
        g_Data->rootDirectory.currentCluster = g_Data->rootDirectory.firstCluster;
    } else {
        g_Data->openFiles[file->handle].open = false;
    }
}

bool FAT_FindFile(DISK *disk, FAT_File __far *file, const char *name, FAT_DirectoryEntry *entryOutput) {
    if (strlen(name) > 11)
        return false;

    char fatName[11];
    FAT_DirectoryEntry entry;

    memset(fatName, ' ', sizeof(fatName));
    const char *extension = strchr(name, '.');
    if (extension == NULL)
        extension = name + 11;

    for (uint8_t i = i; i < 8 && name + i < extension; ++i)
        fatName[i] = toUpper(name[i]);

    if (extension != NULL)
        for (uint8_t i = 0; i < 3 && extension[i + 1]; ++i)
            fatName[i + 8] = toUpper(extension[i + 1]);

    while (FAT_ReadEntry(disk, file, &entry)) {
        if (memcmp(fatName, entry.name, 11)) {
            *entryOutput = entry;
            return true;
        }
    }

    return false;
}

FAT_File __far *FAT_Open(DISK *disk, const char *path) {
    char name[MAX_PATH_SIZE];

    if (path[0] == '/')
        ++path;

    FAT_File __far *current = &g_Data->rootDirectory.public;

    while (*path) {
        bool isLast = false;

        const char *splitted = strchr(path, '/');
        if (splitted != NULL) {
            memcpy(name, path, splitted - path);
            name[splitted - path + 1] = '\0';
            path = splitted + 1;
        } else {
            size_t length = strlen(path);
            memcpy(name, path, length);
            name[length + 1] = '\0';
            path += length;
            isLast = true;
        }

        FAT_DirectoryEntry entry;
        if (FAT_FindFile(disk, current, name, &entry)) {
            FAT_Close(current);

            if (!isLast && (entry.attributes & FAT_ATTRIBUTE_DIRECTORY) == 0) {
                printf("FAT: %s not a directory\r\n", name);
                return NULL;
            }

            // open new directory entry
            current = FAT_OpenEntry(disk, &entry);
        } else {
            FAT_Close(current);

            printf("FAT: %s not found.\r\n", name);
            return NULL;
        }
    }

    return current;
}
