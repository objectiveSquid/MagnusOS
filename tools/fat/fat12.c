#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t BootJumpInstruction[3];
    uint8_t OemIdentifier[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t FatCount;
    uint16_t DirEntryCount;
    uint16_t TotalSectors;
    uint8_t MediaDescriptorType;
    uint16_t SectorsPerFat;
    uint16_t SectorsPerTrack;
    uint16_t Heads;
    uint32_t HiddenSectors;
    uint32_t LargeSectorCount;

    // extended boot record
    uint8_t DriveNumber;
    uint8_t _Reserved;
    uint8_t Signature;
    uint32_t VolumeId;       // serial number, value doesn't matter
    uint8_t VolumeLabel[11]; // 11 bytes, padded with spaces
    uint8_t SystemId[8];
} __attribute__((packed)) /* do not pad to 4 or 8 bytes*/ BootSector;

typedef struct {
    uint8_t Name[11];
    uint8_t Attributes;
    uint8_t _Reserved;
    uint8_t CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh;
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;
    uint32_t Size;
} __attribute__((packed)) DirectoryEntry;

BootSector g_bootSector;
void *g_fat = NULL;
DirectoryEntry *g_rootDirectory = NULL;
uint32_t g_rootDirectoryEnd;

bool readBootSector(FILE *disk) {
    return fread(&g_bootSector, sizeof(g_bootSector), 1, disk) > 0;
}

bool readSectors(FILE *disk, uint32_t lba, uint32_t count, void *outputBuffer) {
    bool success = true;
    success = success && (fseek(disk, lba * g_bootSector.BytesPerSector, SEEK_SET) == 0);
    success = success && (fread(outputBuffer, g_bootSector.BytesPerSector, count, disk) == count);
    return success;
}

bool readFat(FILE *disk) {
    g_fat = (void *)malloc(g_bootSector.SectorsPerFat * g_bootSector.BytesPerSector);
    return readSectors(disk, g_bootSector.ReservedSectors, g_bootSector.SectorsPerFat, g_fat);
}

bool readRootDirectory(FILE *disk) {
    uint32_t lba = g_bootSector.ReservedSectors + (g_bootSector.SectorsPerFat * g_bootSector.FatCount);
    uint32_t size = sizeof(DirectoryEntry) * g_bootSector.DirEntryCount;
    uint32_t sectors = (size / g_bootSector.BytesPerSector);
    if (size % g_bootSector.BytesPerSector != 0)
        ++sectors;

    g_rootDirectoryEnd = lba + sectors;
    g_rootDirectory = (DirectoryEntry *)malloc(sectors * g_bootSector.BytesPerSector);
    return readSectors(disk, lba, sectors, g_rootDirectory);
}

DirectoryEntry *findFile(const char *name) {
    for (uint32_t i = 0; i < g_bootSector.DirEntryCount; i++)
        if (memcmp(name, g_rootDirectory[i].Name, 11) == 0)
            return &g_rootDirectory[i];

    return NULL;
}

bool readFile(FILE *disk, DirectoryEntry *fileEntry, void *outputBuffer) {
    bool success = true;
    uint16_t currentCluster = fileEntry->FirstClusterLow;

    do {
        uint32_t lba = g_rootDirectoryEnd + (currentCluster - 2) * g_bootSector.SectorsPerCluster;
        success = success && readSectors(disk, lba, g_bootSector.SectorsPerCluster, outputBuffer);
        outputBuffer += g_bootSector.SectorsPerCluster * g_bootSector.BytesPerSector;

        uint32_t fatIndex = currentCluster * 3 / 2;
        if (currentCluster % 2 == 0)
            currentCluster = (*(uint16_t *)(g_fat + fatIndex)) & 0x0FFF;
        else {
            currentCluster = (*(uint16_t *)(g_fat + fatIndex)) >> 4;
        }
    } while (success && currentCluster < 0x0FF8);

    return success;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Syntax: %s <disk image> <file name> \n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *disk = fopen(argv[1], "rb");
    if (disk == NULL) {
        fprintf(stderr, "Failed to open disk image: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    if (!readBootSector(disk)) {
        fprintf(stderr, "Failed to read boot sector\n");
        return EXIT_FAILURE;
    }

    if (!readFat(disk)) {
        fprintf(stderr, "Failed to read fat\n");
        free(g_fat);
        return EXIT_FAILURE;
    }

    if (!readRootDirectory(disk)) {
        fprintf(stderr, "Failed to read root directory\n");
        free(g_fat);
        free(g_rootDirectory);
        return EXIT_FAILURE;
    }

    DirectoryEntry *fileEntry = findFile(argv[2]);
    if (fileEntry == NULL) {
        fprintf(stderr, "Could not find file\n");
        free(g_fat);
        free(g_rootDirectory);
        return EXIT_FAILURE;
    }

    char *fileBuffer = (char *)malloc(fileEntry->Size + g_bootSector.BytesPerSector);
    if (!readFile(disk, fileEntry, fileBuffer)) {
        fprintf(stderr, "Failed to read file\n");
        free(g_fat);
        free(g_rootDirectory);
        free(fileBuffer);
        return EXIT_FAILURE;
    }

    for (size_t i = 0; i < fileEntry->Size; i++)
        if (isprint(fileBuffer[i]))
            fputc(fileBuffer[i], stdout);
        else
            printf("<%02x>", fileBuffer[i]);
    fputc('\n', stdout);

    free(g_fat);
    free(g_rootDirectory);
    free(fileBuffer);

    return EXIT_SUCCESS;
}
