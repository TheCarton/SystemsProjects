/*
 * db client for CS5600 Lab3
 *
 * this is a really ugly piece of code, need to clean it up before the
 * next time I assign it.
 *
 * Peter Desnoyers, 2020
 *
 * 2021 fall: modified by CS5600 staff
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <zlib.h>
#include <pthread.h>
#include <argp.h>
#include <assert.h>

#include "lab3.h"

/* --------- argument parsing ---------- */

static struct argp_option options[] = {
    {"set",          'S', "KEY",  0, "set KEY to VALUE"},
    {"get",          'G', "KEY",  0, "get value for KEY"},
    {"delete",       'D', "KEY",  0, "delete KEY"},
    {"quit",         'q',  0,     0, "send QUIT command"},
    {"test",         'T', "#threads",  0, "run your concurrency test with given number of threads"},
    {0}
};

enum {OP_SET = 1, OP_GET = 2, OP_DELETE = 3, OP_QUIT = 4, OP_TEST = 5};

struct args {
    int nthreads;
    int count;
    int port;
    int max;
    int op;
    char *key;
    char *val;
    char *logfile;
    FILE *logfp;
    struct sockaddr_in addr;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct args *a = state->input;
    switch (key) {
    case ARGP_KEY_INIT:
        a->nthreads = 1;
        a->count = 1000;
        a->port = 5600;
        a->max = 200;
        a->logfp = NULL;
        break;

    case 'l':
        a->logfile = arg;
        if ((a->logfp = fopen(arg, "w")) == NULL)
            fprintf(stderr, "Error opening logfile : %s : %s\n", arg,
                    strerror(errno)), exit(1);
        break;

    case 'q':
        a->op = OP_QUIT;
        break;

    case 'G':
        a->op = OP_GET;
        if (strlen(arg) > 30)
            printf("key must be <= 30 chars\n"), argp_usage(state);
        a->key = arg;
        break;

    case 'S':
        a->op = OP_SET;
        if (strlen(arg) > 30)
            printf("key must be <= 30 chars\n"), argp_usage(state);
        a->key = arg;
        break;

    case 'T':
        a->op = OP_TEST;
        a->nthreads = atoi(arg);
        if (a->nthreads == 0) {
            printf("#threads must be a number\n"), argp_usage(state);
        }
        break;

    case 'D':
        a->op = OP_DELETE;
        if (strlen(arg) > 30)
            printf("key must be <= 30 chars\n"), argp_usage(state);
        a->key = arg;
        break;

    case 'n':
        a->count = atoi(arg); break;

    case 'p':
        a->port = atoi(arg);
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num == 0 && a->op == OP_SET) {
            a->val = arg;
        printf("val set to %s\n", arg);
    }
        else
            argp_usage(state);
        break;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, NULL, NULL};

/* --------- everything else ---------- */

int do_connect(struct sockaddr_in *addr)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0 || connect(sock, (struct sockaddr*)addr, sizeof(*addr)) < 0)
        fprintf(stderr, "can't connect: %s\n", strerror(errno)), exit(0);
    return sock;
}


void do_del(struct args *args, char *name, char *result, int quiet)
{
    int sock = do_connect(&args->addr);

    struct request rq;
    snprintf(rq.name, sizeof(rq.name), "%s", name);

    rq.op_status = 'D';
    int val = write(sock, &rq, sizeof(rq));
    if ((val = read(sock, &rq, sizeof(rq))) < 0)
        printf("DEL: REPLY: READ ERROR: %s\n", strerror(errno));
    else if (val < sizeof(rq))
        printf("DEL: REPLY: SHORT READ: %d\n", val);
    else if (rq.op_status != 'K' && !quiet)
        printf("DEL: FAILED (%c)\n", rq.op_status);
    else if (!quiet)
        printf("ok\n");

    if (result != NULL)
        *result = rq.op_status;

    close(sock);
}

