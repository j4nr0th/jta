//
// Created by jan on 13.5.2023.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../../mem/lin_alloc.h"
#include "../formatted.h"

//  Note: specifier %p with snprintf does not match with lin_sprintf, because snprintf does not print leading zeros and
//        uses only lower case hex and 'x', but lin_sprintf uses upper case hex and 'x' with leading zeros

int main()
{
    linear_allocator allocator = lin_alloc_create(1 << 16);
    const char* const fmt_string = "C%#-6.8llX - %d %n si se%cora";
    char* sample_buffer = lin_alloc_allocate(allocator, 1024);
    memset(sample_buffer, 0, 1024);
    int so_far_1 = 0, so_far_2 = 0;
    size_t l_base = snprintf(sample_buffer, 1024, fmt_string, 13ll, (int)sample_buffer, &so_far_1, u'\u00F1');
    printf("Converted value by snprintf was \"%s\" (%zu bytes long)\n", sample_buffer, l_base);
    size_t l_compare;
    char* comparison = lin_sprintf(allocator, &l_compare, fmt_string, 13ll, (int)sample_buffer, &so_far_2, u'\u00F1');
    printf("Converted value by lin_sprintf was \"%s\" (%zu bytes long)\n", comparison, l_compare);
    assert(so_far_2 == so_far_1);
    assert(l_compare == l_base);
    assert(strcmp(sample_buffer, comparison) == 0);

    lin_alloc_destroy(allocator);
    return 0;
}
