//
// Created by jan on 27.4.2023.
//

#include "jalloc.h"
#include <malloc.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
typedef struct mem_chunk_struct mem_chunk;
struct mem_chunk_struct
{
    uint_fast64_t size:62;
    uint_fast64_t used:1;
    uint_fast64_t malloced:1;
    mem_chunk* next;
    mem_chunk* prev;
};
typedef struct mem_pool_struct mem_pool;
struct mem_pool_struct
{
    uint_fast64_t free;
    uint_fast64_t used;
    mem_chunk* largest;
    mem_chunk* smallest;
    void* base;
};


struct jallocator_struct
{
    uint_fast64_t pool_size;
    uint_fast64_t capacity;
    uint_fast64_t count;
    mem_pool* pools;
    uint_fast64_t malloc_limit;
};


jallocator* jallocator_create(uint_fast64_t pool_size, uint_fast64_t malloc_limit, uint_fast64_t initial_pool_count)
{
    jallocator* this = malloc(sizeof(*this));
    if (!this)
    {
        return this;
    }
    *this = (jallocator){};
    this->capacity = initial_pool_count < 32 ? 32 : initial_pool_count;
    this->pools = calloc(this->capacity, sizeof(*this->pools));
    if (!this->pools)
    {
        free(this);
        return NULL;
    }

    static long PG_SIZE = 0;
    if (!PG_SIZE)
    {
        PG_SIZE = sysconf(_SC_PAGESIZE);
    }
    assert(PG_SIZE != 0);
    pool_size = PG_SIZE * (pool_size / PG_SIZE + ((pool_size % PG_SIZE) != 0)) ;
    this->malloc_limit = malloc_limit;
    this->pool_size = pool_size;
    for (uint_fast32_t i = 0; i < initial_pool_count; ++i)
    {
        mem_pool* p = this->pools + i;
        p->base = mmap(NULL, pool_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        if (p->base == MAP_FAILED)
        {
            for (uint_fast32_t j = 0; j < i; ++j)
            {
                int r = munmap(this->pools[i].base, pool_size);
                assert(r == 0);
            }
            free(this->pools);
            free(this);
            return NULL;
        }
//        p->mem_size = pool_size;
        p->used = 0;
        p->free = pool_size;
        mem_chunk* c = p->base;
        p->smallest = c;
        p->largest = c;
        c->used = 0;
        c->malloced = 0;
        c->prev = NULL;
        c->next = NULL;
        c->size = pool_size;
    }
    this->count = initial_pool_count;

    return this;
}

void jallocator_destroy(jallocator* allocator)
{
    for (uint_fast32_t i = 0; i < allocator->count; ++i)
    {
        int r = munmap(allocator->pools[i].base, allocator->pool_size);
        assert(r == 0);
    }
    memset(allocator->pools, 0, sizeof(*allocator->pools) * allocator->count);
    free(allocator->pools);
    memset(allocator, 0, sizeof(*allocator));
    free(allocator);
}

static inline uint_fast64_t round_up_size(uint_fast64_t size)
{
    uint_fast64_t remainder = 8 - (size & 7);
    size += remainder;
    size += offsetof(mem_chunk, next);
    if (size < sizeof(mem_chunk))
    {
        return sizeof(mem_chunk);
    }
    return size;
}

static inline mem_pool* find_supporting_pool(jallocator* allocator, uint_fast64_t size)
{
    for (uint_fast32_t i = 0; i < allocator->count; ++i)
    {
        mem_pool* pool = allocator->pools + i;
        if (pool->largest && size <= pool->largest->size)
        {
            return pool;
        }
    }
    return NULL;
}

static inline mem_chunk* find_ge_chunk_from_largest(mem_pool* pool, uint_fast64_t size)
{
    mem_chunk* current;
    for (current = pool->largest; current; current = current->prev)
    {
        if (current->size >= size)
        {
            return current;
        }
    }
    return NULL;
}

static inline mem_chunk* find_ge_chunk_from_smallest(mem_pool* pool, uint_fast64_t size)
{
    mem_chunk* current;
    for (current = pool->smallest; current; current = current->next)
    {
        if (current->size >= size)
        {
            return current;
        }
    }
    return NULL;
}

static inline void remove_chunk_from_pool(mem_pool* pool, mem_chunk* chunk)
{
    if (chunk->next)
    {
        assert(chunk->next->prev == chunk);
        (chunk->next)->prev = chunk->prev;
    }
    else
    {
//        assert(pool->largest == chunk);
        pool->largest = chunk->prev;
    }
    if (chunk->prev)
    {
        assert(chunk->prev->next == chunk);
        (chunk->prev)->next = chunk->next;
    }
    else
    {
        assert(pool->smallest == chunk);
        pool->smallest = chunk->next;
    }
    pool->free -= chunk->size;
    pool->used += chunk->size;
}

static inline void insert_chunk_into_pool(mem_pool* pool, mem_chunk* chunk)
{
beginning_of_fn:
    assert(chunk->malloced == 0);
    assert(chunk->used == 0);
    //  Check if pool has other chunks
    if (pool->free == 0)
    {
        assert(!pool->smallest && !pool->largest);
        pool->smallest = chunk;
        pool->largest = chunk;
        chunk->next = NULL;
        chunk->prev = NULL;
    }
    else
    {
        mem_chunk* ge_chunk = NULL;
        for (mem_chunk* current = pool->smallest; current; current = current->next)
        {
            //  Check if the current directly follows chunk
            if (((void*)chunk) + chunk->size == (void*)current)
            {
                //  Pull the current from the pool
                remove_chunk_from_pool(pool, current);
                //  Merge chunk with current
                chunk->size += current->size;
                goto beginning_of_fn;
            }

            //  Check if the current directly precedes chunk
            if (((void*)current) + current->size == (void*)chunk)
            {
                //  Pull the current from the pool
                remove_chunk_from_pool(pool, current);
                //  Merge chunk with current
                current->size += chunk->size;
                chunk = current;
                goto beginning_of_fn;
            }

            //  Check if current->size is greater than or equal chunk->size
            if (!ge_chunk && current->size >= chunk->size)
            {
                ge_chunk = current;
            }
        }

        //  Find first larger or equally sized chunk
//        mem_chunk* ge_chunk = find_ge_chunk_from_smallest(pool, chunk->size);
        if (!ge_chunk)
        {
            //  No others are larger or of equal size, so this is the new largest
            assert(pool->largest->size < chunk->size);
            pool->largest->next = chunk;
            chunk->prev = pool->largest;
            chunk->next = NULL;
            pool->largest = chunk;
        }
        else
        {
            //  Chunk belongs after the ge_chunk in the list
            chunk->prev = ge_chunk->prev;
            chunk->next = ge_chunk;
            assert(!chunk->prev || chunk->prev->size <= chunk->size);
            assert(!chunk->next || chunk->next->size >= chunk->size);
            //  Check if ge_chunk was smallest in pool
            if (ge_chunk->prev)
            {
                (ge_chunk->prev)->next = chunk;
            }
            else
            {
                assert(ge_chunk == pool->smallest);
                pool->smallest = chunk;
            }
            ge_chunk->prev = chunk;
        }
    }
    pool->free += chunk->size;
    pool->used -= chunk->size;
}

static inline mem_pool* find_chunk_pool(jallocator* allocator, void* ptr)
{
    for (uint_fast32_t i = 0; i < allocator->count; ++i)
    {
        mem_pool* pool = allocator->pools + i;
        if (pool->base <= ptr && pool->base + (pool->used + pool->free) > ptr)
        {
            return pool;
        }
    }
    return NULL;
}

void* jalloc(jallocator* allocator, uint_fast64_t size)
{
    const uint_fast64_t original_size = size;
    //  Round up size to 8 bytes
    size = round_up_size(size);

    //  Check if malloc should be used
    if (size >= allocator->malloc_limit)
    {
        mem_chunk* chunk = malloc(size);
        if (!chunk)
        {
            return NULL;
        }
        chunk->used = 1;
        chunk->malloced = 1;
        chunk->size = size;
        return &chunk->next;
    }

    //  Check there's a pool that can support the allocation
    mem_pool* pool = find_supporting_pool(allocator, size);
    if (!pool)
    {
        //  Create a new pool
        if (allocator->count == allocator->capacity)
        {
            uint_fast64_t new_capacity = allocator->capacity << 1;
            mem_pool* new_ptr = realloc(allocator->pools, new_capacity * sizeof(*allocator->pools));
            if (!new_ptr)
            {
                return NULL;
            }
            memset(new_ptr + allocator->count, 0, sizeof(*new_ptr) * (allocator->capacity - allocator->count));
            allocator->pools = new_ptr;
            allocator->capacity = new_capacity;
        }
        mem_chunk* base_chunk = mmap(NULL, allocator->pool_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        if (base_chunk == MAP_FAILED)
        {
            return NULL;
        }
        base_chunk->size = allocator->pool_size;
        base_chunk->malloced = 0;
        base_chunk->used = 0;
        base_chunk->next = NULL;
        base_chunk->prev = NULL;
        mem_pool new_pool =
                {
                .base = base_chunk,
                .largest = base_chunk,
                .smallest = base_chunk,
                .used = 0,
                .free = allocator->pool_size,
                };
        pool = allocator->pools + allocator->count;
        allocator->pools[allocator->count++] = new_pool;
    }

    //  Find the smallest block which fits
    mem_chunk* chunk = find_ge_chunk_from_largest(pool, size);
    assert(chunk);
    remove_chunk_from_pool(pool, chunk);

    //  Check if chunk can be split
    uint_fast64_t remaining = chunk->size - size;
    if (remaining >= sizeof(mem_chunk))
    {
        mem_chunk* new_chunk = ((void*)chunk) + size;
        new_chunk->malloced = 0;
        new_chunk->used = 0;
        new_chunk->size = remaining;
        chunk->size = size;
        insert_chunk_into_pool(pool, new_chunk);
    }

    chunk->used = 1;
    return &chunk->next;
}

void* jrealloc(jallocator* allocator, void* ptr, uint_fast64_t new_size)
{
    if (!ptr)
    {
        return jalloc(allocator, new_size);
    }
    const uint_fast64_t original_size = new_size;
    new_size = round_up_size(new_size);



    mem_chunk* chunk = ptr - offsetof(mem_chunk, next);
    //  Try and find pool it came from
    mem_pool* pool = find_chunk_pool(allocator, ptr);
    //  Check if it came from a pool
    if (!pool)
    {
        //  It did not come from a pool, so check if it was malloced (this will be a crash for an invalid pointer
        if (chunk->malloced == 1)
        {
            //  was malloced, use realloc
            mem_chunk* new_chunk = realloc(chunk, new_size);
            if (!new_chunk)
            {
                return NULL;
            }
            new_chunk->malloced = 1;
            new_chunk->used = 1;
            new_chunk->size = new_size;
            return &new_chunk->next;
        }
        return NULL;
    }

    assert(chunk->malloced == 0);
    //  Since this dereferences chunk, this can cause SIGSEGV
    if (new_size == chunk->size)
    {
        return ptr;
    }


    //  Does its new size warrant the usage of malloc?
    if (new_size > allocator->malloc_limit)
    {
        //  Allocate the new chunk
        assert(new_size > chunk->size);
        mem_chunk* new_chunk = malloc(new_size + offsetof(mem_chunk, next));
        if (!new_chunk)
        {
            return NULL;
        }
        //  Write chunk information
        new_chunk->size = new_size;
        new_chunk->used = 1;
        new_chunk->malloced = 1;
        //  Copy data to new chunk from old location
        memcpy(&new_chunk->next, ptr, new_size > chunk->size ? chunk->size : new_size);
        //  Return the old chunk to its pool
        chunk->used = 0;
        insert_chunk_into_pool(pool, chunk);
        //  Return new memory
        return &new_chunk->next;
    }

    //  Check if increasing or decreasing the chunk's size
    if (new_size > chunk->size)
    {
        //  Increasing
        //  Check if current block can be expanded so that there's no moving it
        //  Location of potential candidate
        mem_chunk* possible_chunk = ((void*)chunk) + chunk->size;
        if (!((void*)possible_chunk >= pool->base                              //  Is the pointer in range?
            && (void*)possible_chunk < pool->base + pool->free + pool->used //  Is the pointer in range?
            && possible_chunk->used == 0                                         //  Is the other chunk in use
            && possible_chunk->size + chunk->size >= new_size))               //  Is the other chunk large enough to accommodate us
        {
            //  Can not make use of any adjacent chunks, so allocate a new block, copy memory to it, free current block, then return the new block
            void* new_ptr = jalloc(allocator, new_size - offsetof(mem_chunk, next));
            memcpy(new_ptr, ptr, chunk->size - offsetof(mem_chunk, next));
            jfree(allocator, ptr);
            return new_ptr;
        }

        //  All requirements met
        //  Pull the chunk from the pool
        remove_chunk_from_pool(pool, possible_chunk);
        //  Join the two chunks together
        chunk->size += possible_chunk->size;
        //  Redo size check
        goto size_check;
    }
    else
    {
    size_check:
        assert(chunk->size >= new_size);
        //  Decreasing
        //  Check if block can be split in two
        const uint_fast64_t remainder = chunk->size - new_size;
        if (remainder < sizeof(mem_chunk))
        {
            //  Can not be split, return the original pointer
            return ptr;
        }
        //  Split the chunk
        mem_chunk* new_chunk = ((void*)chunk) + new_size;
        chunk->size = new_size;
        new_chunk->size = remainder;
        new_chunk->used = 0;
        new_chunk->malloced = 0;
        //  Put the split chunk into the pool
        insert_chunk_into_pool(pool, new_chunk);
    }


    return &chunk->next;
}

int jallocator_verify(jallocator* allocator, int_fast32_t* i_pool, int_fast32_t* i_block)
{
#ifndef NDEBUG
#define VERIFICATION_CHECK(x) assert(x)
#else
#define VERIFICATION_CHECK(x) if (!(x)) { if (i_pool) *i_pool = i; if (i_block) *i_block = j; return -1;}
#endif

    for (int_fast32_t i = 0, j = 0; i < allocator->count; ++i, j = -1)
    {
        const mem_pool* pool = allocator->pools + i;
        uint_fast32_t accounted_free_space = 0, accounted_used_space = 0;
        //  Loop forward to verify forward links and free space
        j = 0;
        for (const mem_chunk* current = pool->smallest; current; current = current->next, ++j)
        {
            VERIFICATION_CHECK(current->prev || current == pool->smallest);
            VERIFICATION_CHECK(!current->prev || current->size >= current->prev->size);
            VERIFICATION_CHECK(!current->next || current->next->prev == current);
            VERIFICATION_CHECK(current->used == 0);
            VERIFICATION_CHECK(current->malloced == 0);
            VERIFICATION_CHECK(current->size >= sizeof(mem_chunk));
            accounted_free_space += current->size;
        }
        VERIFICATION_CHECK(accounted_free_space == pool->free);

        accounted_free_space = 0;
        //  Loop forward to verify backwards links and free space
        j = 0;
        for (const mem_chunk* current = pool->largest; current; current = current->prev, ++j)
        {
            VERIFICATION_CHECK(current->next || current == pool->largest);
            VERIFICATION_CHECK(!current->next || current->size <= current->next->size);
            VERIFICATION_CHECK(!current->prev || current->prev->next == current);
            VERIFICATION_CHECK(current->used == 0);
            VERIFICATION_CHECK(current->malloced == 0);
            VERIFICATION_CHECK(current->size >= sizeof(mem_chunk));
            accounted_free_space += current->size;
        }
        VERIFICATION_CHECK(accounted_free_space == pool->free);

        accounted_free_space = 0;
        j = 0;
        //  Do a full walk through the whole block
        for (void* current = pool->base; current < pool->base + allocator->pool_size; current += ((mem_chunk*)current)->size, j -= 1)
        {
            mem_chunk* chunk = current;
            VERIFICATION_CHECK(chunk->size >= sizeof(mem_chunk));
            VERIFICATION_CHECK(chunk->malloced == 0);
            VERIFICATION_CHECK(current < pool->base + allocator->pool_size);
            if (chunk->used)
            {
                accounted_used_space += chunk->size;
            }
            else
            {
                accounted_free_space += chunk->size;
            }
            if (chunk->used == 0)
            {
                if (chunk->next)
                {
                    VERIFICATION_CHECK(chunk->next->prev == chunk && chunk->size <= chunk->next->size);
                }
                if (chunk->prev)
                {
                    VERIFICATION_CHECK(chunk->prev->next == chunk && chunk->size >= chunk->prev->size);
                }
            }
        }
    }
    return 0;
}

void jfree(jallocator* allocator, void* ptr)
{
    //  Check for null
    if (!ptr) return;
    //  Check what pool this is from
    mem_pool* pool = find_chunk_pool(allocator, ptr);
    mem_chunk* chunk = ptr - offsetof(mem_chunk, next);
    if (!pool)
    {
        //  This is either going to be a malloced chunk or a SIGSEGV
        if (chunk->malloced)
        {
            free(chunk);
        }
        return;
    }
    assert(chunk->malloced == 0);

    //  Mark chunk as no longer used, then return it back to the pool
    chunk->used = 0;
    insert_chunk_into_pool(pool, chunk);
}

static inline uint_fast64_t check_for_aligned_size(mem_chunk* chunk, uint_fast64_t alignment, uint_fast64_t size)
{
    void* ptr = (void*)&chunk->next;
    uintptr_t extra = (uintptr_t)ptr & (alignment - 1);
    if (!extra)
    {
        return size;
    }
    return size + (alignment - extra);
}

void* jalloc_aligned(jallocator* allocator, uint_fast64_t alignment, uint_fast64_t size)
{
    //  Check that the alignment is a power of two
    assert(((alignment - 1) ^ alignment) == ((alignment - 1) | alignment));
    return NULL;
}
