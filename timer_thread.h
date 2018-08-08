#define _GNU_SOURCE
#pragma once
#include <time.h>



void showtime(struct timespec *time);
void show_timespec(struct timespec *time);
int timespec_subtract(struct timespec *result, struct timespec x, struct timespec y);
