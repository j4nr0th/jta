//
// Created by jan on 14.5.2023.
//

#include "vk_mem_allocator.h"
#include "../../jfw/error_system/error_stack.h"

typedef struct allocation_chunk_struct allocation_chunk;
struct allocation_chunk_struct
{
    u64 used:1;
    u64 offset:63;
    u64 size;
};

typedef struct buffer_pool_struct buffer_pool;
#define POOL_CHUNK_MAX (1 << 8)
struct buffer_pool_struct
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkMemoryRequirements requirements;
    VkBufferUsageFlags usage;
    VkSharingMode sharing_mode;
    u32 heap_index;
    u32 chunk_count;
    allocation_chunk chunk_list[POOL_CHUNK_MAX];
    u32 n_qfi;
    u32* v_qfi;
};

struct vk_buffer_allocator_struct
{
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceMemoryProperties mem_props;
    VkDeviceSize pool_init_size;
    u32 pool_count;
    u32 pool_capacity;
    buffer_pool** pools;
    VkDeviceSize total_used;
    VkDeviceSize total_free;
};


vk_buffer_allocator*
vk_buffer_allocator_create(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize pool_init_size)
{
    vk_buffer_allocator* this = malloc(sizeof*this);
    if (!this)
    {
        return NULL;
    }
    memset(this, 0, sizeof*this);
    this->pool_capacity = 8;
    this->pools = calloc(this->pool_capacity, sizeof(buffer_pool*));
    if (!this->pools)
    {
        free(this);
        return NULL;
    }
    this->pool_count = 0;
    this->pool_init_size = pool_init_size;
    this->device = device;
    this->physical_device = physical_device;

    vkGetPhysicalDeviceMemoryProperties(physical_device, &this->mem_props);

    return this;
}

void vk_buffer_allocator_destroy(vk_buffer_allocator* allocator)
{
    for (u32 i = 0; i < allocator->pool_count; ++i)
    {
        buffer_pool* pool = allocator->pools[i];
        assert(pool);
        vkFreeMemory(allocator->device, pool->memory, NULL);
        vkDestroyBuffer(allocator->device, pool->buffer, NULL);
        free(pool);
        allocator->pools[i] = NULL;
    }
    free(allocator->pools);
    memset(allocator, 0, sizeof(*allocator));
    free(allocator);
}

