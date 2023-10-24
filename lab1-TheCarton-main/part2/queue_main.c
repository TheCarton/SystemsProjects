#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "queue.h"

void usage() {
    printf("Usage: ./queue <file>\n");
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
 * Exercise 5:
 * based on the content of "lineptr",
 * you should either call "enqueue" or "dequeue".
 * Read "lab1-shell-APIs.txt" for examples.
 *
 * Hint: check out what string functions are provided by C library.
 * Also, here are some possibly useful functions: strtok, strcmp
 **/
void process_line(queue_t *q, FILE* fp, char *lineptr) {
    char *action = strtok(lineptr, " ");
    if (strcmp(action, "enqueue\n") == 0) {
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
        process_line(q, fp, lineptr);
    }
    final_print(q);
    free_and_close(q, fp, lineptr);
    return 0;
}
