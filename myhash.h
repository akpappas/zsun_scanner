#pragma once
#include <sys/time.h>
#include <search.h>
struct hash_struct
{
    struct hsearch_data *hashtable;
    char **keys;
    struct observations *data;
    size_t size;
    size_t capacity;
};

struct observations {
    struct timespec *observation_times;
    size_t size;
    size_t capacity;
  };

typedef struct observations observations;

int hashcreate(size_t capacity, struct hash_struct *ht);
int hashsearch(char* key, observations **data, struct hash_struct *ht);
int hashadd( char* key, struct timespec stamp, struct hash_struct *ht);
int hashclear(struct hash_struct *ht);
int hashresize(struct hash_struct *ht);
int add_observation(observations *o, struct timespec t);
void hashstats(struct hash_struct *ht);
