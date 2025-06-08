#include "ascii.h"

bool isLower(char character) {
    return character >= 'a' && character <= 'z';
}

bool isUpper(char character) {
    return character >= 'A' && character <= 'Z';
}

char toUpper(char character) {
    if (isLower(character))
        return character - 32;
    return character;
}

char toLower(char character) {
    if (isUpper(character))
        return character + 32;
    return character;
}
