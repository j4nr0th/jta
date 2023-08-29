//
// Created by jan on 22.8.2023.
//

#include <jdm.h>
#include "textures.h"

gfx_result jta_texture_load(
        unsigned w, unsigned h, const void* ptr, const jta_vulkan_window_context* ctx, jta_texture_create_info info,
        jta_texture** p_out)
{
    JDM_ENTER_FUNCTION;
    jta_texture* const this = ill_jalloc(G_JALLOCATOR, sizeof(*this));
    if (!this)
    {
        JDM_ERROR("Could not allocate memory for texture struct");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    const uint32_t queue_indices[2] =
            {
                    ctx->queue_graphics_data.index,
                    ctx->queue_transfer_data.index,
            };
    VkImageCreateInfo create_info =
            {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .extent = {.width = w, .height = h, .depth = 1},
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .samples = info.samples,
            .format = info.format,
            .arrayLayers = 1,
            .mipLevels = 1,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .imageType = VK_IMAGE_TYPE_2D,
            .queueFamilyIndexCount = 2,
            .pQueueFamilyIndices = queue_indices,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .tiling = info.tiling,
            .flags = 0,
            };
    VkResult vk_res = jvm_image_create(ctx->vulkan_allocator, &create_info, 0, 0, 0, &this->img);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not allocate texture image, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
        ill_jfree(G_JALLOCATOR, this);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }

    gfx_result res = jta_texture_transition(
            ctx, this, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not transition the image to transfer layout, reason: %s", gfx_result_to_str(res));
        goto failed;
    }

    res = jta_vulkan_memory_to_texture(ctx, (uint64_t)(w) * h, ptr, this);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not transfer the memory to texture image, reason: %s", gfx_result_to_str(res));
        goto failed;
    }

    res = jta_texture_transition(
            ctx, this, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not transition the image from transfer layout, reason: %s", gfx_result_to_str(res));
        goto failed;
    }

    *p_out = this;
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

failed:
    jta_texture_destroy(this);
    JDM_LEAVE_FUNCTION;
    return res;
}

gfx_result jta_texture_transition(
        const jta_vulkan_window_context* ctx, const jta_texture* texture, VkFormat fmt, VkImageLayout layout_old,
        VkImageLayout layout_new)
{
    JDM_ENTER_FUNCTION;
    (void) fmt;
    VkCommandBuffer cmd_buffer;
    gfx_result res = jta_vulkan_queue_begin_transient(ctx, &ctx->queue_graphics_data, &cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not begin transient command buffer, reason: %s", gfx_result_to_str(res));
        goto failed;
    }

    VkAccessFlags access_src, access_dst;
    VkPipelineStageFlags src_stage, dst_stage;
    if (layout_old == VK_IMAGE_LAYOUT_UNDEFINED && layout_new == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        access_src = 0;
        access_dst = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (layout_old == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        access_dst = VK_ACCESS_TRANSFER_WRITE_BIT;
        access_src = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    else
    {
        JDM_ERROR("Unsupported transition format");
        res = GFX_RESULT_BAD_TRANSITION;
        goto failed;
    }

    VkImageMemoryBarrier barrier =
            {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = layout_old,
            .newLayout = layout_new,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = jvm_image_allocation_get_image(texture->img),
            .subresourceRange =
                    {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .baseArrayLayer = 0,
                    .levelCount = 1,
                    .layerCount = 1,
                    },
            .srcAccessMask = access_src,
            .dstAccessMask = access_dst,
            };
    vkCmdPipelineBarrier(cmd_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
    res = jta_vulkan_queue_end_transient(ctx, &ctx->queue_graphics_data, cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not end transition transient command, reason: %s", gfx_result_to_str(res));
        goto failed;
    }


failed:
    JDM_LEAVE_FUNCTION;
    return res;
}

gfx_result jta_vulkan_memory_to_texture(
        const jta_vulkan_window_context* ctx, uint64_t size, const void* ptr, const jta_texture* destination)
{
    JDM_ENTER_FUNCTION;
    gfx_result res;
    if (size > jvm_buffer_allocation_get_size(ctx->transfer_buffer))
    {
        JDM_ERROR("Transfer buffer's size (%zu) is insufficient for transfer of %zu bytes to texture", (size_t)jvm_buffer_allocation_get_size(ctx->transfer_buffer), (size_t)size);
        res = GFX_RESULT_LOW_TRANSFER_MEM;
        goto failed;
    }
    VkCommandBuffer cmd_buffer;
    res = jta_vulkan_queue_begin_transient(ctx, &ctx->queue_transfer_data, &cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not begin transient command, reason: %s", gfx_result_to_str(res));
        goto failed;
    }

    void* mapped_memory;
    size_t mapped_size;
    VkResult vk_res = jvm_buffer_map(ctx->transfer_buffer, &mapped_size, &mapped_memory);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not map transfer buffer to host memory, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
        res = GFX_RESULT_MAP_FAILED;
        goto failed;
    }
    memcpy(mapped_memory, ((const uint8_t*)ptr), size);
    vk_res = jvm_buffer_unmap(ctx->transfer_buffer);
    mapped_memory = (void*)0xCCCCCCCCCCCCCCCC;
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not unmap transfer buffer, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
        res = GFX_RESULT_MAP_FAILED;
        goto failed;
    }

    const VkBufferImageCopy cpy_info =
            {
                    .bufferOffset = 0,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,

                    .imageSubresource =
                            {
                            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .layerCount = 1,
                            .baseArrayLayer = 0,
                            .mipLevel = 0,
                            },
                    .imageOffset = {0, 0, 0},
                    .imageExtent = {destination->width, destination->height, 1},
            };
    vkCmdCopyBufferToImage(cmd_buffer, jvm_buffer_allocation_get_buffer(ctx->transfer_buffer), jvm_image_allocation_get_image(destination->img), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy_info);
    res = jta_vulkan_queue_end_transient(ctx, &ctx->queue_transfer_data, cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not end transient command, reason: %s", gfx_result_to_str(res));
        res = GFX_RESULT_BAD_VK_CALL;
        goto failed;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

failed:
    JDM_LEAVE_FUNCTION;
    return res;
}

gfx_result jta_texture_destroy(jta_texture* tex)
{
    JDM_ENTER_FUNCTION;

    jvm_image_destroy(tex->img);
    ill_jfree(G_JALLOCATOR, tex);

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}
