#include "stats.h"

// Exercise 3: fix concurrency bugs by Monitor

// FIXME:
// These statistics should be implemented as a Monitor,
// which keeps track of dbserver's status

int n_writes = 0;  // number of writes
int n_reads = 0;   // number of reads
int n_deletes = 0; // number of deletes
int n_fails = 0;   // number of failed operations


// TODO: define your synchronization variables here
// (hint: don't forget to initialize them)

pthread_mutex_t w_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t r_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t d_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t f_lock = PTHREAD_MUTEX_INITIALIZER;

// FIXME: implementation below is not thread-safe.
// Fix this by implementing them as a Monitor.

void inc_write() {
    pthread_mutex_lock(&w_lock);
    n_writes++;
    pthread_mutex_unlock(&w_lock);
}

void inc_read() {
    pthread_mutex_lock(&r_lock);
    n_reads++;
    pthread_mutex_unlock(&r_lock);

}

void inc_delete() {
    pthread_mutex_lock(&d_lock);
    n_deletes++;
    pthread_mutex_unlock(&d_lock);
}

void inc_fail() {
    pthread_mutex_lock(&f_lock);
    n_fails++;
    pthread_mutex_unlock(&f_lock);
}


int get_writes() { // todo! do these need locks?!?
    return n_writes;
}

int get_reads() {
    return n_reads;
}

int get_deletes() {
    return n_deletes;
}

int get_fails() {
    return n_fails;
}
