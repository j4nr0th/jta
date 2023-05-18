//
// Created by jan on 20.4.2023.
//

#include <errno.h>
#include "lin_jalloc.h"
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include <sys/unistd.h>
#include <sys/mman.h>


struct linear_jallocator_struct
{
    void* max;
    void* base;
    void* current;
};

void lin_jallocator_destroy(linear_jallocator* allocator)
{
    linear_jallocator* this = (linear_jallocator*)allocator;
    int res = munmap(this->base, this->max - this->base);
    assert(res == 0);
    free(this);
}

void* lin_jalloc(linear_jallocator* allocator, uint_fast64_t size)
{
    linear_jallocator* this = (linear_jallocator*)allocator;
    //  Get current allocator position and find new bottom
    void* ret = this->current;
    void* new_bottom = ret + size;
    //  Check if it overflows
    if (new_bottom > this->max)
    {
//        return malloc(size);
        return NULL;
    }
    this->current = new_bottom;
#ifndef NDEBUG
    memset(ret, 0xCC, size);
#endif
    return ret;
}

void lin_jfree(linear_jallocator* allocator, void* ptr)
{
    linear_jallocator* this = (linear_jallocator*)allocator;
    if (this->base <= ptr && this->max > ptr)
    {
        //  ptr is from the allocator
        if (this->current > ptr)
        {
#ifndef NDEBUG
        memset(ptr, 0xCC, this->current - ptr);
#endif
            this->current = ptr;
        }
    }
//    else
//    {
//        //  ptr is not from the allocator, should probably be freed
//        free(ptr);
//    }
}

void* lin_jrealloc(linear_jallocator* allocator, void* ptr, uint_fast64_t new_size)
{
    if (!ptr) return lin_jalloc(allocator, new_size);
    linear_jallocator* this = allocator;
    //  Is the ptr from this allocator
    if (this->base <= ptr && this->max > ptr)
    {
        //  Check for overflow
        void* new_bottom = new_size + ptr;
        if (new_bottom > this->max)
        {
//            //  Overflow would happen, so use malloc
//            void* new_ptr = malloc(new_size);
//            if (!new_ptr) return NULL;
//            memcpy(new_ptr, ptr, this->current - ptr);
//            //  Free memory (reduce the ptr)
//            assert(ptr < this->current);
//            this->current = ptr;
//            return new_ptr;
            return NULL;
        }
        //  return ptr if no overflow, but update bottom of stack position
#ifndef NDEBUG
        if (new_bottom > this->current)
        {
          memset(this->current, 0xCC, new_bottom - this->current);
        }
        else if (new_bottom < this->current)
        {
            memset(new_bottom, 0xCC, this->current - new_bottom);
        }
#endif
        this->current = new_bottom;
        return ptr;
    }
    //  ptr was not from this allocator, so assume it was malloced and just pass it on to realloc
//    return realloc(ptr, new_size);
    return NULL;
}

linear_jallocator* lin_jallocator_create(uint_fast64_t total_size)
{
    uint64_t PAGE_SIZE = sysconf(_SC_PAGESIZE);
    linear_jallocator* this = malloc(sizeof(*this));
    if (!this) return this;
    uint64_t extra = (total_size % PAGE_SIZE);
    if (extra)
    {
        total_size += (PAGE_SIZE - extra);
    }
    void* mem = mmap(NULL, total_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (mem == MAP_FAILED)
    {
        free(this);
        return NULL;
    }
    this->base = mem;
    this->current = mem;
    this->max = mem + total_size;

    return this;
}
