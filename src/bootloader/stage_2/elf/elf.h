#pragma once

#include "disk/fat.h"

int ELF_Read32Bit(FAT_Filesystem *filesystem, const char *filepath, void **entryPoint);
