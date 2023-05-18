//
// Created by jan on 13.5.2023.
//
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include "../../mem/lin_jalloc.h"
#include "../formatted.h"
#include "../../profiling_timer.h"

#include <time.h>

//  Note: specifier %p with snprintf does not match with lin_sprintf, because snprintf does not print leading zeros and
//        uses only lower case hex and 'x', but lin_sprintf uses upper case hex and 'x' with leading zeros

int main()
{
    linear_jallocator* allocator = lin_jallocator_create(1 << 16);
    const char* const fmt_string = "C%#-6.8llX - %10G %12.30G %+025.015d %n si se%cora %p";
    char* sample_buffer = lin_jalloc(allocator, 1024);
//    memset(sample_buffer, 0, 1024);
    int so_far_1 = 0, so_far_2 = 0, so_far_3 = 0;
    struct timespec ts_base_0, ts_base_1, ts_mine_0, ts_mine_1, ts_ext_0, ts_ext_1;

    size_t l_compare;

    timer_monotonic_get(&ts_mine_0);
    char* comparison = lin_sprintf(allocator, &l_compare, fmt_string, 13ll, 130.0, DBL_EPSILON, 0xDEADBEEF, &so_far_2, u'\u00F1', sample_buffer);
    timer_monotonic_get(&ts_mine_1);

    printf("Converted value by lin_sprintf was \"%s\" (%zu bytes long), took %li ns\n", comparison, l_compare, timer_diff(&ts_mine_0, &ts_mine_1));
    lin_jfree(allocator, comparison);


    timer_monotonic_get(&ts_base_0);
    size_t l_base = snprintf(sample_buffer, 1024, fmt_string, 13ll, 130.0, DBL_EPSILON, 0xDEADBEEF, &so_far_1, u'\u00F1', sample_buffer);
    timer_monotonic_get(&ts_base_1);

    printf("Converted value by snprintf was \"%s\" (%zu bytes long), took %li ns\n", sample_buffer, l_base, timer_diff(&ts_base_0, &ts_base_1));

    timer_monotonic_get(&ts_mine_0);
    comparison = lin_sprintf(allocator, &l_compare, fmt_string, 13ll, 130.0, DBL_EPSILON, 0xDEADBEEF, &so_far_2, u'\u00F1', sample_buffer);
    timer_monotonic_get(&ts_mine_1);

    printf("Converted value by lin_sprintf was \"%s\" (%zu bytes long), took %li ns\n", comparison, l_compare, timer_diff(&ts_mine_0, &ts_mine_1));
    lin_eprintf(allocator, "Converted value by lin_sprintf was \"%s\" (%zu bytes long), took %li ns\n", comparison, l_compare, timer_diff(&ts_mine_0, &ts_mine_1));
    assert(so_far_2 == so_far_1);
    assert(l_compare == l_base);
    assert(strcmp(sample_buffer, comparison) == 0);
    lin_jfree(allocator, comparison);


    lin_jallocator_destroy(allocator);
    return 0;
}
