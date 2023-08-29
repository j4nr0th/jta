//
// Created by jan on 2.6.2023.
//
#include "common.h"

ill_jallocator* G_JALLOCATOR = NULL;
lin_jallocator* G_LIN_JALLOCATOR = NULL;

void jta_timer_set(jta_timer* timer)
{
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &timer->ts);
}

f64 jta_timer_get(jta_timer* timer)
{
    struct timespec end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    return (f64)(end.tv_sec - timer->ts.tv_sec) + (f64)(end.tv_nsec - timer->ts.tv_nsec) * 1e-9;
}
