//
// Created by jan on 4.5.2023.
//
#include <stdlib.h>
#include "../vk_allocator.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define A 128
#define ALIGN_MIN 1
#define ALIGN_MAX 11
#define ALLOC_MIN 32
#define ALLOC_MAX (1 << 16)

void* POINTER_ARRAY[A] = {};
size_t ALIGN_ARRAY[A] = {};
size_t SIZE_ARRAY[A] = {};

int check_allocated_blocks(aligned_jallocator* allocator, void* exclude)
{
    for (uint32_t j = 0; j < A; ++j)
    {
        if (POINTER_ARRAY[j] == exclude) continue;
        size_t size = aligned_jalloc_block_size(allocator, POINTER_ARRAY[j]);
        assert(size >= SIZE_ARRAY[j]);
        assert(((uintptr_t)POINTER_ARRAY[j] & (ALIGN_ARRAY[j] - 1)) == 0);
    }
    return 1;
}

int main()
{

    aligned_jallocator* allocator = aligned_jallocator_create(4, 4);

    for (uint32_t i = 0; i < A; ++i)
    {
        assert(aligned_jallocator_verify(allocator));
        for (uint32_t j = 0; j < i; ++j)
        {
            for (uint32_t k = 0; k < SIZE_ARRAY[j]; ++k)
            {
                assert(((uint8_t*)POINTER_ARRAY[j])[k] == (j));
            }
        }
        ALIGN_ARRAY[i] = 1 << ((random() % (ALIGN_MAX - ALIGN_MIN)) + ALIGN_MIN);
        SIZE_ARRAY[i] = (random() % (ALLOC_MAX - ALLOC_MIN)) + ALLOC_MIN;
        SIZE_ARRAY[i] &= ~(ALIGN_ARRAY[i] - 1);
        if (!SIZE_ARRAY[i])
        {
            SIZE_ARRAY[i] = ALIGN_ARRAY[i];
        }
        assert((SIZE_ARRAY[i] % ALIGN_ARRAY[i]) == 0);
        void* pointer = aligned_jalloc(allocator, ALIGN_ARRAY[i], SIZE_ARRAY[i]);
        assert(((uintptr_t)pointer % ALIGN_ARRAY[i]) == 0);
        assert(aligned_jallocator_verify(allocator));
        memset(pointer, (int)(i), SIZE_ARRAY[i]);
        assert(aligned_jallocator_verify(allocator));
        POINTER_ARRAY[i] = pointer;

        for (uint32_t j = 0; j < i + 1; ++j)
        {
            for (uint32_t k = 0; k < SIZE_ARRAY[j]; ++k)
            {
                assert(((uint8_t*)POINTER_ARRAY[j])[k] == (j));
            }
        }
    }

    for (uint32_t i = 0; i < A; ++i)
    {
        assert(aligned_jallocator_verify(allocator));
        aligned_jfree(allocator, POINTER_ARRAY[i]);
        POINTER_ARRAY[i] = 0;
        assert(aligned_jallocator_verify(allocator));
    }

    size_t total_allocation = 0;
    for (uint i = 0; i < A; ++i)
    {
        total_allocation += SIZE_ARRAY[i];
    }
    printf("Total allocated memory: %zu bytes\n", total_allocation);
    printf("Wasted memory: %zu bytes\n", aligned_jallocator_lifetime_waste(allocator));

    aligned_jallocator_destroy(allocator);









    allocator = aligned_jallocator_create(4, 4);
    //  Test the reallocation functions
    uint SHUFFLE_ARRAY_1[A] = {};
    uint SHUFFLE_ARRAY_2[A] = {};
    {
        int FREE_ARRAY_1[A] = {};
        int FREE_ARRAY_2[A] = {};
        for (int i = 0; i < A; ++i)
        {
            FREE_ARRAY_1[i] = i;
            FREE_ARRAY_2[i] = i;
        }
        for (uint i = 0; i < A; ++i)
        {
            uint v = random() % A;
            while (FREE_ARRAY_1[v] == -1)
            {
                v = random() % A;
            }
            SHUFFLE_ARRAY_1[i] = v;
            FREE_ARRAY_1[i] = -1;
            v = random() % A;
            while (FREE_ARRAY_2[v] == -1)
            {
                v = random() % A;
            }
            SHUFFLE_ARRAY_2[i] = v;
            FREE_ARRAY_2[i] = -1;
        }
    }

    for (uint32_t i = 0; i < A; ++i)
    {
        assert(aligned_jallocator_verify(allocator));
        for (uint32_t j = 0; j < i; ++j)
        {
            for (uint32_t k = 0; k < SIZE_ARRAY[j]; ++k)
            {
                assert(((uint8_t*)POINTER_ARRAY[j])[k] == (j));
            }
        }
        ALIGN_ARRAY[i] = 1 << ((random() % (ALIGN_MAX - ALIGN_MIN)) + ALIGN_MIN);
        SIZE_ARRAY[i] = (random() % (ALLOC_MAX - ALLOC_MIN)) + ALLOC_MIN;
        SIZE_ARRAY[i] &= ~(ALIGN_ARRAY[i] - 1);
        if (!SIZE_ARRAY[i])
        {
            SIZE_ARRAY[i] = ALIGN_ARRAY[i];
        }
        assert((SIZE_ARRAY[i] % ALIGN_ARRAY[i]) == 0);
        void* pointer = aligned_jalloc(allocator, ALIGN_ARRAY[i], SIZE_ARRAY[i]);
        assert(((uintptr_t)pointer % ALIGN_ARRAY[i]) == 0);
        assert(aligned_jallocator_verify(allocator));
        memset(pointer, (int)(i), SIZE_ARRAY[i]);
        assert(aligned_jallocator_verify(allocator));
        POINTER_ARRAY[i] = pointer;

        for (uint32_t j = 0; j < i + 1; ++j)
        {
            for (uint32_t k = 0; k < SIZE_ARRAY[j]; ++k)
            {
                assert(((uint8_t*)POINTER_ARRAY[j])[k] == (j));
            }
            assert(aligned_jalloc_block_size(allocator, POINTER_ARRAY[j]) >= SIZE_ARRAY[j]);
        }
    }

    uint32_t NEW_SIZES[A];
    uint32_t NEW_ALIGN[A];
    for (uint32_t i = 0; i < A; ++i)
    {
        NEW_SIZES[i] = SIZE_ARRAY[SHUFFLE_ARRAY_1[i]];
        NEW_ALIGN[i] = ALIGN_ARRAY[SHUFFLE_ARRAY_1[i]];
    }
    struct {uint8_t v;} *p;
    for (uint32_t i = 0; i < A; ++i)
    {
        for (uint32_t j = 0; j < A; ++j)
        {
            uint32_t smaller_size = SIZE_ARRAY[j] > NEW_SIZES[j] ? NEW_SIZES[j] : SIZE_ARRAY[j];
            for (uint32_t k = 0; k < smaller_size; ++k)
            {
                assert(((uint8_t*)POINTER_ARRAY[j])[k] == (j));
            }
        }
        for (uint32_t j = 0; j < A; ++j)
        {
            size_t size = aligned_jalloc_block_size(allocator, POINTER_ARRAY[j]);
            assert(size >= SIZE_ARRAY[j]);
        }
        assert(aligned_jallocator_verify(allocator));
        void* new_ptr = aligned_jrealloc(allocator, POINTER_ARRAY[i], NEW_ALIGN[i], NEW_SIZES[i]);
        assert(new_ptr != NULL);
        if (i == 7)
        {
            p = new_ptr + 0;
        }
        assert(((uintptr_t)new_ptr % NEW_ALIGN[i]) == 0);
        assert(aligned_jallocator_verify(allocator));
        POINTER_ARRAY[i] = new_ptr;
        for (uint32_t j = 0; j < A; ++j)
        {
            uint32_t smaller_size = SIZE_ARRAY[j] > NEW_SIZES[j] ? NEW_SIZES[j] : SIZE_ARRAY[j];
            for (uint32_t k = 0; k < smaller_size; ++k)
            {
                assert(((uint8_t*)POINTER_ARRAY[j])[k] == (j));
            }
        }
        SIZE_ARRAY[i] = NEW_SIZES[i];
        ALIGN_ARRAY[i] = NEW_ALIGN[i];
        memset(new_ptr, i, NEW_SIZES[i]);
    }

    for (uint32_t i = 0; i < A; ++i)
    {
        assert(aligned_jallocator_verify(allocator));
        aligned_jfree(allocator, POINTER_ARRAY[i]);
        POINTER_ARRAY[i] = 0;
        assert(aligned_jallocator_verify(allocator));
    }

    total_allocation = 0;
    for (uint i = 0; i < A; ++i)
    {
        total_allocation += SIZE_ARRAY[i];
    }
    printf("Total allocated memory: %zu bytes\n", total_allocation);
    printf("Wasted memory: %zu bytes\n", aligned_jallocator_lifetime_waste(allocator));

    aligned_jallocator_destroy(allocator);



    return 0;
}