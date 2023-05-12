//
// Created by jan on 20.4.2023.
//

#include <errno.h>
#include "lin_alloc.h"
#include <string.h>
#include <malloc.h>
#include <assert.h>


struct linear_allocator_struct
{
    void* max;
    void* base;
    void* current;
};

void lin_alloc_destroy(linear_allocator allocator)
{
    linear_allocator this = (linear_allocator)allocator;
    free(this->base);
    free(this);
}

void* lin_alloc_allocate(linear_allocator allocator, uint_fast64_t size)
{
    linear_allocator this = (linear_allocator)allocator;
    //  Get current allocator position and find new bottom
    void* ret = this->current;
    void* new_bottom = ret + size;
    //  Check if it overflows
    if (new_bottom > this->max)
    {
        return malloc(size);
    }
    this->current = new_bottom;
    return ret;
}

void lin_alloc_deallocate(linear_allocator allocator, void* ptr)
{
    linear_allocator this = (linear_allocator)allocator;
    if (this->base <= ptr && this->max > ptr)
    {
        //  ptr is from the allocator
        if (this->current > ptr) this->current = ptr;
    }
    else
    {
        //  ptr is not from the allocator, should probably be freed
        free(ptr);
    }
}

void* lin_alloc_reallocate(linear_allocator allocator, void* ptr, uint_fast64_t new_size)
{
    if (!ptr) return lin_alloc_allocate(allocator, new_size);
    linear_allocator this = allocator;
    //  Is the ptr from this allocator
    if (this->base <= ptr && this->max > ptr)
    {
        //  Check for overflow
        void* new_bottom = new_size + ptr;
        if (new_bottom > this->max)
        {
            //  Overflow would happen, so use malloc
            void* new_ptr = malloc(new_size);
            if (!new_ptr) return NULL;
            memcpy(new_ptr, ptr, this->current - ptr);
            //  Free memory (reduce the ptr)
            assert(ptr < this->current);
            this->current = ptr;
            return new_ptr;
        }
        //  return ptr if no overflow, but update bottom of stack position
        this->current = new_bottom;
        return ptr;
    }
    //  ptr was not from this allocator, so assume it was malloced and just pass it on to realloc
    return realloc(ptr, new_size);
}

linear_allocator lin_alloc_create(uint_fast64_t total_size)
{
    linear_allocator this = malloc(sizeof(*this));
    if (!this) return this;
    void* mem = calloc(total_size, 1);
    if (!mem)
    {
        free(this);
        return NULL;
    }
    this->base = mem;
    this->current = mem;
    this->max = mem + total_size;

    return this;
}
