//
// Created by jan on 30.4.2023.
//

#define _GNU_SOURCE

#include <sys/mman.h>
#include "vk_allocator.h"
#include <assert.h>
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
typedef uint_fast64_t u64;


//  This deals with "small chunk allocations"
#define SMALL_CHUNK_SIZE (1 << 8)
#define ASSUMED_PAGE_SIZE (1 << 12)
typedef struct mem_chunk_small_struct mem_chunk_small;
struct mem_chunk_small_struct
{
    _Alignas(SMALL_CHUNK_SIZE) uint8_t bytes[SMALL_CHUNK_SIZE];
};

typedef struct mem_pool_small_struct mem_pool_small;
struct mem_pool_small_struct
{
    union
    {
        mem_chunk_small bitmap_chunk;
        uint8_t bitmap[SMALL_CHUNK_SIZE];
    };
    mem_chunk_small chunks[(SMALL_CHUNK_SIZE << 3) - 1];
};

static_assert(sizeof(mem_pool_small) == (SMALL_CHUNK_SIZE << 3) * SMALL_CHUNK_SIZE);    //  Check that expected size is good
static_assert(_Alignof(mem_chunk_small) == SMALL_CHUNK_SIZE);
static_assert(_Alignof(mem_chunk_small) <= ASSUMED_PAGE_SIZE);


//  This is to deal with larger allocations (greater than SMALL_CHUNK_SIZE, expect average size of SMALL_CHUNK_SIZE << 8)
typedef struct mem_chunk_med_struct mem_chunk_med;
struct mem_chunk_med_struct
{
    uint32_t used:1;
    uint32_t offset:31;
    uint32_t size;
};
//  Check size is small :)
static_assert(sizeof(mem_chunk_med) == sizeof(void*));
typedef struct mem_pool_med_struct mem_pool_med;
#define MED_CHUNK_CAPACITY (ASSUMED_PAGE_SIZE / sizeof(mem_chunk_med) - 1)
struct mem_pool_med_struct
{
    uint32_t chunk_count;
    uint32_t chunk_max;
    //  Chunk list has to be sorted by address make merging simpler
    mem_chunk_med chunk_list[MED_CHUNK_CAPACITY];
    uint8_t bytes[];
};
//  Check the size of the header is good
static_assert(sizeof(mem_pool_med) == ASSUMED_PAGE_SIZE);
#define MED_POOL_SIZE (((ASSUMED_PAGE_SIZE / sizeof(mem_chunk_med)) * (1 << 10)))

//  To handle allocations larger than MED_CHUNK_LIMIT use mmap directly
#define MED_CHUNK_LIMIT (1 << 16)
static_assert((UINT32_MAX >> 1) >= MED_CHUNK_LIMIT);
static_assert(((ASSUMED_PAGE_SIZE / sizeof(mem_chunk_med) - 1) * (1 << 10)) >= MED_CHUNK_LIMIT);
typedef struct mem_pool_big_struct mem_pool_big;
#define BIG_ALLOCATION_CAPACITY (ASSUMED_PAGE_SIZE / 2 / sizeof(void*) - 1)
struct mem_pool_big_struct
{
    u64 ptr_count;
    u64 ptr_capacity;
    void* pointer_array[BIG_ALLOCATION_CAPACITY];
    u64 size_array[BIG_ALLOCATION_CAPACITY];
};
static_assert(sizeof(mem_pool_big) == ASSUMED_PAGE_SIZE);

struct aligned_jallocator_struct
{
    u64 lifetime_memory_waste;
    //  Small pools for allocations with size less or equal to SMALL_CHUNK_SIZE and alignment of SMALL_CHUNK_SIZE or less
    uint_fast64_t small_pool_count;
    uint_fast64_t small_pool_capacity;
    mem_pool_small** p_small_pools;

    //  Medium pools for allocations with sizes between SMALL_CHUNK_SIZE and MED_CHUNK_LIMIT
    uint_fast64_t med_pool_count;
    uint_fast64_t med_pool_capacity;
    mem_pool_med** p_med_pools;

    //  Big pool(s) for allocations with sizes between over MED_CHUNK_LIMIT
    mem_pool_big big_pool;
};
//static_assert(sizeof(aligned_jallocator) <= ASSUMED_PAGE_SIZE);

