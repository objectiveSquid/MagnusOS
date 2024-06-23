#include "string.h"
#include "stdint.h"

const char *strchr(const char *str, char chr) {
    if (str == NULL)
        return NULL;

    for (; *str; ++str) {
        if (*str == chr)
            return str;
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
