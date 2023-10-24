#include "queue.h"

// Exercise 2: implement a concurrent queue

// (hint: don't forget to initialize them)
static pthread_mutex_t queue = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_non_empty = PTHREAD_COND_INITIALIZER;


// add a new task to the end of the queue
// NOTE: queue must be implemented as a monitor
void enqueue(queue_t *q, task_t *t) {
    pthread_mutex_lock(&queue);
    if (q->head == NULL) {
        q->head = t;
        q->tail = t;
    } else {
        q->tail->next = t;
        q->tail = t;
    }
    pthread_cond_signal(&queue_non_empty);
    pthread_mutex_unlock(&queue);
}

// fetch a task from the head of the queue.
// if the queue is empty, the thread should wait.
// NOTE: queue must be implemented as a monitor
task_t* dequeue(queue_t *q) {
    pthread_mutex_lock(&queue);
    while (q->head == NULL) {
        pthread_cond_wait(&queue_non_empty, &queue);
    }
    task_t *task = q->head;
    q->head = q->head->next;

    pthread_mutex_unlock(&queue);
    return task;
}

// return the number of tasks in the queue.
// NOTE: queue must be implemented as a monitor
int queue_count(queue_t *q) {
    pthread_mutex_lock(&queue);
    int count = 0;
    task_t *task = q->head;
    while(task != NULL) {
        task = task->next;
        count++;
    }
    pthread_mutex_unlock(&queue);
    return count;
}
