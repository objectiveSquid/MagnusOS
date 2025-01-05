#include "string.h"
#include <stddef.h>
#include <stdint.h>

int strncmp(const char *string_1, const char *string_2, size_t n) {
    if (string_1 == NULL || string_2 == NULL)
        return -1;

    for (size_t i = 0; i < n; i++) {
        if (string_1[i] != string_2[i]) {
            return (uint8_t)string_1[i] - (uint8_t)string_2[i];
        }
        if (string_1[i] == '\0' || string_2[i] == '\0') {
            break;
        }
    }
    return 0;
}

const char *strchr(const char *str, char chr) {
    if (str == NULL)
        return NULL;

    while (*str) {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char *strcpy(char *destination, const char *source) {
    char *originalDestination = destination;

    if (destination == NULL)
        return NULL;

    if (source == NULL) {
        *destination = '\0';
        return destination;
    }

    while (*source) {
        *destination = *source;
        ++source;
        ++destination;
    }

    *destination = '\0';
    return originalDestination;
}

size_t strlen(const char *string) {
    size_t length = 0;
    for (; string[length]; ++length) {
    }
    return length;
}
