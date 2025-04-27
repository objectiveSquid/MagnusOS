#pragma once
#include <stddef.h>
#include <stdint.h>

int strncmp(const char *string_1, const char *string_2, size_t n);
const char *strchr(const char *string, char character);
char *strcpy(char *destination, const char *source);
size_t strlen(const char *string);
