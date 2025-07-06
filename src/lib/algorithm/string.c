#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int strcmp(const char *string1, const char *string2) {
    while (*string1 && (*string1 == *string2)) {
        string1++;
        string2++;
    }
    return *(unsigned char *)string1 - *(unsigned char *)string2;
}

int strncmp(const char *string1, const char *string2, size_t length) {
    if (string1 == NULL || string2 == NULL)
        return -1;

    for (size_t i = 0; i < length; i++) {
        if (string1[i] != string2[i]) {
            return (uint8_t)string1[i] - (uint8_t)string2[i];
        }
        if (string1[i] == '\0' || string2[i] == '\0') {
            break;
        }
    }
    return 0;
}

const char *strchr(const char *string, char character) {
    if (string == NULL)
        return NULL;

    while (*string) {
        if (*string == character)
            return string;

        ++string;
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
    for (; string[length]; ++length)
        ;
    return length;
}
