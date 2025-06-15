#pragma once

#include <lib/disk/fat.h>

int ELF_Load32Bit(FAT_Filesystem *filesystem, const char *filepath, void **entryPoint);