void do_set(struct args *args, char *name, void *data, int len, char *result, int quiet)
{
    int sock = do_connect(&args->addr);

    struct request rq;
    snprintf(rq.name, sizeof(rq.name), "%s", name);
    int val;

    rq.op_status = 'W';
    sprintf(rq.len, "%d", len);
    write(sock, &rq, sizeof(rq));
    write(sock, data, len);
    if ((val = read(sock, &rq, sizeof(rq))) < 0)
        printf("WRITE: REPLY: READ ERROR: %s\n", strerror(errno));
    else if (val < sizeof(rq))
        printf("WRITE: REPLY: SHORT READ: %d\n", val);
    else if (rq.op_status != 'K' && !quiet)
        printf("WRITE: FAILED (%c)\n", rq.op_status);
    else if (!quiet)
        printf("ok\n");

    if (result != NULL)
        *result = rq.op_status;
    close(sock);
}

void do_quit(struct args *args)
{
    int sock = do_connect(&args->addr);
    struct request rq;
    rq.op_status = 'Q';
    write(sock, &rq, sizeof(rq));
    /* if both sides close-on-exit you won't get TIME_WAIT */
}

void do_get(struct args *args, char *name, void *data, int *len_p, char *result)
{
    int val, sock = do_connect(&args->addr);
    struct request rq;
    snprintf(rq.name, sizeof(rq.name), "%s", name);

    rq.op_status = 'R';
    sprintf(rq.len, "%d", 0);
    write(sock, &rq, sizeof(rq));
    if ((val = read(sock, &rq, sizeof(rq))) < 0)
        printf("READ: REPLY: READ ERROR: %s\n", strerror(errno));
    else if (val < sizeof(rq))
        printf("READ: REPLY: SHORT READ: %d\n", val);
    else if (rq.op_status != 'K')
        printf("READ: FAILED (%c)\n", rq.op_status);
    else {
        int len = atoi(rq.len);
        char buf[len];

        for (void *ptr = buf, *max = ptr+len; ptr < max; ) {
            int n = read(sock, ptr, max-ptr);
            if (n < 0) {
                printf("READ DATA: READ ERROR: %s\n", strerror(errno));
                break;
            }
            ptr += n;
        }
        if (data != NULL) {
            memcpy(data, buf, len);
            *len_p = len; //segfault
        }
        else
            printf("=\"%.*s\"\n", len, buf);
    }

    if (result != NULL)
        *result = rq.op_status;

    close(sock);
}


// === concurrency test ===
// helper functions
// (feel free to modify helper functions)

// helper function: generate random string
void randstr(char *buf, int len)
{
    for (int i = 0; i < len; i++)
        buf[i] = 'A' + (random() % 25);
}

// helper function: generate random keys
void gen_randkey(char *buf) {
    int len = random() % 30;
    randstr(buf, len);
}

// helper function: generate random values
void gen_randval(char *buf) {
    int len = random() % 4096;
    randstr(buf, len);
}

typedef struct write_args {
    struct args *args;
    char name[255];
    char data[255];
    int len;
    char result[255];
    int quiet;
} write_args_t;

void *write_thread(void *write_args) {
    struct args *args = ((write_args_t *) write_args)->args;
    char *name = ((write_args_t*)write_args)->name;
    void *data = ((write_args_t*)write_args)->data;
    int len = ((write_args_t*)write_args)->len;
    char *result = ((write_args_t*)write_args)->result;
    int quiet = ((write_args_t*)write_args)->quiet;
    do_set(args, name, data, len, result, quiet);
}

void *write_thread_random_sleep(void *write_args){
    long time = random() % 50000;
    usleep(time);
    write_thread(write_args);
}


typedef struct read_args {
    struct args *args;
    char name[255];
    char * data;
    int * len_p;
    char result[255];
} read_args_t;

void * read_thread(void *read_args) {
    struct args *args = ((read_args_t *) read_args)->args;
    char *name = ((read_args_t*)read_args)->name;
    void *data = ((read_args_t*)read_args)->data;
    int * len_p = ((read_args_t*)read_args)->len_p;
    char *result = ((read_args_t*)read_args)->result;
    do_get(args, name, data, len_p, result);
}

void * read_thread_random_sleep(void *write_args){
    long time = random() % 50000;
    usleep(time);
    read_thread(write_args);
}

