//
// Created by jan on 4.5.2023.
//
#include <stdlib.h>
#include "../aligned_jalloc.h"
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
        memset(new_ptr, (int)i, NEW_SIZES[i]);
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


    allocator = aligned_jallocator_create(0, 4);
    assert(allocator);
    assert(aligned_jallocator_verify(allocator));

    void** small_ptrs = aligned_jalloc(allocator, 32, sizeof(*small_ptrs) * 2048);

    for (uint64_t i = 0; i < 2048; ++i)
    {
        small_ptrs[i] = aligned_jalloc(allocator, 8, 64);
        assert(small_ptrs[i]);
    }

    for (uint64_t i = 0; i < 2048; ++i)
    {
        aligned_jfree(allocator, small_ptrs[i]);
    }

    {
        void* tmp = aligned_jrealloc(allocator, small_ptrs[69], 256, 256);
        assert(tmp == small_ptrs[69]);
    }
    aligned_jfree(allocator, small_ptrs);


    void* big_pointers[4];
    for (uint32_t i = 0; i < 4; ++i)
    {
        big_pointers[i] = aligned_jalloc(allocator, 2048, 1 << 20);
        assert(big_pointers[i]);
        memset(big_pointers[i], 69, 1 << 20);
    }

    {
        void* tmp = aligned_jrealloc(allocator, big_pointers[0], 64, 64);
        assert(tmp);
        big_pointers[0] = tmp;
        for (uint32_t i = 0; i < 64; ++i)
        {
            assert(((uint8_t*)tmp)[i] == 69);
        }
        memset(tmp, 69, 64);
        tmp = aligned_jrealloc(allocator, big_pointers[1], 64, 512);
        assert(tmp);
        big_pointers[1] = tmp;
        for (uint32_t i = 0; i < 512; ++i)
        {
            assert(((uint8_t*)tmp)[i] == 69);
        }
        memset(tmp, 69, 512);
        tmp = aligned_jrealloc(allocator, big_pointers[2], 8, 1 << 21);
        assert(tmp);
        big_pointers[2] = tmp;
        for (uint32_t i = 0; i < 1 << 20; ++i)
        {
            assert(((uint8_t*)tmp)[i] == 69);
        }
        memset(tmp, 69, 1 << 21);
    }
    for (uint32_t i = 0; i < 4; ++i)
    {
        aligned_jfree(allocator, big_pointers[i]);
    }

    void* lmao = aligned_jalloc(allocator, 8, 512 + 8);
    void* ptr = aligned_jalloc(allocator, 8, 1024);
    assert(ptr);
    memset(ptr, 69, 1024);
    void* tmp = aligned_jrealloc(allocator, ptr, 2048, 2048);
    assert(aligned_jallocator_verify(allocator));
    for (uint32_t i = 0; i < 1024; ++i)
    {
        assert(((uint8_t*)tmp)[i] == 69);
    }
    aligned_jfree(allocator, tmp);
    assert(aligned_jallocator_verify(allocator));
    aligned_jfree(allocator, lmao);
    assert(aligned_jallocator_verify(allocator));


    void* med_array[4] = {};
    for (uint32_t i = 0; i < 4; ++i)
    {
        med_array[i] = aligned_jrealloc(allocator, NULL, 8, ((1 << 16) - (1 << 10)));
        assert(med_array[i]);
    }
    ptr = aligned_jrealloc(allocator, med_array[3], 8, 1 << 8);
    assert(ptr == med_array[3]);
    med_array[3] = ptr;
    for (uint32_t i = 0; i < 4; ++i)
    {
        aligned_jfree(allocator, med_array[i]);
    }


    assert(aligned_jallocator_verify(allocator));
    printf("Wasted memory: %zu bytes\n", aligned_jallocator_lifetime_waste(allocator));
    aligned_jallocator_destroy(allocator);

    return 0;
}