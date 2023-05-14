//
// Created by jan on 27.4.2023.
//

#ifndef JFW_PROFILING_TIMER_H
#define JFW_PROFILING_TIMER_H
#include <stdint.h>
#include <time.h>

typedef int64_t ns_time_t;

ns_time_t timer_diff(struct timespec* t_begin, struct timespec* t_end);

void timer_thread_get(struct timespec* ts);

void timer_process_get(struct timespec* ts);

void timer_real_get(struct timespec* ts);

void timer_monotonic_get(struct timespec* ts);

#endif //JFW_PROFILING_TIMER_H