void free_write_args(write_args_t * w) {
    free(w);
}

void free_read_args(read_args_t * r) {
    free(r->len_p);
    free(r->data);
    free(r);
}

void test_write_then_read_random_sleep(struct args *args, int num_threads) {
    num_threads = num_threads <=TABLE_MAX ? num_threads : TABLE_MAX;
    printf("test_write_then_read_random_sleep\n");
    pthread_t test_threads[num_threads];
    write_args_t * write_thread_args[num_threads];
    char * expected_key[num_threads];
    char * expected_val[num_threads];

    for (int i = 0; i < num_threads; i++) {
        write_args_t* w = (write_args_t*) malloc(sizeof(write_args_t));
        write_thread_args[i] = w;
        w->args = args;
        char key[32];


        snprintf(key, 32, "key:%d%c", i, '\0');
        strcpy(w->name, key);
        expected_key[i] = strdup(key);

        char val[32];
        snprintf(val, 32, "value:%d%c", i, '\0');
        strcpy(w->data, val);
        expected_val[i] = strdup(val);
        w->len = 4096;
        w->quiet = 1;
        int rc = pthread_create(&test_threads[i], NULL, write_thread_random_sleep, (void *)w);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(test_threads[i], NULL);
    }

    read_args_t * read_thread_args[num_threads];
    char * actual_vals[num_threads];

    for (int i = 0; i < num_threads; i++) {
        read_args_t * r = (read_args_t*) malloc(sizeof(read_args_t));
        read_thread_args[i] = r;
        r->args = args; // ?
        char name[32];
        snprintf(name, 32, "key:%d%c", i, '\0');
        strcpy(r->name, name);
        int len_data = 4096;

        int * len_p = malloc(sizeof(int *));
        *len_p = len_data;

        char * data;
        data = (char *) malloc(*len_p);
        data[*len_p] = '\0';
        actual_vals[i] = data;
        r->data = data;

        r->len_p = len_p;

        int rc = pthread_create(&test_threads[i], NULL, read_thread_random_sleep, (void *) r);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(test_threads[i], NULL);
    }

    for (int i = 0; i < num_threads; i++) {
        int correct_read = strcmp(actual_vals[i], expected_val[i]);
        if (correct_read != 0) {
            printf("Incorrect read value in random sleep write-read test!\n");
            printf("key: %s\n", expected_key[i]);
            printf("expected val: %s\n", expected_val[i]);
            printf("actual val: %s\n", actual_vals[i]);
            assert(correct_read);
        }
        do_del(args, expected_key[i], NULL, 1);
    }

    for (int i = 0; i < num_threads; i++) {

        write_args_t* w = write_thread_args[i];
        free(expected_key[i]);
        free(expected_val[i]);
        free_write_args(w);

        read_args_t* r = read_thread_args[i];
        free_read_args(r);
    }
}

