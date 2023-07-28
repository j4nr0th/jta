//
// Created by jan on 14.5.2023.
//

#ifndef JTA_VK_STATE_H
#define JTA_VK_STATE_H
#include "../common/common.h"
#include "../jfw/window.h"
#include "../mem/vk_mem_allocator.h"
#include "gfxerr.h"
#include "bounding_box.h"

typedef struct vk_state_struct vk_state;
struct vk_state_struct
{
    VkRenderPass render_pass_3D;
    VkRenderPass render_pass_cf;
    VkRenderPass render_pass_UI;
    u32 framebuffer_count;
    VkFramebuffer* framebuffers;
    VkImageView* frame_views;
    u32 frame_index;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineLayout layout_3D;
    VkPipeline gfx_pipeline_3D;
    VkPipelineLayout layout_UI;
    VkPipeline gfx_pipeline_UI;
    vk_buffer_allocator* buffer_allocator;
    vk_buffer_allocation buffer_transfer;
    VkCommandBuffer transfer_cmd_buffer;
    VkFence fence_transfer_free;
    VkImageView depth_view;
    VkImage depth_img;
    VkDeviceMemory depth_mem;
    VkFormat depth_format;
    mtx4 view;
    uint32_t last_img_idx;
    VkCommandPool transfer_cmd_pool;
};

gfx_result vk_state_create(vk_state* p_state, const jfw_window_vk_resources* vk_resources);

void vk_state_destroy(vk_state* p_state, jfw_window_vk_resources* vk_resources);

gfx_result vk_transfer_memory_to_buffer(jfw_window_vk_resources* vk_resources, vk_state* p_state, vk_buffer_allocation* buffer, size_t size, void* data);

#endif //JTA_VK_STATE_H
