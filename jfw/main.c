//
// Created by jan on 16.1.2023.
//
#include "window.h"
#include "widget-base.h"
#include "error_system/error_stack.h"
#include <stdio.h>
#include <unistd.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>

#include "practice_shaders/vert.spv"
#include "practice_shaders/frag.spv"

#include "gfx_math.h"

#include <time.h>

static VkResult
record_cmd_queue(
        VkCommandBuffer cmd_buffer, VkRenderPass render_pass, VkFramebuffer framebuffer, VkExtent2D extent,
        const VkViewport viewport, const VkRect2D scissor, VkPipeline pipeline, const VkBuffer vtx_buffer,
        VkBuffer idx_buffer, VkPipelineLayout layout, VkDescriptorSet const* desc_set)
{
    VkResult vk_result;
    {
        VkCommandBufferBeginInfo begin_info =
                {
                        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                        .pInheritanceInfo = NULL,
                        .flags = 0,
                        .pNext = NULL,
                };
        vk_result = vkBeginCommandBuffer(cmd_buffer, &begin_info);
        if (vk_result != VK_SUCCESS)
        {
            return vk_result;
        }
    }
    {
        VkClearValue clear_color =
                {
                .color = {0.0f, 0.0f, 0.0f, 1.0f},
                };
        VkClearValue clear_ds =
                {
                .depthStencil = {1.0f, 0},
                };
        VkClearValue clear_array[2] = {clear_color, clear_ds};
        VkRenderPassBeginInfo render_pass_info =
                {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = render_pass,
                .framebuffer = framebuffer,
                .renderArea.offset = {0, 0},
                .renderArea.extent = extent,
                .clearValueCount = 2,
                .pClearValues = clear_array,
                };
        vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    }

    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);

    VkDeviceSize device_sizes[] = {0};
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &vtx_buffer, device_sizes);
    vkCmdBindIndexBuffer(cmd_buffer, idx_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, desc_set, 0, NULL);
    vkCmdDrawIndexed(cmd_buffer, 12, 1, 0, 0, 0);
    vkCmdEndRenderPass(cmd_buffer);

    vk_result = vkEndCommandBuffer(cmd_buffer);
    if (vk_result != VK_SUCCESS)
    {
        return vk_result;
    }
    return VK_SUCCESS;
}

struct draw_info_struct
{
    VkRenderPass render_pass;
    VkFramebuffer* framebuffers;
    u32 frame_index;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipeline gfx_pipeline;
    VkPipelineLayout layout;
    VkBuffer vtx_buffer, stage_buffer, idx_buffer;
    VkDeviceMemory vtx_mem, stage_mem, idx_mem;
    VkDescriptorSetLayout ubo_layout;
    u32 n_uniform_buffers;
    VkBuffer* uniform_buffers;
    VkDeviceMemory* uniform_mem;
    void** p_mapped_array;
    VkDescriptorSet* desc_set;
    VkDescriptorPool desc_pool;
    VkImageView depth_view;
    VkImage depth_img;
    VkDeviceMemory depth_mem;
    VkFormat depth_format;
};


struct UBO_struct
{
    mtx4 model;
    mtx4 view;
    mtx4 proj;
};



static i32 find_memory_type(const jfw_window_vk_resources* res, u32 type_filter, VkMemoryPropertyFlags props)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(res->physical_device, &mem_props);
    for (u32 i = 0; i < mem_props.memoryTypeCount; ++i)
    {
        if (type_filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & props))
        {
            return (i32)i;
        }
    }
    return -1;
}

static void update_uniforms(const jfw_window_vk_resources* vk_res, struct draw_info_struct* draw, u32 img_idx, u32 w, u32 h)
{
    static clock_t t = -1;
    f32 dt;
    if (t == -1)
    {
        dt = 0;
        t = clock();
    }
    else
    {
        clock_t new_t = clock();
        dt = (1000.0f * (f32)(new_t - t)) / (f32)CLOCKS_PER_SEC;
        printf("DT: %g\n", dt);
    }
    f32 a = M_PI / 10.0f * dt;
    f32 mag = 20.f;
    mtx4 m = mtx4_identity;
    struct UBO_struct ubo =
            {
            .model = mtx4_identity,//euler_rotation_z(a),
            .proj = mtx4_projection(M_PI_2, ((f32)w)/((f32)h), 0.5f * 1980.0f/(f32)w),
            .view = mtx4_view_look_at(
                    (vec4){.y = -3, .x = 0, .z = -10.0f},
                    (vec4){.x = 0.0f, .y = 0.0f, .z = 0.0f},
                    M_PI
                    ),
            };
    memcpy(draw->p_mapped_array[img_idx], &ubo, sizeof(ubo));
}

