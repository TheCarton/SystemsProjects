#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "queue.h"
#include "caesar.h"

void usage() {
    printf("Usage: ./ciphered-queue <file> \n");
}

void free_and_close(queue_t *q, FILE *fp, char *lineptr) {
    fclose(fp);
    if (lineptr != NULL) {
        free(lineptr);
    }
    free_queue(q);
    free(q);
}

void error(queue_t *q, FILE* fp, char *lineptr) {
    printf("ERROR\n");
    free_and_close(q, fp, lineptr);
    exit(1);
}

/**
 * Exercise 6:
 * define and implement two functions: "encode_enqueue" and "dequeue_decode"
 *   -- "encode_enqueue": encode the string and then enqueue
 *   -- "dequeue_decode": dequeue an element and then decode
 * Read "lab1-shell-APIs.txt" for examples.
 **/

void encode_enqueue(queue_t *q, char *data, int key) {
    char *enc_data = encode(data, key);
    enqueue(q, enc_data);
}

void dequeue_decode(queue_t *q, int key) {
    node_t* node = dequeue(q);
    char* enc_msg = node->data;
    char* msg;
    if (!strcmp(enc_msg, "ILLCHAR")) {
        msg = enc_msg;
    } else {
        msg = decode(node->data, key);
    }
    printf("%s\n", msg);
    free_node(node);
}


/**
 * Exercise 6:
 * based on the content of "lineptr", you should call one of the four functions:
 *  "enqueue", "dequeue", "encode_enqueue", and "dequeue_decode"
 * Read "lab1-shell-APIs.txt" for examples.
 *
 * Hint: (possibly) useful functions: strtok, strcmp, atoi
 **/
void process_line2(queue_t *q, FILE* fp, char *lineptr) {
    char *action = strtok(lineptr, " ");
    if (!strcmp(action, "encode_enqueue")) {

        action = strtok(NULL, " ");
        int key_len = strlen(action) + 1;
        char key_s[key_len];
        strcpy(key_s, action);
        int key = atoi(key_s);

        action = strtok(NULL, "\n");
        int word_len = strlen(action) + 1;
        char word[word_len];
        strcpy(word, action);
        encode_enqueue(q, word, key);
    } else if (!strcmp(action, "dequeue_decode")) {
        action = strtok(NULL, " ");
        int key_len = strlen(action) + 1;
        char key_s[key_len];
        strcpy(key_s, action);
        int key = atoi(key_s);
        dequeue_decode(q, key);
    } else if (strcmp(action, "enqueue\n") == 0) {
        error(q, fp, lineptr);
    } else if (!strcmp(action, "enqueue")) {
        action = strtok(NULL, "\n");
        enqueue(q, action);
    } else if (!strcmp(action, "dequeue\n")) {
        node_t* node = dequeue(q);
        if (node == NULL) {
            printf("NULL\n");
        } else {
            printf("%s\n", node->data);
            free_node(node);
        }
    } else {
        error(q, fp, lineptr);
    }
}


// Notice that the following two functions are borrowed from "queue_main.c".
// (which is bad!) You should not copy-paste code. But in order to isolate you
// possible modifications to "queue_main.c", we clone a copy here.

void final_print(queue_t *q) {
    print_queue(q);
}

int main(int argc, const char *argv[]) {

    if (argc != 2) {
        usage();
        return -1;
    }

    // create a new queue
    queue_t *q = (queue_t *) malloc(sizeof(queue_t));
    q->head = NULL;
    q->tail = NULL;

    // read from file line by line
    const char *path = argv[1];
    FILE *fp;
    char *lineptr = NULL;
    size_t n = 0;

    fp = fopen(path, "r");
    assert(fp != NULL);

    while (getline(&lineptr, &n, fp) != -1) {
        process_line2(q, fp, lineptr);
    }
    final_print(q);

    free_and_close(q, fp, lineptr);

    return 0;
}
