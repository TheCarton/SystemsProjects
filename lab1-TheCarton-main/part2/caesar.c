#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "caesar.h"

bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z');
}

bool is_upper_alpha(char c) {
    return (c >= 'A' && c <= 'Z');
}

bool is_numeric(char c) {
    return (c >= '0' && c <= '9');
}


bool is_alphanumeric(char c) {
    return is_alpha(c) || is_upper_alpha(c) || is_numeric(c);
}

char shift_char(char c, int shift) {
    int len;
    int first_char;
    int last_char;
    if (is_alpha(c)) {
        len = 26;
        first_char = 'a';
        last_char = 'z';
    } else if (is_upper_alpha(c)) {
        len = 26;
        first_char = 'A';
        last_char = 'Z';
    } else {
        len = 10;
        first_char = '0';
        last_char = '9';
    }
    int ascii = c;
    int norm_shift = shift % len;
    int d = ascii + norm_shift;
    if (d > last_char) {
        d = d - len;
    }
    else if (d < first_char) {
        d = d + len;
    }
    char new_c = d;
    return new_c;
}
/**
 * Exercise 1
 * this function encodes the string "plaintext" using the Caesar cipher
 * by shifting characters by "key" positions.
 * Hint: you can reuse the memory from input as the output has
 * the same length as the input.
 **/
char *encode(char plaintext[128], int key) {
    int len = strlen(plaintext);
    for (int i = 0; i < len; i++) {
        if (!is_alphanumeric(plaintext[i])) {
            return "ILLCHAR";
        }
        char d = shift_char(plaintext[i], key);
        plaintext[i] = d;
    }
    return plaintext;
}


/**
 * Exercise 2
 * This function decodes the "ciphertext" string using the Caesar cipher
 * by shifting the characters back by "key" positions.
 **/
char *decode(char ciphertext[128], int key) {
    int len = strlen(ciphertext);
    for (int i = 0; i < len; i++) {
        if (!is_alphanumeric(ciphertext[i])) {
            return "ILLCHAR\n";
        }
        char new_char = shift_char(ciphertext[i], -key);
        ciphertext[i] = new_char;
    }

    return ciphertext;
}