static jfw_res draw_the_window(jfw_widget* this)
{
    const jfw_window_vk_resources* ctx = jfw_window_get_vk_resources(this->window);
    void* const ptr = jfw_widget_get_user_pointer(this);
    struct draw_info_struct* draw = ptr;
    u32 i_frame = draw->frame_index;
    VkResult vk_res =  vkWaitForFences(ctx->device, 1, ctx->swap_fences + i_frame, VK_TRUE, UINT64_MAX);
    assert(vk_res == VK_SUCCESS);
    u32 img_index;
    vk_res = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, ctx->sem_img_available[i_frame], VK_NULL_HANDLE, &img_index);
    assert(vk_res == VK_SUCCESS);
    switch (vk_res)
    {
    case VK_NOT_READY: return jfw_res_success;
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR: break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    {

        jfw_res res = jfw_window_update_swapchain(this->window);
        assert(res == jfw_res_success);
        VkImageView depth_view;
        VkImage depth_img;
        VkDeviceMemory depth_mem;
        {
            VkImageCreateInfo create_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                            .imageType = VK_IMAGE_TYPE_2D,
                            .extent.width = ctx->extent.width,
                            .extent.height = ctx->extent.height,
                            .extent.depth = 1,
                            .mipLevels = 1,
                            .arrayLayers = 1,
                            .format = draw->depth_format,
                            .tiling = VK_IMAGE_TILING_OPTIMAL,
                            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    };
            vk_res = vkCreateImage(ctx->device, &create_info, NULL, &depth_img);
            assert(vk_res == VK_SUCCESS);
            VkMemoryRequirements mem_req;
            vkGetImageMemoryRequirements(ctx->device, depth_img, &mem_req);
            VkMemoryAllocateInfo alloc_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                            .allocationSize = mem_req.size,
                            .memoryTypeIndex = find_memory_type(ctx, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                    };
            vk_res = vkAllocateMemory(ctx->device, &alloc_info, NULL, &depth_mem);
            assert(vk_res == VK_SUCCESS);
            vk_res = vkBindImageMemory(ctx->device, depth_img, depth_mem, 0);
            assert(vk_res == VK_SUCCESS);
            VkImageViewCreateInfo create_info_view =
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = draw->depth_format,

                            .components.a = VK_COMPONENT_SWIZZLE_A,
                            .components.b = VK_COMPONENT_SWIZZLE_B,
                            .components.g = VK_COMPONENT_SWIZZLE_G,
                            .components.r = VK_COMPONENT_SWIZZLE_R,

                            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | ((draw->depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || draw->depth_format == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0),
                            .subresourceRange.baseMipLevel = 0,
                            .subresourceRange.levelCount = 1,
                            .subresourceRange.baseArrayLayer = 0,
                            .subresourceRange.layerCount = 1,
                            .image = depth_img,
                    };
            vk_res = vkCreateImageView(ctx->device, &create_info_view, NULL, &depth_view);
            assert(vk_res == VK_SUCCESS);

        }

        vkDestroyImageView(ctx->device, draw->depth_view, NULL);
        vkDestroyImage(ctx->device, draw->depth_img, NULL);
        vkFreeMemory(ctx->device, draw->depth_mem, NULL);
        draw->depth_view = depth_view;
        draw->depth_img = depth_img;
        draw->depth_mem = depth_mem;



        VkFramebufferCreateInfo fb_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                        .renderPass = draw->render_pass,
                        .attachmentCount = 2,
                        .width = ctx->extent.width,
                        .height = ctx->extent.height,
                        .layers = 1,
                };
        for (u32 i = 0; i < ctx->n_images; ++i)
        {
            VkImageView attachments[2] = { ctx->views[i], depth_view};
            vkDestroyFramebuffer(ctx->device, draw->framebuffers[i], NULL);
            fb_create_info.pAttachments = attachments;
            vk_res = vkCreateFramebuffer(ctx->device, &fb_create_info, NULL, draw->framebuffers + i);
            assert(vk_res == VK_SUCCESS);
        }
        draw->viewport.width = (f32)ctx->extent.width;
        draw->viewport.height = (f32)ctx->extent.height;
        draw->scissor.extent = ctx->extent;

        printf("Recreated swapchain\n");
    }
        break;
    default: { assert(0); } return jfw_res_error;
    }


    vk_res = vkResetFences(ctx->device, 1, ctx->swap_fences + i_frame);
    assert(vk_res == VK_SUCCESS);
    vk_res = vkResetCommandBuffer(ctx->cmd_buffers[i_frame], 0);
    assert(vk_res == VK_SUCCESS);
    vk_res = record_cmd_queue(
            ctx->cmd_buffers[i_frame], draw->render_pass, draw->framebuffers[img_index], ctx->extent, draw->viewport,
            draw->scissor, draw->gfx_pipeline, draw->vtx_buffer, draw->idx_buffer, draw->layout, draw->desc_set + i_frame);
    assert(vk_res == VK_SUCCESS);

    update_uniforms(ctx, draw, i_frame, ctx->extent.width, ctx->extent.height);

    VkPipelineStageFlags stage_flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = ctx->cmd_buffers + i_frame,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = ctx->sem_present + i_frame,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = ctx->sem_img_available + i_frame,
        .pWaitDstStageMask = stage_flags,
    };
    vk_res = vkQueueSubmit(ctx->queue_gfx, 1, &submit_info, ctx->swap_fences[i_frame]);
    assert(vk_res == VK_SUCCESS);
    VkPresentInfoKHR present_info =
            {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains = &ctx->swapchain,
            .pImageIndices = &img_index,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = ctx->sem_present + i_frame,
            };
    vk_res = vkQueuePresentKHR(ctx->queue_present, &present_info);
    draw->frame_index = (i_frame + 1) % ctx->n_frames_in_flight;
    switch (vk_res)
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR: printf("Draw fn worked!\n"); return jfw_res_success;
    case VK_ERROR_OUT_OF_DATE_KHR:
    {
        jfw_res res = jfw_window_update_swapchain(this->window);
        assert(res == jfw_res_success);

        VkImageView depth_view;
        VkImage depth_img;
        VkDeviceMemory depth_mem;
        {
            VkImageCreateInfo create_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                            .imageType = VK_IMAGE_TYPE_2D,
                            .extent.width = ctx->extent.width,
                            .extent.height = ctx->extent.height,
                            .extent.depth = 1,
                            .mipLevels = 1,
                            .arrayLayers = 1,
                            .format = draw->depth_format,
                            .tiling = VK_IMAGE_TILING_OPTIMAL,
                            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                            .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                            .samples = VK_SAMPLE_COUNT_1_BIT,
                            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                    };
            vk_res = vkCreateImage(ctx->device, &create_info, NULL, &depth_img);
            assert(vk_res == VK_SUCCESS);
            VkMemoryRequirements mem_req;
            vkGetImageMemoryRequirements(ctx->device, depth_img, &mem_req);
            VkMemoryAllocateInfo alloc_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                            .allocationSize = mem_req.size,
                            .memoryTypeIndex = find_memory_type(ctx, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                    };
            vk_res = vkAllocateMemory(ctx->device, &alloc_info, NULL, &depth_mem);
            assert(vk_res == VK_SUCCESS);
            vk_res = vkBindImageMemory(ctx->device, depth_img, depth_mem, 0);
            assert(vk_res == VK_SUCCESS);
            VkImageViewCreateInfo create_info_view =
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = draw->depth_format,

                            .components.a = VK_COMPONENT_SWIZZLE_A,
                            .components.b = VK_COMPONENT_SWIZZLE_B,
                            .components.g = VK_COMPONENT_SWIZZLE_G,
                            .components.r = VK_COMPONENT_SWIZZLE_R,

                            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | ((draw->depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || draw->depth_format == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0),
                            .subresourceRange.baseMipLevel = 0,
                            .subresourceRange.levelCount = 1,
                            .subresourceRange.baseArrayLayer = 0,
                            .subresourceRange.layerCount = 1,
                            .image = depth_img,
                    };
            vk_res = vkCreateImageView(ctx->device, &create_info_view, NULL, &depth_view);
            assert(vk_res == VK_SUCCESS);

        }

        vkDestroyImageView(ctx->device, draw->depth_view, NULL);
        vkDestroyImage(ctx->device, draw->depth_img, NULL);
        vkFreeMemory(ctx->device, draw->depth_mem, NULL);
        draw->depth_view = depth_view;
        draw->depth_img = depth_img;
        draw->depth_mem = depth_mem;



        VkFramebufferCreateInfo fb_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = draw->render_pass,
                .attachmentCount = 2,
                .width = ctx->extent.width,
                .height = ctx->extent.height,
                .layers = 1,
                };
        for (u32 i = 0; i < ctx->n_images; ++i)
        {
            VkImageView attachments[2] = {ctx->views[i], depth_view};
            vkDestroyFramebuffer(ctx->device, draw->framebuffers[i], NULL);
            fb_create_info.pAttachments = attachments;
            vk_res = vkCreateFramebuffer(ctx->device, &fb_create_info, NULL, draw->framebuffers + i);
            assert(vk_res == VK_SUCCESS);
        }
        draw->viewport.width = (f32)ctx->extent.width;
        draw->viewport.height = (f32)ctx->extent.height;
        draw->scissor.extent = ctx->extent;
    }
        break;
    default: { assert(0); } return jfw_res_error;
    }

    return jfw_res_success;
}

