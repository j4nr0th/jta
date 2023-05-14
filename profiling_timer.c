//
// Created by jan on 27.4.2023.
//

#include "profiling_timer.h"
#ifdef linux
#include <unistd.h>
#endif

ns_time_t timer_diff(struct timespec* t_begin, struct timespec* t_end)
{
    ns_time_t t0 = 1000000000 * t_begin->tv_sec + t_begin->tv_nsec;
    ns_time_t t1 = 1000000000 * t_end->tv_sec + t_end->tv_nsec;
    return t1 - t0;
}

void timer_thread_get(struct timespec* ts)
{
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
}

void timer_process_get(struct timespec* ts)
{
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, ts);
}

void timer_real_get(struct timespec* ts)
{
    clock_gettime(CLOCK_REALTIME, ts);
}

void timer_monotonic_get(struct timespec* ts)
{
    clock_gettime(CLOCK_MONOTONIC, ts);
}