aligned_jallocator* aligned_jallocator_create(uint_fast64_t small_pool_count, uint_fast64_t med_pool_count)
{
    //  Check that page size is appropriate
    u64 pg_size = sysconf(_SC_PAGESIZE);
    assert(pg_size == ASSUMED_PAGE_SIZE);

    aligned_jallocator* this = malloc(sizeof*this);
    if (!this)
    {
        return NULL;
    }
    //  Zero this out
    memset(this, 0, sizeof*this);

    this->small_pool_capacity = small_pool_count > 8 ? small_pool_count : 8;
    this->med_pool_capacity = med_pool_count > 8 ? med_pool_count : 8;

    //  Allocate arrays to hold small and med pools
    this->p_small_pools = calloc(this->small_pool_capacity, sizeof(mem_pool_small*));
    if (!this->p_small_pools)
    {
        goto failed;
    }
    //  Allocate memory for small pools (if needed)
    for (u64 i = 0; i < small_pool_count; ++i)
    {
        mem_pool_small* pool = mmap(NULL, sizeof(mem_pool_small), PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        if (pool == MAP_FAILED)
        {
            goto failed;
        }
        pool->bitmap[0] = 1;    //  Mark first chunk as used (since that's where the pool bitmap is held)
        this->p_small_pools[this->small_pool_count++] = pool;
    }

    this->p_med_pools = calloc(this->med_pool_capacity, sizeof(mem_pool_med*));
    if (!this->p_med_pools)
    {
        goto failed;
    }
    //  Allocate memory for large pools (if needed)
    for (u64 i = 0; i < med_pool_count; ++i)
    {
        mem_pool_med* pool = mmap(NULL, MED_POOL_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        if (pool == MAP_FAILED)
        {
            goto failed;
        }
        this->p_med_pools[this->med_pool_count++] = pool;
        pool->chunk_count = 1;
        pool->chunk_list[0] = (mem_chunk_med){.used = 0, .size = (MED_POOL_SIZE - sizeof(*pool)), .offset = offsetof(mem_pool_med, bytes)};
        pool->chunk_max = MED_CHUNK_CAPACITY;
    }

    this->big_pool.ptr_capacity = BIG_ALLOCATION_CAPACITY;

    return this;
failed:
    //  Here this is a valid ptr already
    if (this->p_small_pools)
    {
        for (u64 j = 0; j < this->small_pool_count; ++j)
        {
            int res = munmap(this->p_small_pools[j], sizeof(mem_pool_small));
            assert(res == 0);
            this->p_small_pools[j] = NULL;
        }
        this->small_pool_count = 0;
        free(this->p_small_pools);
    }
    if (this->p_med_pools)
    {
        for (u64 j = 0; j < this->med_pool_count; ++j)
        {
            int res = munmap(this->p_med_pools[j], MED_POOL_SIZE);
            assert(res == 0);
            this->p_med_pools[j] = NULL;
        }
        this->med_pool_count = 0;
        free(this->p_med_pools);
    }
    free(this);

    return NULL;
}

void aligned_jallocator_destroy(aligned_jallocator* allocator)
{
    assert(allocator->big_pool.ptr_count == 0);
    for (u64 i = 0; i < allocator->big_pool.ptr_count; ++i)
    {
        int res = munmap(allocator->big_pool.pointer_array[i], allocator->big_pool.size_array[i]);
        assert(res == 0);
        allocator->big_pool.pointer_array[i] = 0;
        allocator->big_pool.size_array[i] = 0;
    }
    allocator->big_pool.ptr_count = 0;

    assert(allocator->p_small_pools);
    for (u64 j = 0; j < allocator->small_pool_count; ++j)
    {
        mem_pool_small* const pool = allocator->p_small_pools[j];
        assert(pool->bitmap[0] == 1);
        for (u64 k = 1; k < sizeof(pool->bitmap) / sizeof(*pool->bitmap) - 1; ++k)
        {
            assert(pool->bitmap[k] == 0);
        }
        int res = munmap(allocator->p_small_pools[j], sizeof(mem_pool_small));
        assert(res == 0);
        allocator->p_small_pools[j] = NULL;
    }
    allocator->small_pool_count = 0;
    free(allocator->p_small_pools);

    assert(aligned_jallocator_verify(allocator));
    assert(allocator->p_med_pools);
    for (u64 j = 0; j < allocator->med_pool_count; ++j)
    {
        mem_pool_med* const pool = allocator->p_med_pools[j];
        assert(pool->chunk_count == 1);
        int res = munmap(allocator->p_med_pools[j], MED_POOL_SIZE);
        assert(res == 0);
        allocator->p_med_pools[j] = NULL;
    }
    allocator->med_pool_count = 0;
    free(allocator->p_med_pools);
    memset(allocator, 0, sizeof*allocator);
    free(allocator);
}

void* aligned_jalloc(aligned_jallocator* allocator, uint_fast64_t alignment, uint_fast64_t size)
{
    assert(alignment <= ASSUMED_PAGE_SIZE);
    if (alignment > ASSUMED_PAGE_SIZE)
    {
        //  Not going to align more than a page man
        return NULL;
    }
    assert(size % alignment == 0);
    if (size & (alignment - 1))
    {
        return NULL;
    }

    //  Check if this should be allocated using a small pool
    if (size <= sizeof(mem_chunk_small) && alignment <= _Alignof(mem_chunk_small))
    {
        mem_pool_small* pool;
        //  Use small pool for allocation
        for (u64 i = 0; i < allocator->small_pool_count; ++i)
        {
            pool = allocator->p_small_pools[i];
            //  Check bitmap for first non-full chunk
            u64 j, k;
            uint8_t v = 0;
            for (j = 0, v = 0; j < sizeof(pool->bitmap); ++j)
            {
                v = pool->bitmap[j];
                if (v ^ ((uint8_t)~0)) break;
            }
            if (j == sizeof(pool->bitmap))
            {
                //  Pool is closed :)
                continue;
            }
            //  Invert v
//            uint8_t iv = ~v;
            //  shift iv until lowers bit is one
            for (k = 0; v & (1); v >>= 1) k += 1;
            assert(pool->bitmap[j] ^ (1 << k));
            //  Now both large part of offset and individual offset are known
            u64 offset = j << 3 | k;    //  number of block is 8 * j + k
            assert(8 * j + k == offset);
            assert(offset >= 1 && offset <= sizeof(pool->chunks) / sizeof(*pool->chunks));
            //  Get the memory address
            void* const ptr = pool->chunks + offset - 1;
            //  Mark the chunk as being in use now
            assert(!(pool->bitmap[j] & (uint8_t)(1 << k)));
            pool->bitmap[j] ^= (uint8_t)(1 << k);
            //  Compute memory waste
            allocator->lifetime_memory_waste += sizeof(mem_chunk_small) - size;
            //  Return the pointer
            return ptr;
        }
        //  Pools are closed, need more pools
        if (allocator->small_pool_count == allocator->small_pool_capacity)
        {
            const u64 new_capacity = allocator->small_pool_capacity << 1;
            mem_pool_small** new_ptr = realloc(allocator->p_small_pools, new_capacity * sizeof(mem_pool_small*));
            if (!new_ptr)
            {
                return NULL;
            }
            memset(new_ptr + allocator->small_pool_count, 0, sizeof(mem_pool_small*) * (new_capacity - allocator->small_pool_count));
            allocator->small_pool_capacity = new_capacity;
            allocator->p_small_pools = new_ptr;
        }

        pool = mmap(NULL, sizeof(mem_pool_small), PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if (pool == MAP_FAILED)
        {
            //  Map failed :(
            return NULL;
        }
        pool->bitmap[0] = 3;
        void* ptr = pool->chunks + 0;
        assert(allocator->p_small_pools[allocator->small_pool_count] == NULL);
        allocator->p_small_pools[allocator->small_pool_count++] = pool;
        //  Return first chunk
        return ptr;
    }

    //  Check if the allocation is too big to be done using med pool
    if (size > MED_CHUNK_LIMIT)
    {
        mem_pool_big* const pool = &allocator->big_pool;
        if (pool->ptr_count == pool->ptr_capacity)
        {
            //  Out of allocations to record
            return NULL;
        }
        u64 extra = size % ASSUMED_PAGE_SIZE;
        size += extra ? ASSUMED_PAGE_SIZE - extra : 0;
        void* mapping = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
        if (mapping == MAP_FAILED)
        {
            return NULL;
        }
        //  Record the mapping
        pool->pointer_array[pool->ptr_count] = mapping;
        pool->size_array[pool->ptr_count] = size;
        pool->ptr_count += 1;
        //  Return the newly mapped pages
        return mapping;
    }

#ifndef NDEBUG
    uint test_bool = 1;
#endif
    //  Try using a med pool
    mem_pool_med* pool;
    for (u64 i = 0; i < allocator->med_pool_count; ++i)
    {
        //  Go through available pools
        pool = allocator->p_med_pools[i];

#ifndef NDEBUG
        assert(test_bool);
#endif
    check_pool_possibilities:
        //  Check if there are free chunks (with proper alignment)
        for (mem_chunk_med* p_chunk = pool->chunk_list; p_chunk != pool->chunk_list + pool->chunk_count; ++p_chunk)
        {
            //  Check for usage
            if (p_chunk->used == 1) continue;
            //  Check for size
            if (p_chunk->size < size) continue;
            //  Check for alignment
            uintptr_t ptr_v = (uintptr_t)(pool) + p_chunk->offset;
            uintptr_t extra = ptr_v & (alignment - 1);
            const uintptr_t padding = alignment - extra;
            if (extra && size + padding > p_chunk->size)
            {
                //  Not aligned enough
                continue;
            }
            //  Increase pointer value to be aligned
            if (extra)
            {
                ptr_v += padding;
                allocator->lifetime_memory_waste += padding;
            }
            assert((ptr_v & (alignment - 1)) == 0);
            //  Check if the chunk can be split
            uintptr_t remaining = (((uintptr_t)pool + p_chunk->offset) + p_chunk->size) - (ptr_v + size);
            if (remaining)
            {
                //  Check if it can be joined with the following chunk (which must be free for merger)
                if (p_chunk + 1 < pool->chunk_list + pool->chunk_count && p_chunk[1].used == 0)
                {
                    //  Extend the following chunk backwards
                    mem_chunk_med* next = p_chunk + 1;
                    next->offset -= remaining;
                    next->size += remaining;
                    p_chunk->size -= remaining;
                }
                else if (pool->chunk_count < pool->chunk_max)
                {
                    //  Remaining chunk is large enough to split, and we're not over capacity of new chunks
                    uintptr_t offset = (ptr_v + size)  - (uintptr_t)pool;
                    //  Move the other chunks in the list out of the way
                    memmove(p_chunk + 2, p_chunk + 1, sizeof(mem_chunk_med) * (pool->chunk_count - (p_chunk - pool->chunk_list + 1)));
                    //  Assign the new chunk in place
                    p_chunk[1] = (mem_chunk_med){.offset = offset, .size = remaining, .used = 0};
                    pool->chunk_count += 1;
                    p_chunk->size -= remaining;
                }
                else
                {
                    allocator->lifetime_memory_waste += remaining;
                }
            }
            //  Mark as used and return
            p_chunk->used = 1;
            return (void*)ptr_v;
        }
    }

    //  No medium pool was empty/aligned enough for the job :(
    //  Create a new pool!


    if (allocator->med_pool_count == allocator->med_pool_capacity)
    {
        //  Resize array holding pointers to med_pools
        const u64 new_capacity = allocator->med_pool_capacity << 1;
        mem_pool_med** const new_ptr = realloc(allocator->p_med_pools, sizeof(mem_pool_med*) * new_capacity);
        if (!new_ptr)
        {
            return NULL;
        }
        memset(new_ptr + allocator->med_pool_count, 0, sizeof(mem_pool_med*) * (new_capacity - allocator->med_pool_count));
        allocator->p_med_pools = new_ptr;
        allocator->med_pool_capacity = new_capacity;
    }

    pool = mmap(NULL, MED_POOL_SIZE, PROT_WRITE|PROT_READ, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
    if (pool == MAP_FAILED)
    {
        return NULL;
    }
    pool->chunk_max = MED_CHUNK_LIMIT;
    pool->chunk_count = 1;
    pool->chunk_list[0] = (mem_chunk_med){.used = 0, .offset = offsetof(mem_pool_med, bytes), .size = MED_POOL_SIZE - offsetof(mem_pool_med , bytes)};
    allocator->p_med_pools[allocator->med_pool_count++] = pool;
#ifndef NDEBUG
    test_bool = 0;
#endif
    goto check_pool_possibilities;
}

void* aligned_jrealloc(aligned_jallocator* allocator, void* ptr, uint_fast64_t alignment, uint_fast64_t new_size)
{
    assert(alignment <= ASSUMED_PAGE_SIZE);
    if (alignment > ASSUMED_PAGE_SIZE)
    {
        return NULL;
    }
    assert(new_size % alignment == 0);
    if (new_size & (alignment - 1))
    {
        return NULL;
    }
    if (!ptr)
    {
        return aligned_jalloc(allocator, alignment, new_size);
    }
    //  Search where the pointer is from
    //  Check small block pools
    for (u64 i = 0; i < allocator->small_pool_count; ++i)
    {
        mem_pool_small* const pool = allocator->p_small_pools[i];
        //  Is the pointer outside
        if ((void*)pool->chunks > ptr || (((uintptr_t)pool->chunks) + sizeof(pool->chunks)) <= (uintptr_t)ptr)
        {
            continue;
        }
        if (new_size <= sizeof(mem_chunk_small) && alignment <= _Alignof(mem_chunk_small))
        {
            //  No moving, just let it keep the same chunk :)
            return ptr;
        }
        //  Need new memory location, since the chunks in small pools don't get to grow
        void* new_ptr = aligned_jalloc(allocator, alignment, new_size);
        if (new_ptr)
        {
            //  Copy memory to new pointer
            memcpy(new_ptr, ptr, new_size > sizeof(mem_chunk_small) ? sizeof(mem_chunk_small) : new_size);
            //  Give the chunk back to the pool
            u64 index = ((mem_chunk_small*)ptr) - pool->chunks + 1;
            assert(pool->bitmap[index >> 3] & (uint8_t)((1 << (index & 7))));
            pool->bitmap[index >> 3] ^= (uint8_t)(1 << (index & 7));
        }
        return new_ptr;
    }

    //  Check if the pointer is from the medium size pool
    for (u64 i = 0; i < allocator->med_pool_count; ++i)
    {
        mem_pool_med* const pool = allocator->p_med_pools[i];
        //  Check if pointer is outside of pool
        if (((uintptr_t)pool->bytes) > (uintptr_t)ptr || ((uintptr_t)ptr) > (((uintptr_t)pool) + MED_POOL_SIZE))
        {
            continue;
        }
        u64 j;
        //  Compute the offset from the pool base
        u64 offset = (u64)ptr - (uintptr_t)pool;
        mem_chunk_med* chunk;
        for (j = 0; j < pool->chunk_count; ++j)
        {
            chunk = pool->chunk_list + j;
            if (chunk->offset <= offset && chunk->offset + chunk->size > offset)
            {
                assert(chunk->used == 1);
                break;
            }
        }
        assert(j != pool->chunk_count);
        if (j == pool->chunk_count)
        {
            //  Didn't find the chunk in the list, something went wrong
            return NULL;
        }
        uintptr_t ptr_v = (uintptr_t)pool + chunk->offset;
        uintptr_t extra = ptr_v & (alignment - 1);
        uint_fast64_t padding = extra ? alignment - extra : 0;
        if (extra)
        {
            ptr_v += padding;
        }
        //  Check if size is adequate
        if (chunk->size > padding && chunk->size - padding >= new_size)
        {
            //  Does the block need expansion, contraction, or realignment?
            if (new_size == chunk->size - padding && (uintptr_t) ptr == ptr_v)
            {
                return (void*) ptr_v;
            }

            if (new_size < chunk->size)  //  Has to be shrunken
            {
                mem_chunk_med* possible = pool->chunk_list + j + 1;
                u64 shrinkage = chunk->size - new_size - padding;
                if (
                        j + 1 != pool->chunk_count //  The chunk is not the last one in the list
                        && possible->used == 0 //   The chunk is not in use
                        )
                {
                    assert(possible->offset >= shrinkage);
                    possible->offset -= shrinkage;
                    possible->size += shrinkage;
                    //  Return the original pointer after extra chunk space is given back to other chunk
                }
                else if (j + 1 == pool->chunk_count && pool->chunk_count < pool->chunk_max)
                {
                    pool->chunk_list[pool->chunk_count++] = (mem_chunk_med) { .used = 0, .size = shrinkage, .offset =
                    chunk->offset + new_size + padding };
                }
                else if (pool->chunk_count < pool->chunk_max)
                {
                    //  Inset a new chunk into the array
                    memmove(chunk + 2, chunk + 1, sizeof(*chunk) * (pool->chunk_count - 1 - j));
                    *(chunk + 1) = (mem_chunk_med) { .used = 0, .size = shrinkage, .offset = chunk->offset + new_size + padding};
                    pool->chunk_count += 1;
                }
                if (ptr_v != (uintptr_t)ptr)
                {
                    uintptr_t size = ptr_v > (uintptr_t)ptr ? new_size : new_size - (((uintptr_t)ptr) - ptr_v);
                    assert(size <= chunk->size);
                    memmove((void*) ptr_v, ptr, size);
                }
                chunk->size = new_size + padding;
                return (void*) ptr_v;
            }
        }

        if (new_size + padding > chunk->size)  //  Has to be expanded
        {
            //  Check if the block can be extended in place
            mem_chunk_med* possible = pool->chunk_list + j + 1;
            if (
                    j + 1 != pool->chunk_count //  The chunk is not the last one in the list
                    && possible->used == 0 //   The chunk is not in use
                    && possible->size + chunk->size >= new_size + padding // joining both together gives enough space
                    )
            {
                assert(aligned_jallocator_verify(allocator));
                u64 remainder = possible->size + chunk->size - new_size - padding;
                chunk->size += possible->size - remainder;
                possible->offset += possible->size - remainder;
                possible->size = remainder;
                if (!remainder)
                {
                    //  There is no remaining memory left in the other chunk, remove the empty chunk with no size
                    memmove(possible, possible + 1, sizeof(*possible) * (pool->chunk_count - j - 2));
                    pool->chunk_count -= 1;
                    memset(pool->chunk_list + pool->chunk_count, 0, sizeof(pool->chunk_list[pool->chunk_count]));
                }
                //  Chunk has been expanded, return the original space
                allocator->lifetime_memory_waste += padding;
                if (ptr_v != (uintptr_t)ptr)
                {
                    uintptr_t size = ptr_v > (uintptr_t)ptr ? (uintptr_t)pool + chunk->offset + chunk->size - ptr_v : (uintptr_t)pool + chunk->offset + chunk->size - (uintptr_t)ptr;
                    assert(size <= chunk->size);
                    memmove((void*) ptr_v, ptr, size);
                }
                assert(aligned_jallocator_verify(allocator));
                return (void*) ptr_v;
            }
        }

//  Return the chunk back to the pool
        chunk->used = 0;
        const u64 original_size = chunk->size - ((uintptr_t)ptr - ((uintptr_t)pool + chunk->offset));
        if (j + 1 != pool->chunk_count && pool->chunk_list[j + 1].used == 0)
        {
            //  Chunk can be given to the next one in the list
            pool->chunk_list[j + 1].offset -= chunk->size;
            pool->chunk_list[j + 1].size += chunk->size;
            memmove(pool->chunk_list + j, pool->chunk_list + j + 1, (pool->chunk_count - 1 - j) * sizeof(*pool->chunk_list));
            pool->chunk_count -= 1;
            memset(pool->chunk_list + pool->chunk_count, 0, sizeof(pool->chunk_list[pool->chunk_count]));
        }
        if (j != 0 && pool->chunk_list[j - 1].used == 0)
        {
            //  Chunk can be given to the previous one in the list
            pool->chunk_list[j - 1].size += chunk->size;
            memmove(pool->chunk_list + j, pool->chunk_list + j + 1, (pool->chunk_count - 1 - j) * sizeof(*pool->chunk_list));
            pool->chunk_count -= 1;
            memset(pool->chunk_list + pool->chunk_count, 0, sizeof(pool->chunk_list[pool->chunk_count]));
        }
        assert(aligned_jallocator_verify(allocator));
        // this can change what the chunk is pointing at, since it can insert/remove new blocks to and from the array
        void* new_memory = aligned_jalloc(allocator, alignment, new_size);
        assert(((uintptr_t)new_memory & (alignment - 1)) == 0);
        memmove(new_memory, ptr, new_size > original_size ? original_size : new_size);

        //  Chunk was freed, so return the address now
        return new_memory;
    }

    //  Chunk has to have been from the large pool
    mem_pool_big* const pool = &allocator->big_pool;
    for (u64 i = 0; i < pool->ptr_count; ++i)
    {
        //  Check if ptr matches
        if (ptr != pool->pointer_array[i])
        {
            continue;
        }

        //  Round up size to page size
        u64 extra = ASSUMED_PAGE_SIZE - new_size;
        if (extra)
        {
            new_size += ASSUMED_PAGE_SIZE - extra;
        }

        if (new_size == pool->size_array[i])
        {
            //  size remains the same
            return ptr;
        }

        //  Resize the mapping
        void* new_mapping = mremap(ptr, pool->size_array[i], new_size, MAP_ANONYMOUS|MAP_PRIVATE);
        if (new_mapping == MAP_FAILED)
        {
            //  Try again, but create a fresh mapping
            new_mapping = mmap(NULL, new_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
            if (new_mapping == MAP_FAILED)
            {
                //  Can't create new mapping
                return NULL;
            }
            memcpy(new_mapping, ptr, new_size > pool->size_array[i] ? pool->size_array[i] : new_size);
            int res = munmap(ptr, pool->size_array[i]);
            ptr = NULL;
            assert(res == 0);
        }
        pool->pointer_array[i] = new_mapping;
        pool->size_array[i] = new_size;
        return new_mapping;
    }

    //  ptr was not allocated using this allocator
    assert(0);
    return NULL;
}

void aligned_jfree(aligned_jallocator* allocator, void* ptr)
{
    if (!ptr) return;
    //  Search where the pointer is from
    //  Check small block pools
    for (u64 i = 0; i < allocator->small_pool_count; ++i)
    {
        mem_pool_small* const pool = allocator->p_small_pools[i];
        //  Is the pointer outside
        if ((void*)pool->chunks > ptr || (((uintptr_t)pool->chunks) + sizeof(pool->chunks)) <= (uintptr_t)ptr)
        {
            continue;
        }

        //  mark the block as free on the pool's bitmap
        u64 index = ((mem_chunk_small*)ptr) - pool->chunks + 1;
        assert(pool->bitmap[index >> 3] & (uint8_t)(1 << (index & 7)));
        pool->bitmap[index >> 3] ^= (uint8_t)(1 << (index & 7));
        return;
    }

    //  Check if the pointer is from the medium size pool
    for (u64 i = 0; i < allocator->med_pool_count; ++i)
    {
        mem_pool_med* const pool = allocator->p_med_pools[i];
        //  Check if pointer is outside of pool
        if (((uintptr_t)pool->bytes) > (uintptr_t)ptr || ((uintptr_t)ptr) > (((uintptr_t)pool) + MED_POOL_SIZE))
        {
            continue;
        }
        u64 j;
        //  Compute the offset from the pool base
        u64 offset = (u64)ptr - (uintptr_t)pool;
        mem_chunk_med* chunk;
        for (j = 0; j < pool->chunk_count; ++j)
        {
            chunk = pool->chunk_list + j;
            if (chunk->offset <= offset && chunk->offset + chunk->size > offset)
            {
                assert(chunk->used == 1);
                break;
            }
        }
        assert(j != pool->chunk_count);
        if (j == pool->chunk_count)
        {
            //  Didn't find the chunk in the list, something went wrong
            assert(0);
            return;
        }
        assert(aligned_jallocator_verify(allocator));
        chunk->used = 0;
        if (j + 1 != pool->chunk_count && pool->chunk_list[j + 1].used == 0)
        {
            //  Chunk can be given to the next one in the list
            pool->chunk_list[j + 1].offset -= chunk->size;
            pool->chunk_list[j + 1].size += chunk->size;
            chunk->used = 0;
            memmove(pool->chunk_list + j, pool->chunk_list + j + 1, (pool->chunk_count - 1 - j) * sizeof(*pool->chunk_list));
            pool->chunk_count -= 1;
            memset(pool->chunk_list + pool->chunk_count, 0, sizeof(pool->chunk_list[pool->chunk_count]));
        }
        if (j != 0 && pool->chunk_list[j - 1].used == 0)
        {
            //  Chunk can be given to the previous one in the list
            pool->chunk_list[j - 1].size += chunk->size;
            memmove(pool->chunk_list + j, pool->chunk_list + j + 1, (pool->chunk_count - 1 - j) * sizeof(*pool->chunk_list));
            pool->chunk_count -= 1;
            memset(pool->chunk_list + pool->chunk_count, 0, sizeof(pool->chunk_list[pool->chunk_count]));
        }
        assert(aligned_jallocator_verify(allocator));

        return;
    }

    //  Chunk has to have been from the large pool
    mem_pool_big* const pool = &allocator->big_pool;
    for (u64 i = 0; i < pool->ptr_count; ++i)
    {
        //  Check if ptr matches
        if (ptr != pool->pointer_array[i])
        {
            continue;
        }


        //  free the mapping
        int res = munmap(ptr, pool->size_array[i]);
        memmove(pool->pointer_array + i, pool->pointer_array + i + 1, (pool->ptr_count - i - 1) * sizeof(void*));
        memmove(pool->size_array + i, pool->size_array + i + 1, (pool->ptr_count - i - 1) * sizeof(u64));
        pool->ptr_count -= 1;
        ptr = NULL;
        assert(res == 0);

        return;
    }

    //  ptr was not allocated using this allocator
    assert(0);
}

size_t aligned_jallocator_lifetime_waste(aligned_jallocator* allocator)
{
    return allocator->lifetime_memory_waste;
}

int aligned_jallocator_verify(aligned_jallocator* allocator)
{
    for (u64 i = 0; i < allocator->med_pool_count; ++i)
    {
        mem_pool_med* pool = allocator->p_med_pools[i];
        if (pool->chunk_list[0].offset != offsetof(mem_pool_med, bytes))
        {
            assert(0);
            return 0;
        }
        uintptr_t total_byte_count = pool->chunk_list[0].size + pool->chunk_list[0].offset;
        for (u64 j = 1; j < pool->chunk_count; ++j)
        {
            mem_chunk_med* chunk = pool->chunk_list + j;
            if (chunk->offset <= (chunk-1)->offset)
            {
                assert(0);
                return 0;
            }
            if (chunk->offset - (chunk - 1)->offset != (chunk - 1)->size)
            {
                assert(0);
                return 0;
            }
            if (chunk->offset != total_byte_count)
            {
                assert(0);
                return 0;
            }
            if (pool->chunk_list[j - 1].used == 0 && chunk->used == 0)
            {
                assert(0);
                return 0;
            }
            total_byte_count += chunk->size;
        }
        if (total_byte_count != MED_POOL_SIZE)
        {
            assert(0);
            return 0;
        }
    }
    return 1;
}

size_t aligned_jalloc_block_size(aligned_jallocator* allocator, void* ptr)
{
    //  Search where the pointer is from
    //  Check small block pools
    for (u64 i = 0; i < allocator->small_pool_count; ++i)
    {
        mem_pool_small* const pool = allocator->p_small_pools[i];
        //  Is the pointer outside
        if ((void*)pool->chunks > ptr || (((uintptr_t)pool->chunks) + sizeof(pool->chunks)) <= (uintptr_t)ptr)
        {
            continue;
        }

        return sizeof(mem_chunk_small);
    }

    //  Check if the pointer is from the medium size pool
    for (u64 i = 0; i < allocator->med_pool_count; ++i)
    {
        mem_pool_med* const pool = allocator->p_med_pools[i];
        //  Check if pointer is outside of pool
        if (((uintptr_t)pool->bytes) > (uintptr_t)ptr || ((uintptr_t)ptr) > (((uintptr_t)pool) + MED_POOL_SIZE))
        {
            continue;
        }
        u64 j;
        //  Compute the offset from the pool base
        u64 offset = (u64)ptr - (uintptr_t)pool;
        mem_chunk_med* chunk;
        for (j = 0; j < pool->chunk_count; ++j)
        {
            chunk = pool->chunk_list + j;
            if (chunk->offset <= offset && chunk->offset + chunk->size > offset)
            {
                assert(chunk->used == 1);
                break;
            }
        }
        assert(j != pool->chunk_count);
        if (j == pool->chunk_count)
        {
            //  Didn't find the chunk in the list, something went wrong
            assert(0);
            return 0;
        }
        return chunk->size;
    }

    //  Chunk has to have been from the large pool
    mem_pool_big* const pool = &allocator->big_pool;
    for (u64 i = 0; i < pool->ptr_count; ++i)
    {
        //  Check if ptr matches
        if (ptr != pool->pointer_array[i])
        {
            continue;
        }

        return pool->size_array[i];
    }

    //  ptr was not allocated using this allocator
    assert(0);
    return 0;
}
