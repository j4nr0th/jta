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
//      - Do not consume more than TBD bytes of memory overhead
//      - Provide lower time overhead than malloc and free
//      - When no more memory is available, return NULL
//      - Do not allow fragmentation
//

typedef struct linear_allocator_struct* linear_allocator;

linear_allocator lin_alloc_create(uint_fast64_t total_size);

void* lin_alloc_reallocate(linear_allocator allocator, void* ptr, uint_fast64_t new_size);

void lin_alloc_deallocate(linear_allocator allocator, void* ptr);

void* lin_alloc_allocate(linear_allocator allocator, uint_fast64_t size);

void lin_alloc_destroy(linear_allocator allocator);


#endif //JFW_LIN_ALLOC_H