static jfw_res destroy_draw(jfw_widget* this)
{
    void* const ptr = jfw_widget_get_user_pointer(this);
    struct draw_info_struct* draw = ptr;
    jfw_window_vk_resources* vk_res = jfw_window_get_vk_resources(this->window);
    vkDeviceWaitIdle(vk_res->device);
    for (u32 i = 0; i < vk_res->n_images; ++i)
    {
        vkDestroyFramebuffer(vk_res->device, draw->framebuffers[i], NULL);
    }
    vkDestroyDescriptorSetLayout(vk_res->device, draw->ubo_layout, NULL);
    vkDestroyPipeline(vk_res->device, draw->gfx_pipeline, NULL);
    vkDestroyPipelineLayout(vk_res->device, draw->layout, NULL);
    vkDestroyRenderPass(vk_res->device, draw->render_pass, NULL);
    vkDestroyBuffer(vk_res->device, draw->vtx_buffer, NULL);
    vkFreeMemory(vk_res->device, draw->vtx_mem, NULL);
    vkDestroyBuffer(vk_res->device, draw->idx_buffer, NULL);
    vkFreeMemory(vk_res->device, draw->idx_mem, NULL);
    vkDestroyBuffer(vk_res->device, draw->stage_buffer, NULL);
    vkFreeMemory(vk_res->device, draw->stage_mem, NULL);
    vkDestroyDescriptorPool(vk_res->device, draw->desc_pool, NULL);
    for (u32 i = 0; i < draw->n_uniform_buffers; ++i)
    {
        vkDestroyBuffer(vk_res->device, draw->uniform_buffers[i], NULL);
        vkFreeMemory(vk_res->device, draw->uniform_mem[i], NULL);
    }
    vkDestroyImageView(vk_res->device, draw->depth_view, NULL);
    vkDestroyImage(vk_res->device, draw->depth_img, NULL);
    vkFreeMemory(vk_res->device, draw->depth_mem, NULL);
    return jfw_res_success;
}

