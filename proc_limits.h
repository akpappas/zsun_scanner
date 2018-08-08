#pragma once
#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <search.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include "myhash.h"
typedef struct timespec timespec;
typedef struct timeval timeval;
// typedef struct hsearch_data hsearch_data;

// struct observations {
//     timespec *observation_times;
//     size_t count;
// };
// typedef struct observations observations;

void check_for_storage();
long int get_kb_occupied();
void track_filesize(char* filename,FILE **fp);
