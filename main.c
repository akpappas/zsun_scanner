#define _POSIX_C_SOURCE 20089L
#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include "proc_limits.h"
#include "myhash.h"
#include "timer_thread.h"

typedef struct timespec timespec;
typedef struct timeval timeval;

FILE *input, *output, *resources_fp;
timespec time_start;
timespec now;
timespec sampling_period = {1, 0};
clock_t cpu_clock_start;
clock_t cpu_clock_end;

// hsearch_data *pairs;
struct hash_struct *pairs;

void unquote(char *str, size_t *length)
{
    int len = strlen(str);
    int quotes = 0;
    int start = 0;
    int end = 0;
    for (int i = 0; i < len; ++i) {
        if (str[i] == '"') {
            if (quotes == 0) {
                start = i;
            } else if (quotes == 1) {
                end = i - 1;
                break;
            }
            quotes++;
        }
    }
    if (end == 0) {
        end = len;
    }
    len = end - start;
    memmove(str, (str + start + 1), len);
    str[len] = '\0';
    *length = len;
    return;
}

void record_resources(timespec *t)
{
#ifdef DEBUG
    FILE *resources_fp = fopen("resources.txt", "a");
    if (resources_fp == NULL) {
        perror("fopen");
        printf("Can't store cpu usage");
    }
    static int count = 0;
    time_t cpu_clock_end = clock();
    float cpu_secs = ((float) cpu_clock_end - cpu_clock_start) / CLOCKS_PER_SEC;
    float percent = cpu_secs / (t->tv_sec + t->tv_nsec / 10e9);
    char line[128];
    sprintf(line, "Time elapsed %ld.%09ld:  CPU time used:%f sec (%f%%) \n", t->tv_sec, t->tv_nsec, cpu_secs, 100*percent);
    fwrite(line, strlen(line), sizeof(char), resources_fp);
    fflush(resources_fp);
    count++;
    if (count > 1000000) {
        resources_fp = fopen("resources.txt", "a");
        if (resources_fp == NULL) {
            perror("fopen");
            printf("Can't store cpu usage");
        } else {
            count = 0;
        }
    }
    fclose(resources_fp);
#endif
}

