#pragma once

#include <lib/disk/fat.h>

int ELF_Load32BitStatic(FAT_Filesystem *filesystem, const char *filepath, void **entryPoint);