static i32 hook_fn(const char* thread_name, u32 stack_trace_count, const char*const* stack_trace, jfw_error_level level, u32 line, const char* file, const char* function, const char* message, void* param)
{
    printf("Error reported at level \"%s\" at line %u in file %s (function %s), message: %s\n", jfw_error_level_str(level), line, file, function, message);
    return 0;
}
static i32 report_fn(u32 total_count, u32 index, jfw_error_level level, u32 line, const char* file, const char* function, const char* message, void* param)
{
    printf("Error (%u out of %u) reported at level \"%s\" at line %u in file %s (function %s), message: %s\n", index + 1, total_count, jfw_error_level_str(level), line, file, function, message);
    return 0;
}

static VkResult create_buffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, jfw_window_vk_resources* const vk_res, VkBuffer* p_buffer, VkDeviceMemory* p_mem)
{
    *p_buffer = NULL;
    *p_mem = NULL;
    VkBuffer buffer;
    VkDeviceMemory mem;
    VkBufferCreateInfo create_info =
        {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = size,
                .usage = usage,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
    VkResult vk_result = vkCreateBuffer(vk_res->device, &create_info, NULL, &buffer);
    assert(vk_result == VK_SUCCESS);
    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(vk_res->device, buffer, &mem_req);
    i32 index = find_memory_type(vk_res, mem_req.memoryTypeBits, props);
    assert(index != -1);
    VkMemoryAllocateInfo alloc_info =
            {
                    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                    .allocationSize = mem_req.size,
                    .memoryTypeIndex = (u32)index,
            };
    vk_result = vkAllocateMemory(vk_res->device, &alloc_info, NULL, &mem);
    assert(vk_result == VK_SUCCESS);
    vk_result = vkBindBufferMemory(vk_res->device, buffer, mem, 0);
    assert(vk_result == VK_SUCCESS);
    *p_buffer = buffer;
    *p_mem = mem;
    return vk_result;
}

static VkResult copy_buffers(jfw_window_vk_resources* ctx, struct draw_info_struct* draw, size_t size, VkBuffer src_buffer, VkBuffer dst_buffer)
{
    VkResult vk_result;
    VkCommandBufferAllocateInfo alloc_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandBufferCount = 1,
            .commandPool = ctx->cmd_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            };
    VkCommandBuffer cmd_buffer;
    vk_result = vkAllocateCommandBuffers(ctx->device, &alloc_info, &cmd_buffer);
    assert(vk_result == VK_SUCCESS);
    VkCommandBufferBeginInfo begin_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
    vkBeginCommandBuffer(cmd_buffer, &begin_info);
    VkBufferCopy copy_info =
            {
            .size = size,
            .dstOffset = 0,
            .srcOffset = 0,
            };
    vkCmdCopyBuffer(cmd_buffer, src_buffer, dst_buffer, 1, &copy_info);
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submit_info =
            {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffer,
            };
    vkQueueSubmit(ctx->queue_transfer, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->queue_transfer);
    vkFreeCommandBuffers(ctx->device, ctx->cmd_pool, 1, &cmd_buffer);

    return VK_SUCCESS;
}

