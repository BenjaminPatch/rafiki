#ifndef GET_KEY_H
#define GET_KEY_H

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>


/**
 * Gets the key from keyfile
 */
char* get_key(char* fileName, int* valid);

#endif //GET_KEY_H
