//
// Created by jan on 14.5.2023.
//

#ifndef JTB_VK_MEM_ALLOCATOR_H
#define JTB_VK_MEM_ALLOCATOR_H
#include "../common.h"

typedef struct vk_buffer_allocator_struct vk_buffer_allocator;
typedef struct vk_buffer_allocation_struct vk_buffer_allocation;
struct vk_buffer_allocation_struct
{
    u32 pool_index;
    VkDevice device;
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    VkDeviceSize size;
};

vk_buffer_allocator*
vk_buffer_allocator_create(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize pool_init_size);

i32
vk_buffer_allocate(
        vk_buffer_allocator* allocator, VkDeviceSize size, VkFlags type_bits,
        VkMemoryPropertyFlags props, VkBufferUsageFlags usage, VkSharingMode sharing_mode,
        u32 n_queue_family_indices, const u32* p_queue_family_indices, vk_buffer_allocation* p_out);

void vk_buffer_deallocate(vk_buffer_allocator* allocator, vk_buffer_allocation* allocation);

void vk_buffer_allocator_destroy(vk_buffer_allocator* allocator);

i32 vk_buffer_reserve(vk_buffer_allocator* allocator, VkDeviceSize size, VkFlags type_bits,
                      VkMemoryPropertyFlags props, VkBufferUsageFlags usage, VkSharingMode sharing_mode,
                      u32 n_queue_family_indices, const u32* p_queue_family_indices);

#endif //JTB_VK_MEM_ALLOCATOR_H