static VkFormat find_supported_img_format(VkPhysicalDevice dev, u32 n, const VkFormat* supported, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    VkFormatProperties props;
    for (u32 i = 0; i < n; ++i)
    {
        const VkFormat f = supported[i];
        vkGetPhysicalDeviceFormatProperties(dev, f, &props);
        if (features && tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features))
        {
            return f;
        }
        if (VK_IMAGE_TILING_LINEAR == tiling && (props.linearTilingFeatures & features) == features)
        {
            return f;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

static VkSampleCountFlagBits find_max_sample_flag(VkSampleCountFlagBits flags)
{
    if (flags & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (flags & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (flags & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (flags & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (flags & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (flags & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    assert(flags & VK_SAMPLE_COUNT_1_BIT);
    return VK_SAMPLE_COUNT_1_BIT;
}

int main()
{
    jfw_error_init_thread("master", jfw_error_level_none, 64, 64);
    JFW_ENTER_FUNCTION;
    jfw_ctx* context = NULL;
    jfw_window* wnd = NULL;
//    jfw_error_set_hook(hook_fn, NULL);
    jfw_res result;

    VkResult vk_result = VK_SUCCESS;
    result = jfw_context_create(&context, nullptr);
    assert(result == jfw_res_success);
    result = jfw_window_create(context, 720, 480, "Gaming window", (jfw_color) { .r = 0xFF, .a = 0xFF }, &wnd, false);
    assert(result == jfw_res_success);
    jfw_widget* button, *wnd_background;
    result = jfw_widget_create_as_base(wnd, 720, 480, 0, 0, &wnd_background);
    assert(result == jfw_res_success);
    result = jfw_window_set_base(wnd, wnd_background);
    assert(result == jfw_res_success);
    result = jfw_widget_create_as_child(wnd_background, 100, 100, 0, 0, &button);
    assert(result == jfw_res_success);
    jfw_error_process(report_fn, NULL);
    jfw_window_vk_resources* vk_res = jfw_window_get_vk_resources(wnd);

    VkSampleCountFlagBits sample_count = find_max_sample_flag(vk_res->sample_flags);

    VkImage depth_img;
    VkDeviceMemory depth_mem;
    VkImageView depth_view;
    VkFormat depth_format;
    u32 has_stencil;
    {
        const VkFormat possible_depth_formats[] =
                {
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                };
        const u32 n_possible_formats = sizeof(possible_depth_formats) / sizeof(*possible_depth_formats);
        depth_format = find_supported_img_format(vk_res->physical_device, n_possible_formats, possible_depth_formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        assert(depth_format != VK_FORMAT_UNDEFINED);
        has_stencil = depth_format == VK_FORMAT_D24_UNORM_S8_UINT || depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkImageCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .extent.width = vk_res->extent.width,
                .extent.height = vk_res->extent.height,
                .extent.depth = 1,
                .mipLevels = 1,
                .arrayLayers = 1,
                .format = depth_format,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                };
        vk_result = vkCreateImage(vk_res->device, &create_info, NULL, &depth_img);
        assert(vk_result == VK_SUCCESS);
        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(vk_res->device, depth_img, &mem_req);
        VkMemoryAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = mem_req.size,
                .memoryTypeIndex = find_memory_type(vk_res, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
                };
        vk_result = vkAllocateMemory(vk_res->device, &alloc_info, NULL, &depth_mem);
        assert(vk_result == VK_SUCCESS);
        vk_result = vkBindImageMemory(vk_res->device, depth_img, depth_mem, 0);
        assert(vk_result == VK_SUCCESS);
        VkImageViewCreateInfo create_info_view =
                {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depth_format,

                .components.a = VK_COMPONENT_SWIZZLE_A,
                .components.b = VK_COMPONENT_SWIZZLE_B,
                .components.g = VK_COMPONENT_SWIZZLE_G,
                .components.r = VK_COMPONENT_SWIZZLE_R,

                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (has_stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0),
                .subresourceRange.baseMipLevel = 0,
                .subresourceRange.levelCount = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount = 1,
                .image = depth_img,
                };
        vk_result = vkCreateImageView(vk_res->device, &create_info_view, NULL, &depth_view);
        assert(vk_result == VK_SUCCESS);

    }




    VkRenderPass render_pass;
    {
        VkAttachmentDescription color_attach_desc =
        {
            .format = vk_res->surface_format.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        VkAttachmentReference color_attach_ref_desc =
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        VkAttachmentDescription depth_attach_desc =
                {
                        .format = depth_format,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentReference depth_attach_ref_desc =
                {
                        .attachment = 1,
                        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                };
        VkSubpassDescription subpass_description =
        {
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attach_ref_desc,
            .pDepthStencilAttachment = &depth_attach_ref_desc,
        };
        VkSubpassDependency dependency_desc =
                {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = 0,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                };
        VkAttachmentDescription attachment_array[] =
                {
                color_attach_desc, depth_attach_desc
                };
        VkRenderPassCreateInfo render_pass_info =
                {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = 2,
                .pAttachments = attachment_array,
                .subpassCount = 1,
                .pSubpasses = &subpass_description,
                .dependencyCount = 1,
                .pDependencies = &dependency_desc,
                };
        vk_result = vkCreateRenderPass(vk_res->device, &render_pass_info, NULL, &render_pass);
        assert(vk_result == VK_SUCCESS);
    }
    const f32 vertex_data[] =
            {
                    -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
                     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
                     0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
                    -0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f,


                    -1.5f, -1.5f, 2.0f, 0.0f, 1.0f, 0.0f,
                     1.5f, -1.5f, 2.0f, 0.0f, 1.0f, 0.0f,
                     1.5f,  1.5f, 2.0f, 0.0f, 0.0f, 1.0f,
                    -1.5f,  1.5f, 2.0f, 0.0f, 0.0f, 1.0f,
            };
    const u16 index_data[] =
            {
                    4, 7, 6,
                    4, 6, 5,

                    0, 3, 2,
                    0, 2, 1,
            };

    // Create descriptors
    VkDescriptorSetLayout ubo_layout;
    {
        VkDescriptorSetLayoutBinding binding =
                {
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = NULL,
                };
        VkDescriptorSetLayoutCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &binding,
                };
        vk_result = vkCreateDescriptorSetLayout(vk_res->device, &create_info, NULL, &ubo_layout);
        assert(vk_result == VK_SUCCESS);
    }


    VkPipelineLayout pipeline_layout;
    VkShaderModule shader_module_vert, shader_module_frag;
    VkPipeline pipeline;
    VkViewport viewport =
            {
                    .x = 0.0f,
                    .y = 0.0f,
                    .width = (f32)vk_res->extent.width,
                    .height = (f32)vk_res->extent.height,
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
            };
    VkRect2D scissors =
            {
                    .offset = {0, 0},
                    .extent = vk_res->extent,
            };
    {


        VkVertexInputBindingDescription vtx_binding_description =
                {
                        .binding = 0,
                        .stride = sizeof(f32) * 6,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                };
        VkVertexInputAttributeDescription pos_attrib_desc =
                {
                        .binding = 0,
                        .location = 0,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = 0,
                };
        VkVertexInputAttributeDescription col_attrib_desc =
                {
                        .binding = 0,
                        .location = 1,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = sizeof(f32) * 3,
                };

        VkShaderModuleCreateInfo shader_create_info_vert =
                {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = sizeof(practice),
                        .pCode = practice,
                };
        vk_result = vkCreateShaderModule(vk_res->device, &shader_create_info_vert, NULL, &shader_module_vert);
        assert(vk_result == VK_SUCCESS);
        VkShaderModuleCreateInfo shader_create_info_frg =
                {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = sizeof(practice_frag),
                        .pCode = practice_frag,
                };
        vk_result = vkCreateShaderModule(vk_res->device, &shader_create_info_frg, NULL, &shader_module_frag);
        assert(vk_result == VK_SUCCESS);
        VkPipelineShaderStageCreateInfo vert_stage_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = shader_module_vert,
                .pName = "main",
            };
        VkPipelineShaderStageCreateInfo frag_stage_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = shader_module_frag,
                        .pName = "main",
                };
        VkPipelineShaderStageCreateInfo shader_stages_info[] =
                {
                vert_stage_info, frag_stage_info
                };
        VkDynamicState dynamic_states[] =
                {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
                };
        VkPipelineDynamicStateCreateInfo dynamic_state_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                .dynamicStateCount = sizeof(dynamic_states) / sizeof(*dynamic_states),
                .pDynamicStates = dynamic_states,
                };
        VkVertexInputAttributeDescription attrib_desc_array[] =
                {
                pos_attrib_desc, col_attrib_desc,
                };
        VkPipelineVertexInputStateCreateInfo input_state_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexAttributeDescriptionCount = 2,
                .pVertexAttributeDescriptions = attrib_desc_array,
                .vertexBindingDescriptionCount = 1,
                .pVertexBindingDescriptions = &vtx_binding_description,
                };
        VkPipelineInputAssemblyStateCreateInfo assembly_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .primitiveRestartEnable = VK_FALSE,
                .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                };

        VkPipelineViewportStateCreateInfo viewport_state_create_info =
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                    .viewportCount = 1,
                    .scissorCount = 1,
                    .pViewports = &viewport,
                    .pScissors = &scissors,
                };
        VkPipelineRasterizationStateCreateInfo raster_state_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                .depthClampEnable = VK_FALSE,
                .rasterizerDiscardEnable = VK_FALSE,
                .polygonMode = VK_POLYGON_MODE_FILL,
                .lineWidth = 1.0f,
                .cullMode = VK_CULL_MODE_BACK_BIT,
                .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,//VK_FRONT_FACE_CLOCKWISE,
                .depthBiasEnable = VK_FALSE,
                };
        VkPipelineMultisampleStateCreateInfo ms_state_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .sampleShadingEnable = VK_FALSE,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                };
        VkPipelineColorBlendAttachmentState cb_attach_state_create_info =
                {
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
                .blendEnable = VK_FALSE,
                };
        VkPipelineColorBlendStateCreateInfo cb_state_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                .logicOpEnable = VK_FALSE,
                .attachmentCount = 1,
                .pAttachments = &cb_attach_state_create_info,
                };
        VkPipelineLayoutCreateInfo layout_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &ubo_layout,
                };
        vk_result = vkCreatePipelineLayout(vk_res->device, &layout_create_info, NULL, &pipeline_layout);
        assert(vk_result == VK_SUCCESS);
        VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .depthTestEnable = VK_TRUE,
                .depthWriteEnable = VK_TRUE,
                .depthCompareOp = VK_COMPARE_OP_LESS,
                .depthBoundsTestEnable = VK_FALSE,
                .minDepthBounds = 0.0f,
                .maxDepthBounds = 1.0f,
                .stencilTestEnable = VK_FALSE,
                .front = {},
                .back = {},
                };
        VkGraphicsPipelineCreateInfo pipeline_info =
                {
                .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                .stageCount = 2,
                .pStages = shader_stages_info,
                .pVertexInputState = &input_state_info,
                .pInputAssemblyState = &assembly_create_info,
                .pViewportState = &viewport_state_create_info,
                .pRasterizationState = &raster_state_create_info,
                .pMultisampleState = &ms_state_create_info,
                .pDepthStencilState = &depth_stencil_state_create_info,
                .pColorBlendState = &cb_state_create_info,
                .pDynamicState = &dynamic_state_info,
                .layout = pipeline_layout,
                .renderPass = render_pass,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
                };
        vk_result = vkCreateGraphicsPipelines(vk_res->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline);
        assert(vk_result == VK_SUCCESS);
    }

    vkDestroyShaderModule(vk_res->device, shader_module_vert, NULL);
    vkDestroyShaderModule(vk_res->device, shader_module_frag, NULL);

    VkFramebuffer framebuffers[vk_res->n_images];
    for (u32 i = 0; i < vk_res->n_images; ++i)
    {
        VkImageView attachments[2] =
                {
                vk_res->views[i], depth_view
                };
        VkFramebufferCreateInfo fb_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = render_pass,
                .attachmentCount = 2,
                .pAttachments = attachments,
                .width = vk_res->extent.width,
                .height = vk_res->extent.height,
                .layers = 1,
                };
        vk_result = vkCreateFramebuffer(vk_res->device, &fb_create_info, NULL, framebuffers + i);
        assert(vk_result == VK_SUCCESS);
    }




    VkBuffer staging_buffer;
    VkDeviceMemory staging_mem;
    vk_result = create_buffer(sizeof(vertex_data), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk_res, &staging_buffer, &staging_mem);
    assert(vk_result == VK_SUCCESS);
    {
        void* p_stage;
        vk_result = vkMapMemory(vk_res->device, staging_mem, 0, sizeof(vertex_data), 0, &p_stage);
        assert(vk_result == VK_SUCCESS);
        memcpy(p_stage, vertex_data, sizeof(vertex_data));
        vkUnmapMemory(vk_res->device, staging_mem);
    }

    VkBuffer vtx_buffer;
    VkDeviceMemory vtx_mem;
    {
        vk_result = create_buffer(sizeof(vertex_data), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_res, &vtx_buffer, &vtx_mem);
        assert(vk_result == VK_SUCCESS);
        vk_result = copy_buffers(vk_res, NULL, sizeof(vertex_data), staging_buffer, vtx_buffer);
        assert(vk_result == VK_SUCCESS);
    }

    VkBuffer idx_buffer;
    VkDeviceMemory idx_mem;
    {
        void* p_stage;
        vk_result = vkMapMemory(vk_res->device, staging_mem, 0, sizeof(index_data), 0, &p_stage);
        assert(vk_result == VK_SUCCESS);
        memcpy(p_stage, index_data, sizeof(index_data));
        vkUnmapMemory(vk_res->device, staging_mem);
        vk_result = create_buffer(sizeof(index_data), VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_res, &idx_buffer, &idx_mem);
        assert(vk_result == VK_SUCCESS);
        vk_result = copy_buffers(vk_res, NULL, sizeof(index_data), staging_buffer, idx_buffer);
        assert(vk_result == VK_SUCCESS);
    }

    VkBuffer u_buffers[vk_res->n_frames_in_flight];
    VkDeviceMemory u_mem[vk_res->n_frames_in_flight];
    void* u_map[vk_res->n_frames_in_flight];

    for (u32 i = 0; i < vk_res->n_frames_in_flight; ++i)
    {
        vk_result = create_buffer(sizeof(struct UBO_struct), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vk_res, u_buffers + i, u_mem + i);
        assert(vk_result == VK_SUCCESS);
        vk_result = vkMapMemory(vk_res->device, u_mem[i], 0, sizeof(struct UBO_struct), 0, u_map + i);
        assert(vk_result == VK_SUCCESS);
    }

    VkDescriptorPool desc_pool;
    {
        VkDescriptorPoolSize pool_size =
                {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = vk_res->n_frames_in_flight,
                };
        VkDescriptorPoolCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .poolSizeCount = 1,
                .pPoolSizes = &pool_size,
                .maxSets = vk_res->n_frames_in_flight,
                };
        vk_result = vkCreateDescriptorPool(vk_res->device, &create_info, NULL, &desc_pool);
        assert(vk_result == VK_SUCCESS);
    }
    VkDescriptorSet desc_sets[vk_res->n_frames_in_flight];
    {
        VkDescriptorSetLayout set_layouts[vk_res->n_frames_in_flight];
        for (u32 i = 0; i < vk_res->n_frames_in_flight; ++i)
        {
            set_layouts[i] = ubo_layout;
        }
        VkDescriptorSetAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .pSetLayouts = set_layouts,
                .descriptorSetCount = vk_res->n_frames_in_flight,
                .descriptorPool = desc_pool,
                };
        vk_result = vkAllocateDescriptorSets(vk_res->device, &alloc_info, desc_sets);
        assert(vk_result == VK_SUCCESS);
        VkDescriptorBufferInfo buffer_info =
                {
                .offset = 0,
                .range = sizeof(struct UBO_struct),
                    };
        VkWriteDescriptorSet write_set =
                {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &buffer_info,
                .pImageInfo = NULL,
                .pTexelBufferView = NULL,
                };
        for (u32 i = 0; i < vk_res->n_frames_in_flight; ++i)
        {
            buffer_info.buffer = u_buffers[i];
            write_set.dstSet = desc_sets[i];
            vkUpdateDescriptorSets(vk_res->device, 1, &write_set, 0, NULL);
        }
    }

    struct draw_info_struct draw_info =
            {
            .framebuffers = framebuffers,
            .render_pass = render_pass,
            .viewport = viewport,
            .scissor = scissors,
            .gfx_pipeline = pipeline,
            .layout = pipeline_layout,
            .vtx_buffer = vtx_buffer,
            .vtx_mem = vtx_mem,
            .stage_buffer = staging_buffer,
            .stage_mem = staging_mem,
            .idx_buffer = idx_buffer,
            .idx_mem = idx_mem,
            .ubo_layout = ubo_layout,
            .desc_pool = desc_pool,
            .desc_set = desc_sets,
            .uniform_mem = u_mem,
            .uniform_buffers = u_buffers,
            .n_uniform_buffers = vk_res->n_frames_in_flight,
            .p_mapped_array = u_map,
            .depth_img = depth_img,
            .depth_mem = depth_mem,
            .depth_view = depth_view,
            .depth_format = depth_format,
            };


    wnd_background->user_pointer = &draw_info;
    wnd_background->draw_fn = draw_the_window;
    wnd_background->dtor_fn = destroy_draw;

    result = jfw_window_show(context, wnd);
    assert(result == jfw_res_success);
    for (;;)
    {
        jfw_error_process(report_fn, NULL);
        if (!jfw_success(jfw_context_process_events(context)))
        {
            break;
        }
        else
        {
            jfw_window_force_redraw(context, wnd);
            jfw_context_wait_for_events(context);
        }
    }

    result = jfw_context_destroy(context);
    assert(result == jfw_res_success);

    jfw_error_process(report_fn, NULL);
    JFW_LEAVE_FUNCTION;
    jfw_error_cleanup_thread();
    return 0;
}