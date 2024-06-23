#include "disk.h"
#include "fat.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "x86.h"

void _cdecl cstart_(uint16_t bootDrive) {
    DISK disk;
    if (!DISK_Initialize(&disk, bootDrive)) {
        puts("Failed to initialize disk.\r\n");
        goto end;
    }
    if (!FAT_Initialize(&disk)) {
        puts("Failed to initialize FAT.\r\n");
        goto end;
    }

    FAT_File __far *rootDirectoryFd = FAT_Open(&disk, "/");
    FAT_DirectoryEntry entry;
    for (uint8_t i = 0; FAT_ReadEntry(&disk, rootDirectoryFd, &entry) && i < 5; ++i) {
        for (uint8_t i = 0; i < 11; ++i)
            putc(entry.name[i]);
        puts("\r\n");
    }
    FAT_Close(rootDirectoryFd);

    char fileBuffer[64];
    uint32_t read;
    FAT_File __far *readmeFd = FAT_Open(&disk, "info/readme.md");
    while (read = FAT_Read(&disk, readmeFd, sizeof(fileBuffer), fileBuffer))
        for (uint32_t i = 0; i < read; ++i) {
            if (fileBuffer[i] == '\n')
                putc('\r');
            putc(fileBuffer[i]);
        }
    FAT_Close(readmeFd);

end:
    x86_Misc_Halt();
}
