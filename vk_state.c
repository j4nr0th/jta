//
// Created by jan on 14.5.2023.
//

#include "vk_state.h"
#include "jfw/error_system/error_stack.h"

//VkRenderPass render_pass_3D;
//VkRenderPass render_pass_UI;
//u32 framebuffer_count;
//VkFramebuffer* framebuffers;
//VkImageView* frame_views;
//u32 frame_index;
//VkViewport viewport;
//VkRect2D scissor;
//VkPipelineLayout layout_3D;
//VkPipeline gfx_pipeline_3D;
//VkPipelineLayout layout_UI;
//VkPipeline gfx_pipeline_UI;
//vk_buffer_allocator* buffer_allocator;
//vk_buffer_allocation buffer_device_local, buffer_transfer, buffer_uniform;
//VkDescriptorSetLayout ubo_layout;
//void** p_mapped_array;
//VkDescriptorSet* desc_set;
//VkDescriptorPool desc_pool;
//VkImageView depth_view;
//VkImage depth_img;
//VkDeviceMemory depth_mem;
//VkFormat depth_format;

jfw_res vk_state_create(vk_state* const p_state, const jfw_window_vk_resources* const vk_resources)
{
    jfw_res return_value = jfw_res_success;
    memset(p_state, 0, sizeof*p_state);
    //  Create buffer allocators
    vk_buffer_allocator* allocator = vk_buffer_allocator_create(vk_resources->device, vk_resources->physical_device, 1 << 20);
    if (!allocator)
    {
        JFW_ERROR("Failed creating Vulkan buffer allocator");
        return jfw_res_error;
    }
    p_state->buffer_allocator = allocator;


    u32 max_samples;
    VkResult vk_res;
    //  Find the appropriate sample flag
    {
        VkSampleCountFlags flags = vk_resources->sample_flags;
        if (flags & VK_SAMPLE_COUNT_64_BIT) max_samples = VK_SAMPLE_COUNT_64_BIT;
        else if (flags & VK_SAMPLE_COUNT_32_BIT) max_samples = VK_SAMPLE_COUNT_32_BIT;
        else if (flags & VK_SAMPLE_COUNT_32_BIT) max_samples = VK_SAMPLE_COUNT_32_BIT;
        else if (flags & VK_SAMPLE_COUNT_16_BIT) max_samples = VK_SAMPLE_COUNT_16_BIT;
        else if (flags & VK_SAMPLE_COUNT_8_BIT) max_samples = VK_SAMPLE_COUNT_8_BIT;
        else if (flags & VK_SAMPLE_COUNT_4_BIT) max_samples = VK_SAMPLE_COUNT_4_BIT;
        else if (flags & VK_SAMPLE_COUNT_2_BIT) max_samples = VK_SAMPLE_COUNT_2_BIT;
        else max_samples = VK_SAMPLE_COUNT_1_BIT;
        assert(flags & VK_SAMPLE_COUNT_1_BIT);
    }

    //  Create the depth buffer
    {
        VkImage depth_image;
        VkDeviceMemory depth_memory;
        VkImageView depth_view;
        VkFormat depth_format;
        i32 has_depth;

        const VkFormat possible_depth_formats[3] = {VK_FORMAT_D24_UNORM_S8_UINT | VK_FORMAT_D32_SFLOAT_S8_UINT | VK_FORMAT_D32_SFLOAT};
        const u32 n_possible_depth_formats = sizeof(possible_depth_formats) / sizeof(*possible_depth_formats);
        VkFormatProperties props;
        u32 i;
        for (i = 0; i < n_possible_depth_formats; ++i)
        {
            vkGetPhysicalDeviceFormatProperties(vk_resources->physical_device, possible_depth_formats[i], &props);
            if (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT & props.optimalTilingFeatures && props.optimalTilingFeatures == VK_IMAGE_TILING_OPTIMAL)
            {
                break;
            }
        }
        if (i == n_possible_depth_formats)
        {
            JFW_ERROR("Could not find a good image format for the depth buffer");
            return jfw_res_vk_fail;
        }
        depth_format = possible_depth_formats[i];
        has_depth = (depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT);
        VkImageCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .extent = { .width = vk_resources->extent.width, .height = vk_resources->extent.height, .depth = 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .format = depth_format,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                };
        vk_res = vkCreateImage(vk_resources->device, &create_info, NULL, &depth_image);
        if (vk_res != VK_SUCCESS)
        {
            JFW_ERROR("Could not create depth buffer image, reason: %s", jfw_vk_error_msg(vk_res));
            return jfw_res_vk_fail;
        }
        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(vk_resources->device, depth_image, &mem_req);
        u32 visible_queue[] = {vk_resources->i_trs_queue, vk_resources->i_gfx_queue, vk_resources->i_prs_queue};

        vk_buffer_reserve(allocator, 0, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_SHARING_MODE_EXCLUSIVE, )
        VkMemoryAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = mem_req.size,
                .
                };

    }

    return return_value;
}