i32 vk_buffer_allocate(
        vk_buffer_allocator* const allocator, VkDeviceSize size, const VkFlags type_bits,
        const VkMemoryPropertyFlags props, VkBufferUsageFlags usage, VkSharingMode sharing_mode,
        u32 n_queue_family_indices, const u32* p_queue_family_indices, vk_buffer_allocation* const p_out)
{
    buffer_pool* pool;
    const VkPhysicalDeviceMemoryProperties* const mem_props = &allocator->mem_props;
    u32 mem_index, j;
    //  Search through all memory heap types available
    for (mem_index = 0; mem_index < mem_props->memoryTypeCount; ++mem_index)
    {
        if (type_bits & (1 << mem_index) && (mem_props->memoryTypes[mem_index].propertyFlags & props))
        {
            break;
        }
    }
    //  None were found, return error
    if (mem_index == mem_props->memoryTypeCount)
    {
        assert(0);
        return -1;
    }
    //  Check if we have a buffer with desired properties
    for (j = 0; j < allocator->pool_count; ++j)
    {
        pool = allocator->pools[j];
        //  Check if the pool does have the desired heap index
        if (pool->heap_index != mem_index)
        {
            continue;
        }
        //  Check if the pool has the desired usage flags
        if ((pool->usage & usage) != usage)
        {
            continue;
        }
        //  Check if the sharing mode is correct
        if (pool->sharing_mode != sharing_mode)
        {
            continue;
        }

        u32 queue_index_matches = 0;
        //  Check the queue family indices
        for (u32 k = 0; k < n_queue_family_indices && queue_index_matches < n_queue_family_indices; ++k)
        {
            for (u32 l = 0; l < pool->n_qfi; ++l)
            {
                if (p_queue_family_indices[k] == pool->v_qfi[l])
                {
                    queue_index_matches += 1;
                    break;
                }
            }
        }
        //  Check that the number of indices match
        if (queue_index_matches != n_queue_family_indices)
        {
            continue;
        }

        allocation_chunk* chunk;
        //  Try and find a chunk with proper size to fit the allocation into
        for (u32 k = 0; k < pool->chunk_count; ++k)
        {
            chunk = pool->chunk_list + k;
            if (chunk->used != 0 || chunk->size < size)
            {
                //  Can't use the chunk, either because it is used, or because it is too small
                continue;
            }
            VkDeviceSize remainder = chunk->size - size;
            if (remainder && pool->chunk_count < POOL_CHUNK_MAX)
            {
                //  Insert a new empty chunk in the list
                //  Todo: test this works (namely the memset part)
                memmove(
                        pool->chunk_list + k + 1, pool->chunk_list + k + 2,
                        sizeof(*pool->chunk_list) * (pool->chunk_count - k - 2));
                pool->chunk_list[k + 1] = (allocation_chunk) { .size = remainder, .used = 0, .offset = chunk->offset +
                                                                                                       size };
                pool->chunk_count += 1;
                chunk->size = size;
                assert(pool->chunk_list[k + 1].offset + pool->chunk_list[k + 1].size == pool->chunk_list[k + 2].offset);
            }
            chunk->used = 1;
            *p_out = (vk_buffer_allocation) { .offset = chunk->offset, .size = chunk->size, .device = allocator->device, .buffer = pool->buffer, .memory = pool->memory, .pool_index = j };
            return 0;
        }
    }

    jfw_res jfw_result;
    if (allocator->pool_count == allocator->pool_capacity)
    {
        const u32 new_capacity = allocator->pool_capacity << 1;
        jfw_result = jfw_realloc(new_capacity * sizeof(buffer_pool*), &allocator->pools);
        if (!jfw_success(jfw_result))
        {
            JFW_ERROR("Could not reallocate %zu bytes of memory for pool list", new_capacity * sizeof(buffer_pool*));
            return -1;
        }
        allocator->pool_capacity = new_capacity;
    }
    jfw_result = jfw_malloc(sizeof *pool, &pool);
    if (!jfw_success(jfw_result))
    {
        JFW_ERROR("Could not allocate memory for the new buffer pool");
        return -1;
    }

    u32* v_qfi;
    jfw_result =  jfw_calloc(n_queue_family_indices, sizeof(*v_qfi), &v_qfi);
    if (!jfw_success(jfw_result))
    {
        JFW_ERROR("Failed allocating memory for the pool's queue list");
        jfw_free(&pool);
        return -1;
    }
    memcpy(v_qfi, p_queue_family_indices, sizeof(*v_qfi) * n_queue_family_indices);

    //  No pool was good, make a new one
    const VkDeviceSize new_pool_size = size > allocator->pool_init_size ? size : allocator->pool_init_size;
    assert(new_pool_size);
    //  Create the buffer with proper flags and settings
    VkBufferCreateInfo create_info =
            {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = new_pool_size,
            .usage = usage,
            .flags = 0,
            .sharingMode = sharing_mode,
            .queueFamilyIndexCount = n_queue_family_indices,
            .pQueueFamilyIndices = p_queue_family_indices,
            .pNext = NULL,
            };
    VkBuffer new_buffer;
    VkResult vk_res = vkCreateBuffer(allocator->device, &create_info, NULL, &new_buffer);
    if (vk_res != VK_SUCCESS)
    {
        JFW_ERROR("Failed creating vk_buffer, reason: %s", jfw_vk_error_msg(vk_res));
        jfw_free(&v_qfi);
        jfw_free(&pool);
        return -1;
    }
    //  Query buffer's memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(allocator->device, new_buffer, &mem_requirements);

    //  Allocate device memory
    VkDeviceMemory mem;
    VkMemoryAllocateInfo alloc_info =
            {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .memoryTypeIndex = mem_index,
            .allocationSize = mem_requirements.size,
            };
    vk_res = vkAllocateMemory(allocator->device, &alloc_info, NULL, &mem);
    if (vk_res != VK_SUCCESS)
    {
        JFW_ERROR("Failed creating vk_buffer, reason: %s", jfw_vk_error_msg(vk_res));
        vkDestroyBuffer(allocator->device, new_buffer, NULL);
        jfw_free(&v_qfi);
        jfw_free(&pool);
        return -1;
    }

    //  Bind the buffer to memory
    vkBindBufferMemory(allocator->device, new_buffer, mem, 0);

    //  Fill the pool struct with new info
    memset(pool, 0, sizeof(*pool));
    pool->requirements = mem_requirements;
    pool->chunk_count = 1;
    pool->buffer = new_buffer;
    pool->v_qfi = v_qfi;
    pool->n_qfi = n_queue_family_indices;
    pool->sharing_mode = sharing_mode;
    pool->usage = usage;
    pool->memory = mem;
    pool->heap_index = mem_index;

    allocator->pools[allocator->pool_count++] = pool;
    //  Allocate the chunk that was needed
    if (size >= allocator->pool_init_size)
    {
        pool->chunk_count = 1;
        pool->chunk_list[0] = (allocation_chunk){.size = size, .offset = 0, .used = 1};
        *p_out = (vk_buffer_allocation){.offset = 0, .size = size, .device = allocator->device, .memory = pool->memory, .buffer = pool->buffer, .pool_index = pool->chunk_count - 1};
    }
    else
    {
        pool->chunk_count = 2;
        pool->chunk_list[0] = (allocation_chunk){.size = size, .offset = 0, .used = 1};
        pool->chunk_list[1] = (allocation_chunk){.size = new_pool_size - size, .offset = size, .used = 0};
        *p_out = (vk_buffer_allocation){.offset = 0, .size = size, .device = allocator->device, .memory = pool->memory, .buffer = pool->buffer, .pool_index = pool->chunk_count - 1};
    }

    return 0;
}


