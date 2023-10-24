#include "kvstore.h"


// Exercise 4: finish implementing kvstore
// (hint: don't forget to initialize them)
static pthread_mutex_t kv_lock = PTHREAD_MUTEX_INITIALIZER;



/* read a key from the key-value store.
 *
 * if key doesn't exist, return NULL.
 *
 * NOTE: kv-store must be implemented as a monitor.
 */
int find_key(kvstore_t *kv, char *key) {
    // size_t len = strnlen(key, MAX_KEY_LEN);
    for (int i = 0; i < TABLE_MAX; i++) {
        if (kv->keys[i].stat == 0) continue;

        if (strcmp(kv->keys[i].key, key) == 0)
            return i;
    }
    return -1;
}

int key_len(kvstore_t *kv) {
    int len = 0;
    for (int i = 0; i < TABLE_MAX; i++) {
        if (kv->keys[i].stat == 1) len++;
    }
    return len;
}

int get_new_index(kvstore_t *kv) {
    for (int i = 0; i < TABLE_MAX; i++) {
        if (kv->keys[i].stat == 0) return i;
    }
    return -1;
}

int get_index(char * key){
    size_t key_len = strnlen(key, MAX_KEY_LEN);
    int index = 0;
    for (int i = 0; i < key_len; i++) {
        index += key[i] % TABLE_MAX;
    }
    return index;
}

char * calloc_val(char *val) {
    size_t len = strlen(val);
    char *new_val = calloc(len + 1, sizeof(char));
    strcpy(new_val, val);
    return new_val;
}


char* kv_read(kvstore_t *kv, char *key) {
    pthread_mutex_lock(&kv_lock);
    int i = find_key(kv, key);
    char *value = NULL;
    if (i >= 0) value = kv->values[i];

    pthread_mutex_unlock(&kv_lock);
    return value;
}


/* write a key-value pair into the kv-store.
 *
 * - if the key exists, overwrite the old value.
 * - if key doesn't exit,
 *     -- insert one if the number of keys is smaller than TABLE_MAX
 *     -- return failure if the number of keys equals TABLE_MAX
 * - return 0 for success; return 1 for failures.
 *
 * notes:
 * - the input "val" locates on stack, you must copy the string to
 *   kv-store's own memory. (hint: use malloc)
 * - the "val" is a null-terminated string. Pay attention to how many bytes you
 *   need to allocate. (hint: you need an extra to store '\0').
 * - Read "man strlen" and "man strcpy" to see how they handle string length.
 *
 * NOTE: kv-store must be implemented as a monitor.
 */

int kv_write(kvstore_t *kv, char *key, char *val) {
    pthread_mutex_lock(&kv_lock);
    int key_index = find_key(kv, key);
    int write_status = 1;
    if (key_index > 0) { // key exists
        char *new_val = calloc_val(val);
        char *old_val = kv->values[key_index];
        kv->values[key_index] = new_val;
        free(old_val);
        write_status = 0;
    } else if (key_len(kv) < TABLE_MAX) { // insert one if the number of keys is smaller than TABLE_MAX
        key_entry_t new_key;
        new_key.stat = 1;
        strncpy(new_key.key, key, 31);
        new_key.key[31] = '\0';
        int new_index = get_new_index(kv);
        kv->keys[new_index] = new_key;
        char *new_val = calloc_val(val);
        kv->values[new_index] = new_val;
        write_status = 0;
    }

    pthread_mutex_unlock(&kv_lock);
    return write_status;
}


/* delete a key-value pair from the kv-store.
 *
 * - if the key exists, delete it.
 * - if the key doesn't exist, do nothing.
 *
 * NOTE: kv-store must be implemented as a monitor.
 */
void kv_delete(kvstore_t *kv, char *key) {
    pthread_mutex_lock(&kv_lock);
    int key_index = find_key(kv, key);
    if (key_index >= 0) {
        kv->keys[key_index].stat = 0;
        free(kv->values[key_index]);
    }

    pthread_mutex_unlock(&kv_lock);
}


// print kv-store's contents to stdout
// note: use any format that you like; this is mostly for debugging purpose
void kv_dump(kvstore_t *kv) {
    printf("kv_dump:\n");
    for (int i = 0; i < TABLE_MAX; i++) {
        if (kv->keys[i].stat == 1) {
            printf("{K=%s : V=%s}\n", kv->keys[i].key, kv->values[i]);
        }
    }
}