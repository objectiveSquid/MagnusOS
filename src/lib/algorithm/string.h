#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int strncmp(const char *string1, const char *string2, size_t length);
const char *strchr(const char *string, char character);
char *strcpy(char *destination, const char *source);
size_t strlen(const char *string);