void vk_buffer_deallocate(vk_buffer_allocator* allocator, vk_buffer_allocation* allocation)
{
    assert(allocator);
    assert(allocation);
    buffer_pool* const pool = allocator->pools[allocation->pool_index];
    if (allocation->pool_index >= allocator->pool_count)
    {
        JFW_ERROR("Pool index is higher than the pool count (index is %u, should be less than %u)",
                  allocation->pool_index, allocator->pool_count);
        return;
    }
    if (allocation->memory != pool->memory || allocation->buffer != pool->buffer)
    {
        JFW_ERROR("Memory and buffer of the allocation (%p and %p) don't match the pool's values (%p and %p)",
                  allocation->memory,
                  allocation->buffer, pool->memory, pool->buffer);
        return;
    }
    //  Find the chunk it belongs to
    allocation_chunk* chunk = NULL;
    u32 i;
    for (i = 0; i < pool->chunk_count; ++i)
    {
        if (pool->chunk_list[i].offset == allocation->offset && pool->chunk_list[i].size == allocation->size &&
            pool->chunk_list[i].used == 1)
        {
            chunk = pool->chunk_list + i;
            break;
        }
    }
    if (!chunk)
    {
        JFW_ERROR("Could not find the allocation chunk in the pool's chunk list");
        return;
    }

    //  Merge with previous chunk if possible
    if (i > 0 && pool->chunk_list[i].used == 0)
    {
        pool->chunk_list[i - 1].size += chunk->size;
        memmove(pool->chunk_list + i, pool->chunk_list + i + 1, sizeof(*pool->chunk_list) * (pool->chunk_count - i - 1));
        i -= 1;
        pool->chunk_count -= 1;
    }
    //  Merge with next chunk if possible
    if (i < pool->chunk_count - 1)
    {
        chunk->size += pool->chunk_list[i + 1].size;
        memmove(pool->chunk_list + i + 1, pool->chunk_list + i + 2, sizeof(*pool->chunk_list) * (pool->chunk_count - i - 2));
        pool->chunk_count -= 1;
    }
}

