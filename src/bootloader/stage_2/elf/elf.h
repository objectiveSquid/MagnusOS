#pragma once

#include "disk/fat.h"
#include <stdbool.h>

bool ELF_Read32Bit(FAT_Filesystem *filesystem, const char *filepath, void **entryPoint);
