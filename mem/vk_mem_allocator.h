//
// Created by jan on 14.5.2023.
//

#ifndef JTB_VK_MEM_ALLOCATOR_H
#define JTB_VK_MEM_ALLOCATOR_H
#include "../common/common.h"

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
    VkDeviceSize offset_alignment;
};

vk_buffer_allocator*
vk_buffer_allocator_create(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize pool_init_size);

i32
vk_buffer_allocate(
        vk_buffer_allocator* allocator, VkDeviceSize element_count, VkDeviceSize element_size,
        VkMemoryPropertyFlags props, VkBufferUsageFlags usage, VkSharingMode sharing_mode,
        vk_buffer_allocation* p_out);

void vk_buffer_deallocate(vk_buffer_allocator* allocator, vk_buffer_allocation* allocation);

void vk_buffer_allocator_destroy(vk_buffer_allocator* allocator);

i32 vk_buffer_reserve(
        vk_buffer_allocator* allocator, VkDeviceSize size, VkMemoryPropertyFlags props, VkBufferUsageFlags usage,
        VkSharingMode sharing_mode);

const VkPhysicalDeviceMemoryProperties* vk_allocator_mem_props(vk_buffer_allocator* allocator);

void* vk_map_allocation(const vk_buffer_allocation* allocation);

void vk_unmap_allocation(void* ptr_mapped, const vk_buffer_allocation* allocation);

#endif //JTB_VK_MEM_ALLOCATOR_H
