//
// Created by jan on 20.4.2023.
//

#ifndef JFW_LIN_ALLOC_H
#define JFW_LIN_ALLOC_H
#include <stdint.h>

//  Linear allocator
//
//  Purpose:
//      Provide quick and efficient dynamic memory allocation in a way similar to a stack with LOFI mechanism
//      ("Last Out, First In" or rather last to be allocated must be the first to be freed). The allocations do not need
//      to be thread-safe.
//
//  Requirements:
//      - Provide dynamic memory allocation capabilities
//      - Provide lower time overhead than malloc and free
//      - When no more memory is available, return NULL
//      - Do not allow fragmentation
//

typedef struct linear_jallocator_struct linear_jallocator;

linear_jallocator* lin_jallocator_create(uint_fast64_t total_size);

void* lin_jrealloc(linear_jallocator* allocator, void* ptr, uint_fast64_t new_size);

void lin_jfree(linear_jallocator* allocator, void* ptr);

void* lin_jalloc(linear_jallocator* allocator, uint_fast64_t size);

void lin_jallocator_destroy(linear_jallocator* allocator);


#endif //JFW_LIN_ALLOC_H
