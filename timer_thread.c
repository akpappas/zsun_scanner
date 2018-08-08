#define _POSIX_C_SOURCE 20089L
#define _GNU_SOURCE
#include "timer_thread.h"
#include <time.h>
#include <stdio.h>
typedef struct timespec timespec;
typedef struct timeval timeval;

static const long int BILLION = 1000000000;
void showtime(timespec *time)
{
    char buff[100];
    strftime(buff, sizeof buff, "%D %T", gmtime(& (time->tv_sec)));
    printf("Current time: %s.%09ld UTC\n", buff, time->tv_nsec);
}

void show_timespec(timespec *time)
{
    printf("%ld.%09ld\n", time->tv_sec, time->tv_nsec);
}

int timespec_subtract(timespec *result, timespec x, timespec y)
{
    //Original code from GNU C library manual.
    if (x.tv_nsec < y.tv_nsec) {
        int nsec = (y.tv_nsec - x.tv_nsec) / BILLION + 1;
        y.tv_nsec -= BILLION * nsec;
        y.tv_sec += nsec;
    }
    if (x.tv_nsec - y.tv_nsec > BILLION) {
        int nsec = (x.tv_nsec - y.tv_nsec) / BILLION;
        y.tv_nsec += BILLION * nsec;
        y.tv_sec -= nsec;
    }

    if (result != NULL) {
        result->tv_sec = x.tv_sec - y.tv_sec;
        result->tv_nsec = x.tv_nsec - y.tv_nsec;
    }
    return x.tv_sec < y.tv_sec;
}