void handle_events(int fd, int *wd, char *file_watched, struct hash_struct *hash_table, FILE *log)
{
    char buf[4096]
    __attribute__((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    char *ptr;
    /* Loop while events can be read from inotify file descriptor. */
    for (;;) {

        /* Read some events. */
        len = read(fd, buf, sizeof buf);
        if (len == -1 && errno != EAGAIN) {
            puts("cant read inotify events");
            perror("read");
            abort();        }
        /* If the nonblocking read() found no events to read, then
        *                  it returns -1 with errno set to EAGAIN. In that case,
        *                  we exit the loop. */

        if (len <= 0) {
            break;
        }
        /* Loop over all events in the buffer */
        for (ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;
            /* Print event type */
            if (event->mask & IN_CLOSE_WRITE) {
                FILE *new_ssids = fopen(file_watched, "r");
                if (new_ssids == NULL) {
                    puts("cant read new wifi file");
                    perror("fopen");
                }
                char *line = NULL; // = ( char * ) malloc ( 20*sizeof ( char ) );
                size_t line_len = 0;
                struct stat metadata;
                stat(file_watched, &metadata);
                ssize_t read;
                FILE* delay_fp = fopen("delay","a");
                while ((read = getline(&line, &line_len, new_ssids)) != -1) {
                    unquote(line, &line_len);
                    observations *stamps = NULL;
                    int hsearch_err = hashsearch(line, &stamps, hash_table);
                    if (hsearch_err == 0) {
                        char *key = strdup(line);
                        hashadd(key, metadata.st_mtim, hash_table);
                    } else if (hsearch_err != 0) {
                        add_observation(stamps, metadata.st_mtim);
                    }
#ifdef DEBUG
                    timespec now,diff;
                    clock_gettime(CLOCK_REALTIME,&now);
                    timespec_subtract(&diff,now,(metadata.st_mtim));
                    fprintf(delay_fp,"%ld.%09ld\n",diff.tv_sec,diff.tv_nsec);
#endif
                    fprintf(log,"%s, %ld.%09ld\n",line,metadata.st_mtim.tv_sec,metadata.st_mtim.tv_nsec);
                }
                int close_error = fclose(new_ssids);
                if (close_error != 0) {
                    printf("fclose:%d", close_error);
                }
                fclose(delay_fp);
                free(line);
            }
        }
    }
}


void *timer_thread(void *thread_args)
{
    timespec timer, wakeup, slack, scanning_time, elapsed; //,sampling_period;
    int neg;//  = timespec_subtract(&slack, &timer, &wakeup);
    wakeup.tv_sec = 0;
    wakeup.tv_nsec= 0;
    nanosleep(&sampling_period, NULL);
    for (;;) {
        clock_gettime(CLOCK_REALTIME, &timer);
        wakeup.tv_sec = timer.tv_sec + sampling_period.tv_sec;
        timespec_subtract(&elapsed, timer, time_start);
        record_resources(&elapsed);
        timespec start_of_scanning = timer;
        system("./searchWifi.sh");
        clock_gettime(CLOCK_REALTIME, &timer);
        timespec end_of_scanning = timer;
        timespec_subtract(&scanning_time, end_of_scanning, start_of_scanning);
        neg  = timespec_subtract(&slack, wakeup, timer);
        show_timespec(&slack);
        if (neg) {
             printf("Scanning takes longer than the required sampling period.\n");
        }else{
        nanosleep(&slack, NULL);
        }
    }
}

int main(int argc, char **argv)
{
    struct hash_struct ht;
    pairs = &ht;
    mkdir("ssid_log", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    int herr = hashcreate(1024, pairs);
    if (herr == 0) {
        printf("Can't create hash table. Exiting");
        abort();
    }
    cpu_clock_start = clock();
    clock_gettime(CLOCK_REALTIME, &time_start);
    int inotify_fd, poll_num, watch_descriptor;
    struct pollfd poll_fd;
    char *file_watched = NULL;
    pthread_t timer;
    if (argc < 2) {
        int len = strlen("/tmp/wifiout") + 1;
        file_watched = malloc(len * sizeof(char));
        memcpy(file_watched, "/tmp/wifiout", len);
    } else if (argc == 2) {
        sampling_period.tv_sec = atol(argv[1]);
        int len = strlen("/tmp/wifiout") + 1;
        file_watched = malloc(len * sizeof(char));
        memcpy(file_watched, "/tmp/wifiout", len);
    } else {
        sampling_period.tv_sec = atol(argv[1]);
        int len = strlen(argv[2]) + 1;
        memcpy(file_watched, argv[2], len);
        file_watched = strdup(argv[2]);
    }
    FILE *fp = fopen(file_watched, "w");
    fclose(fp);
    int timer_err = pthread_create(&timer, NULL, timer_thread, NULL);
    if (timer_err != 0) {
        errno = timer_err;
        puts("pthread_create");
        perror("pthread_create");
        abort();    }
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd == -1) {
        puts("inotify_init1");
        perror("inotify_init1");
        abort();    }
    watch_descriptor = inotify_add_watch(inotify_fd, file_watched, IN_CLOSE_WRITE);
    if (watch_descriptor == -1) {
        fprintf(stderr, "Cannot watch '%s'\n", file_watched);
        perror("inotify_add_watch");
        abort();
    }
    poll_fd.fd = inotify_fd;
    poll_fd.events = POLLIN;
    /* Wait for events */
    printf("Listening for events.\n");
    char headers[] = "SSID, Time since epoch\n";
    FILE *store_fp = fopen("ssid_log/timestamps", "w");
    fwrite(headers, strlen(headers), sizeof(char), store_fp);
    fclose(store_fp);
    while (1) {
        poll_num = poll(&poll_fd, 1, -1);
        if (poll_num == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            abort();
        }

        if (poll_num > 0) {
            if (poll_fd.revents & POLLIN) {
                check_for_storage();
                FILE *store_fp = fopen("ssid_log/timestamps", "a");
                if (store_fp==NULL)
                {
                    puts("cant open timestamps");
                    perror("fopen");
                    abort();
                }
                track_filesize("ssid_log/timestamps",&store_fp);
                long int mem =  get_kb_occupied();
                if (mem>25*1024)
                {
                    hashclear(pairs);
                }
                handle_events(inotify_fd, &watch_descriptor, file_watched, pairs,store_fp);

                if (store_fp == NULL) {
                    abort();
                }
                fclose(store_fp);
            }
        }
    }
    /* Close inotify file descriptor */
    close(inotify_fd);
    exit(EXIT_SUCCESS);
}





