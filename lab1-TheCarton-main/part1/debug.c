#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

// this function is supposed to return a value that doubles "i"
int double_num(int i) {
    return 2 * i;
}

char *msg_from_internet() {
    return "hello world!\0";  // a fake msg
}

// get a message from internet, copy it to a newly allocated memory,
// and return the memory
char* get_msg() {
    // the original message
    char *msg = msg_from_internet();

    // allocate memory
    int msg_len = strlen(msg);
    char *mem = malloc(msg_len + 1);

    // copy message to memory
    for (int i=0; i<=msg_len; i++) {
        mem[i] = msg[i];
    }

    return mem;
}

void process_msg(char *mem) {
    int len = strlen(mem);
    for (int i=0; i<len; i++) {
        mem[i] = toupper(mem[i]);  // capitalize letters
    }
}


int main(void)
{
    // Exercise 2: find two errors and fix them
    int num = 10;
    int doub_num = double_num(num);
    printf("double a number (%d) is (%d)\n", num, doub_num);
    // check if the returned value is what we want
    assert(num + num == doub_num);

    // Exercise 3: fix segfault
    char *mem = get_msg();
    process_msg(mem);
    printf("processed msg: %s\n", mem);
    free(mem);   // free memory

    return 0;
}

