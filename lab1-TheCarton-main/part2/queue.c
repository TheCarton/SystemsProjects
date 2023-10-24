#include "queue.h"
#include "string.h"
#include "assert.h"

/**
 * Exercise 4:
 * this function adds "data" to the end of the queue "q".
 *
 * Hint: you may need "malloc".
 **/
void enqueue(queue_t *q, char *data) {
    node_t* new_node = (node_t*) malloc(sizeof(node_t));
    int len = strlen(data) + 1;
    char *string_mem = (char*) malloc(len);
    strcpy(string_mem, data);
    new_node->data = string_mem;
    new_node->next = NULL;
    if (q->head == NULL) {
        q->head = new_node;
        q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }
}


/**
 * Exercise 4:
 * this function removes and returns the element
 * at the front of the queue "q".
 *
 * Hint: think of when to free the memory.
 **/
void * dequeue(queue_t *q) {
    if (q->head == NULL) {
        return NULL;
    }
    node_t* prev_head = q->head;
    node_t* new_head = prev_head->next;
    q->head = new_head;
    return prev_head;
}


/**
 * Exercise 4:
 * this function will be called to destroy an existing queue,
 * for example, in the end of main() function.
 * Use free() to free the memory you allocated.
 **/

void free_node(node_t * n) {
    if (n->data != NULL) {
        free(n->data);
    }
    free(n);
}


void free_queue(queue_t *q) {
    node_t* node = q->head;
    while (node != NULL) {
        node_t* next_node = node->next;
        free_node(node);
        node = next_node;
    }
}

/**
 * Exercise 4:
 * this function prints a queue in the following format (quotation marks are
 * not part of the output):
 * """
 * finally: [1st element]->[2nd element]->[3rd element]
 * """
 *
 * Example #1:
 *
 * for a queue with one element with data = "ABC", the output should be:
 * finally: [ABC]
 *
 * Example #2:
 *
 * for a queue with two elements "ABC" and "DEF", the output should be:
 * finally: [ABC]->[DEF]
 *
 * Example #3:
 *
 * for an empty queue, the output should be nothing but "finally: ":
 * finally:
 *
 * Notice: you must strictly follow the format above because the grading tool
 * only recognizes this format. If you print something else, you will not get
 * the scores for this exercise.
 **/
void print_queue(queue_t *q) {
    // the output should start with "finally: "
    printf("finally: ");
    node_t* node = q->head;
    while (node != NULL) {
        printf("[%s]", node->data);
        if (node->next != NULL) {
            printf("->");
        }
        node = node->next;
    }
    // the output requires a new line in the end
    printf("\n");
}
