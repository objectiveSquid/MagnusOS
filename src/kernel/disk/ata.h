#pragma once

#include <stdbool.h>
#include <stdint.h>

bool ATA_ReadSectors(uint32_t lba, void *buffer, uint8_t count, bool master);