i32 vk_buffer_reserve(
        vk_buffer_allocator* allocator, VkDeviceSize size, VkFlags type_bits, VkMemoryPropertyFlags props,
        VkBufferUsageFlags usage, VkSharingMode sharing_mode, u32 n_queue_family_indices,
        const u32* p_queue_family_indices)
{
    //  Make sure that the allocation can be made for the specified size
    buffer_pool* pool;
    const VkPhysicalDeviceMemoryProperties* const mem_props = &allocator->mem_props;
    u32 mem_index, j;
    //  Search through all memory heap types available
    for (mem_index = 0; mem_index < mem_props->memoryTypeCount; ++mem_index)
    {
        if (type_bits & (1 << mem_index) && (mem_props->memoryTypes[mem_index].propertyFlags & props))
        {
            break;
        }
    }
    //  None were found, return error
    if (mem_index == mem_props->memoryTypeCount)
    {
        assert(0);
        return -1;
    }
    //  Check if we have a buffer with desired properties
    for (j = 0; j < allocator->pool_count; ++j)
    {
        pool = allocator->pools[j];
        //  Check if the pool does have the desired heap index
        if (pool->heap_index != mem_index)
        {
            continue;
        }
        //  Check if the pool has the desired usage flags
        if ((pool->usage & usage) != usage)
        {
            continue;
        }
        //  Check if the sharing mode is correct
        if (pool->sharing_mode != sharing_mode)
        {
            continue;
        }

        u32 queue_index_matches = 0;
        //  Check the queue family indices
        for (u32 k = 0; k < n_queue_family_indices && queue_index_matches < n_queue_family_indices; ++k)
        {
            for (u32 l = 0; l < pool->n_qfi; ++l)
            {
                if (p_queue_family_indices[k] == pool->v_qfi[l])
                {
                    queue_index_matches += 1;
                    break;
                }
            }
        }
        //  Check that the number of indices match
        if (queue_index_matches != n_queue_family_indices)
        {
            continue;
        }

        allocation_chunk* chunk;
        //  Try and find a chunk with proper size to fit the allocation into
        for (u32 k = 0; k < pool->chunk_count; ++k)
        {
            chunk = pool->chunk_list + k;
            if (chunk->used != 0 || chunk->size < size)
            {
                //  Can't use the chunk, either because it is used, or because it is too small
                continue;
            }

            return 0;
        }
    }

    jfw_res jfw_result;
    if (allocator->pool_count == allocator->pool_capacity)
    {
        const u32 new_capacity = allocator->pool_capacity << 1;
        jfw_result = jfw_realloc(new_capacity * sizeof(buffer_pool*), &allocator->pools);
        if (!jfw_success(jfw_result))
        {
            JFW_ERROR("Could not reallocate %zu bytes of memory for pool list", new_capacity * sizeof(buffer_pool*));
            return -1;
        }
        allocator->pool_capacity = new_capacity;
    }
    jfw_result = jfw_malloc(sizeof *pool, &pool);
    if (!jfw_success(jfw_result))
    {
        JFW_ERROR("Could not allocate memory for the new buffer pool");
        return -1;
    }

    u32* v_qfi;
    jfw_result =  jfw_calloc(n_queue_family_indices, sizeof(*v_qfi), &v_qfi);
    if (!jfw_success(jfw_result))
    {
        JFW_ERROR("Failed allocating memory for the pool's queue list");
        jfw_free(&pool);
        return -1;
    }
    memcpy(v_qfi, p_queue_family_indices, sizeof(*v_qfi) * n_queue_family_indices);

    //  No pool was good, make a new one
    const VkDeviceSize new_pool_size = size > allocator->pool_init_size ? size : allocator->pool_init_size;
    assert(new_pool_size);
    //  Create the buffer with proper flags and settings
    VkBufferCreateInfo create_info =
            {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                    .size = new_pool_size,
                    .usage = usage,
                    .flags = 0,
                    .sharingMode = sharing_mode,
                    .queueFamilyIndexCount = n_queue_family_indices,
                    .pQueueFamilyIndices = p_queue_family_indices,
                    .pNext = NULL,
            };
    VkBuffer new_buffer;
    VkResult vk_res = vkCreateBuffer(allocator->device, &create_info, NULL, &new_buffer);
    if (vk_res != VK_SUCCESS)
    {
        JFW_ERROR("Failed creating vk_buffer, reason: %s", jfw_vk_error_msg(vk_res));
        jfw_free(&v_qfi);
        jfw_free(&pool);
        return -1;
    }
    //  Query buffer's memory requirements
    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(allocator->device, new_buffer, &mem_requirements);

    //  Allocate device memory
    VkDeviceMemory mem;
    VkMemoryAllocateInfo alloc_info =
            {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .memoryTypeIndex = mem_index,
                    .allocationSize = mem_requirements.size,
            };
    vk_res = vkAllocateMemory(allocator->device, &alloc_info, NULL, &mem);
    if (vk_res != VK_SUCCESS)
    {
        JFW_ERROR("Failed creating vk_buffer, reason: %s", jfw_vk_error_msg(vk_res));
        vkDestroyBuffer(allocator->device, new_buffer, NULL);
        jfw_free(&v_qfi);
        jfw_free(&pool);
        return -1;
    }

    //  Bind the buffer to memory
    vkBindBufferMemory(allocator->device, new_buffer, mem, 0);

    //  Fill the pool struct with new info
    memset(pool, 0, sizeof(*pool));
    pool->requirements = mem_requirements;
    pool->chunk_count = 1;
    pool->buffer = new_buffer;
    pool->v_qfi = v_qfi;
    pool->n_qfi = n_queue_family_indices;
    pool->sharing_mode = sharing_mode;
    pool->usage = usage;
    pool->memory = mem;
    pool->heap_index = mem_index;

    pool->chunk_list[0] = (allocation_chunk){.size = new_pool_size, .offset = 0, .used = 0};

    allocator->pools[allocator->pool_count++] = pool;


    return 0;
}
