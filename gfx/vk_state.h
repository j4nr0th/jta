//
// Created by jan on 14.5.2023.
//

#ifndef JTB_VK_STATE_H
#define JTB_VK_STATE_H
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
    vk_buffer_allocation buffer_vtx_geo, buffer_vtx_mod, buffer_idx, buffer_transfer, buffer_uniform;
    VkFence fence_transfer_free;
    VkDescriptorSetLayout ubo_layout;
    ubo_3d** p_mapped_array;
    VkDescriptorSet* desc_set;
    VkDescriptorPool desc_pool;
    VkImageView depth_view;
    VkImage depth_img;
    VkDeviceMemory depth_mem;
    VkFormat depth_format;
    void* mesh;
    mtx4 view;
    const jtb_point_list* point_list;
};

gfx_result vk_state_create(vk_state* p_state, const jfw_window_vk_resources* vk_resources);

void vk_state_destroy(vk_state* p_state, jfw_window_vk_resources* vk_resources);

gfx_result vk_transfer_memory_to_buffer(jfw_window_vk_resources* vk_resources, vk_state* p_state, vk_buffer_allocation* buffer, size_t size, void* data);

#endif //JTB_VK_STATE_H
