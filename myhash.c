#define _GNU_SOURCE
#include "myhash.h"
#include <errno.h>
#include <stdlib.h>
#include <search.h>
#include <stdio.h>
#include <string.h>
typedef struct timespec timespec;

int hashcreate(size_t capacity, struct hash_struct *ht)
{
    size_t n = capacity / 0.8;
    ht->keys = malloc(capacity*sizeof(char*));
    ht->data = malloc(capacity*sizeof(observations));
    ht->hashtable = calloc(1, sizeof(struct hsearch_data));
    int success = hcreate_r( n, ht->hashtable);
    if (!success) {
        perror("hcreate_r");
    }
    ht->size=0;
    ht->capacity = capacity;
    return success;
}

int hashsearch(char *key, observations **data, struct hash_struct *ht)
{
    ENTRY e;
    e.key = key;
    ENTRY *ep;
    int success = hsearch_r(e, FIND, &ep, ht->hashtable);
    if (!success && errno != ESRCH) {
        perror("hsearch_r");
    } else if (success){
        *data = ep->data;
        if (*data==NULL)
        {
            abort();
        }
    }
    return success;
}

int hashadd(char *key, timespec stamp, struct hash_struct *ht)
{
    if (ht->size+1>ht->capacity)
    {
        hashresize(ht);
    }
    size_t n = ht->size;
    ht->keys[n]=key;
    ht->data[n].observation_times= malloc(8*sizeof(observations));
    ht->data[n].observation_times[0]=stamp;
    ht->data[n].size = 1;
    ht->data[n].capacity = 8;
    ENTRY e, *ep;
    e.key = key;
    e.data = &(ht->data[n]);
    int success = hsearch_r(e, ENTER, &ep, ht->hashtable);
    printf("new_ssid: %s\n",key);
    if (!success) {
        perror("hsearch_r");
    }
    ht->size++;
    return success;
}

int hashclear( struct hash_struct *ht)
{
    for (size_t i = 0; i < ht->size; i++) {
        free(ht->keys[i]);
        free(ht->data[i].observation_times);
    }
    free(ht->data);
    hdestroy_r(ht->hashtable);
    int success = hcreate_r(1024 / 0.8 , ht->hashtable);
    if (!success) {
        perror("hcreate_r");
    }
    ht->capacity = 1024;
    ht->size = 0;
    return success;
}

int hashresize(struct hash_struct *ht)
{
    size_t new_alloc = (size_t) 1.1 * (ht->size);
    ht->keys = realloc(ht->keys, new_alloc * sizeof(char *));
    if (ht->keys == NULL) {
        perror("realloc");
        abort();
    }
    ht->data = realloc(ht->data, new_alloc * sizeof(observations));
    if (ht->data == NULL || errno) {
        perror("realloc");
        abort();
    }
    hdestroy_r(ht->hashtable);

    return 0;
}

int add_observation(observations *o, timespec t)
{
    size_t n = o->size;
    if (n > o->capacity)
    {
        size_t new_alloc = 2*o->capacity*sizeof(observations);
        o->observation_times = realloc(o->observation_times,new_alloc);
        if (o->observation_times == NULL)
        {
            perror("realloc");
            abort();
        }
        o->capacity = 2*o->capacity;
    }
    o->observation_times[n]=t;
    o->size++;
    return 0;
}

void hashstats(struct hash_struct *ht)
{
    printf("ssids: %lu",ht->size);
}
