#ifndef CS5600_LAB1_CAESAR_H
#define CS5600_LAB1_CAESAR_H

#include <stdio.h>
#include <stdlib.h>


char *encode(char plaintext[128], int key);
char *decode(char ciphertext[128], int key);

#endif
