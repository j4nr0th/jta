//
// Created by jan on 30.4.2023.
//

#ifndef JTB_ALIGNED_JALLOC_H
#define JTB_ALIGNED_JALLOC_H
#include <jmem/jmem.h>
#include <vulkan/vulkan.h>

//  Based on jalloc allocator

typedef struct aligned_jallocator_struct aligned_jallocator;

aligned_jallocator* aligned_jallocator_create(uint_fast64_t small_pool_count, uint_fast64_t med_pool_count);

void aligned_jallocator_destroy(aligned_jallocator* allocator);

void* aligned_jalloc(aligned_jallocator* allocator, uint_fast64_t alignment, uint_fast64_t size);

void* aligned_jrealloc(aligned_jallocator* allocator, void* ptr, uint_fast64_t alignment, uint_fast64_t new_size);

void aligned_jfree(aligned_jallocator* allocator, void* ptr);

size_t aligned_jallocator_lifetime_waste(aligned_jallocator* allocator);

int aligned_jallocator_verify(aligned_jallocator* allocator);

size_t aligned_jalloc_block_size(aligned_jallocator* allocator, void* ptr);

#endif //JTB_ALIGNED_JALLOC_H
