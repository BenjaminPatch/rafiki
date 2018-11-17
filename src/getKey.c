#include "getKey.h"

/**
 * Gets the key from keyfile
 */
char* get_key(char* fileName, int* valid) {
    int sizeOfString = 1;
    *valid = 1;
    char* key = (char*)malloc(sizeof(char));
    memset(key, '\0', sizeOfString);
    FILE* keyFile;
    if ((keyFile = fopen(fileName, "r")) == NULL) {
        *valid = 0;
        return key;
    }
    int count = 0;
    int c;
    while (1) {
        c = fgetc(keyFile);
        if (feof(keyFile)) {
            break;
        }
        if (c == '\n') {
            *valid = 0;
        }
        if (count == sizeOfString) {
            key = realloc(key, sizeof(char) * sizeOfString * 2);
            if (key == NULL) {
                *valid = 0;
            } else {
                //key = temp;
                sizeOfString *= 2;
            }
        }
        key[count] = c;
        count++;
    }
    if (count == 0) {
        *valid = 0;
    }
    fclose(keyFile);
    return key;
}