void test_interleaved(struct args *args, int num_threads) {
    num_threads = num_threads <=TABLE_MAX ? num_threads : TABLE_MAX;

    printf("test_interleaved\n");
    pthread_t write_threads[num_threads];
    write_args_t * write_thread_args[num_threads];
    char * expected_key[num_threads];
    char * expected_val[num_threads];

    for (int i = 0; i < num_threads; i++) {
        write_args_t* w = (write_args_t*) malloc(sizeof(write_args_t));
        write_thread_args[i] = w;
        w->args = args;
        char key[32];


        snprintf(key, 32, "key:%d%c", i, '\0');
        strcpy(w->name, key);
        expected_key[i] = strdup(key);

        char val[32];
        snprintf(val, 32, "value:%d%c", i, '\0');
        strcpy(w->data, val);
        expected_val[i] = strdup(val);
        w->len = 4096;
        w->quiet = 1;
        int rc = pthread_create(&write_threads[i], NULL, write_thread_random_sleep, (void *)w);
    }

    pthread_t read_threads[num_threads];
    read_args_t * read_thread_args[num_threads];
    char * actual_vals[num_threads];

    for (int i = 0; i < num_threads; i++) {
        pthread_join(write_threads[i], NULL);

        read_args_t * r = (read_args_t*) malloc(sizeof(read_args_t));
        read_thread_args[i] = r;
        r->args = args; // ?
        char name[32];
        snprintf(name, 32, "key:%d%c", i, '\0');
        strcpy(r->name, name);
        int len_data = strlen(name);

        int * len_p = malloc(sizeof(int *));
        *len_p = len_data;

        char * data;
        data = (char *) malloc(*len_p);
        data[*len_p] = '\0';
        actual_vals[i] = data;
        r->data = data;

        r->len_p = len_p;

        int rc = pthread_create(&read_threads[i], NULL, read_thread_random_sleep, (void *) r);

        pthread_join(read_threads[i], NULL);
        int correct_read = strcmp(actual_vals[i], expected_val[i]);
        if (correct_read != 0) {
            printf("Incorrect read value in random sleep write-read test!\n");
            printf("key: %s\n", expected_key[i]);
            printf("expected val: %s\n", expected_val[i]);
            printf("actual val: %s\n", actual_vals[i]);
            assert(correct_read);
        }
        do_del(args, expected_key[i], NULL, 1);
    }

    for (int i = 0; i < num_threads; i++) {
        write_args_t* w = write_thread_args[i];
        free(expected_key[i]);
        free(expected_val[i]);
        free_write_args(w);

        read_args_t* r = read_thread_args[i];
        free_read_args(r);
    }
}

/*
 * Exercise 5:
 * Write your own concurrency tests.
 *
 * You should (these are *suggestions* not requirements):
 *
 *  - create multiple threads and send concurrent operations to dbserver
 *
 *  - try to produce unnatural schedules and interleaving to increase chances
 *  to find bugs
 *
 *  - you don't have to follow the six rules or always use monitor here
 *    because this is for debugging purpose. For example,
 *
 *    -- you can use sleep,
 *
 *    -- and feel free to use synchronization primitives (e.g., mutexes)
 *    whatever way you like
 *
 *  - to check correctness, we suggest you to compare your writes and check
 *  the return values from reads. How? remember your write values and compare
 *  the values when you read from the same keys.
 *
 *  - hints:
 *    -- read "do_get/do_set/do_del" to see how to issue these operations
 *    -- you can use the above helper functions to generate keys/values
 *    -- if you want to share data between multiple threads, protect the shared data with mutex!
 *       (note: you will not lose points due to coding style in Exercise 5.)
 */
void do_test(struct args *a)
{
    // below is a simple single-threaded test
    char data[4096];
    int len = 4096;
    char result;

    do_set(a, "key0", "val0", strlen("val0"), NULL, 1);
    do_get(a, "key0", &data, &len, NULL);
    data[len] = '\0';    // why we add this? for strcmp
    assert (strcmp(data,"val0") == 0);

    do_del(a, "key0", NULL, 1);
    printf("Expected to see: \"READ: FAILED (X)\" below\n");
    do_get(a, "key0", &data, &len, &result); // should have a READ ERROR
    data[len] = '\0';
    assert (result == 'X');


    /* TODO: your code here */
    // fix random seed; easier to reproduce your bugs
    srandom(5600);

    int num_threads = a->nthreads;
    test_write_then_read_random_sleep(a, num_threads);
    // test_interleaved(a, num_threads); doesn't work on VM????
}

int main(int argc, char **argv)
{
    struct args args;
    memset(&args, 0, sizeof(args));

    argp_parse(&argp, argc, argv, 0, 0, &args);

    args.addr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port = htons(args.port),
        .sin_addr.s_addr = inet_addr("127.0.0.1")}; /* localhost */

    if (args.op == OP_SET)
        do_set(&args, args.key, args.val, strlen(args.val), NULL, 0);
    else if (args.op == OP_GET)
        do_get(&args, args.key, NULL, NULL, NULL);
    else if (args.op == OP_DELETE)
        do_del(&args, args.key, NULL, 0);
    else if (args.op == OP_QUIT)
        do_quit(&args);
    else if (args.op == OP_TEST)
        do_test(&args);
    else
        printf("See usage: \"dbclient --help\"\n");
}

