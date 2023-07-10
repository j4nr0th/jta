#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "mem/aligned_jalloc.h"
#include <jmem/jmem.h>
#include <jdm.h>
#include <jio/iocfg.h>



#include "jfw/window.h"
#include "jfw/widget-base.h"
#include "jfw/error_system/error_codes.h"


#include "gfx/vk_state.h"
#include "gfx/drawing_3d.h"
#include "gfx/camera.h"
#include "gfx/bounding_box.h"
#include "ui.h"
#include "solver/jtaelements.h"
#include "solver/jtanaturalbcs.h"
#include "solver/jtanumericalbcs.h"

//
//static gfx_result save_screenshot(vk_state* const state, jfw_window_vk_resources* resources, const char* filename)
//{
//    //  Code based on https://github.com/SaschaWillems/Vulkan/blob/master/examples/screenshot/screenshot.cpp accessed on 10.7.2023
//    bool supports_blit = true;
//    VkFormatProperties f_props;
//    //  Check for blit support (otherwise fallback on copy)
//    vkGetPhysicalDeviceFormatProperties(resources->physical_device, resources->surface_format.format, &f_props);
//    if (!(f_props.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
//    {
//        //  No blit support for the input (swapchain optimal) format
//        JDM_TRACE("No blitting support for input format for the screenshot");
//        supports_blit = false;
//    }
//    else
//    {
//        vkGetPhysicalDeviceFormatProperties(resources->physical_device, VK_FORMAT_R8G8B8A8_UNORM, &f_props);
//        if (!(f_props.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT))
//        {
//            //  No blit support for the output (linear) format
//            JDM_TRACE("No blitting support for output format for the screenshot");
//            supports_blit = false;
//        }
//    }
//    if (!supports_blit)
//    {
//        JDM_TRACE("Blitting not an option for screenshot, resorting to an image copy");
//    }
//    uint32_t swp_count;
//    VkResult vk_res = vkGetSwapchainImagesKHR(resources->device, resources->swapchain, &swp_count, NULL);
//    if (vk_res != VK_SUCCESS)
//    {
//        JDM_ERROR("Could not acquire swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
//        return GFX_RESULT_BAD_IMG;
//    }
//    VkImage* swapchain_images = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*swapchain_images) * swp_count);
//    if (!swapchain_images)
//    {
//        JDM_ERROR("Could not allocate memory for swapchain image array");
//        return GFX_RESULT_BAD_ALLOC;
//    }
//    VkImage src_image;
//    vk_res = vkGetSwapchainImagesKHR(resources->device, resources->swapchain, &swp_count, swapchain_images);
//    if (vk_res != VK_SUCCESS)
//    {
//        lin_jfree(G_LIN_JALLOCATOR, swapchain_images);
//        JDM_ERROR("Could not acquire swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
//        return GFX_RESULT_BAD_IMG;
//    }
//    assert(state->last_img_idx < swp_count);
//    src_image = swapchain_images[state->last_img_idx];
//    lin_jfree(G_LIN_JALLOCATOR, swapchain_images);
//    VkImage dst_image;
//    {
//        VkImageCreateInfo create_info =
//                {
//                .imageType = VK_IMAGE_TYPE_2D,
//                .format = VK_FORMAT_R8G8B8A8_UINT,
//                .extent.width = resources->extent.width,
//                .extent.height = resources->extent.height,
//                .extent.depth = 1,
//                .arrayLayers = 1,
//                .mipLevels = 1,
//                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
//                .samples = VK_SAMPLE_COUNT_1_BIT,
//                .tiling = VK_IMAGE_TILING_LINEAR,
//                .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
//                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//                };
//
//        vk_res = vkCreateImage(resources->device, &create_info, NULL, &dst_image);
//        if (vk_res != VK_SUCCESS)
//        {
//            JDM_ERROR("Could not create destination image");
//            return GFX_RESULT_BAD_IMG;
//        }
//    }
//    VkDeviceMemory dst_mem;
//    VkMemoryRequirements mem_req;
//    vkGetImageMemoryRequirements(resources->device, dst_image, &mem_req);
//    VkMemoryPropertyFlags mem_props;
//    {
//        //  Find memory type (I swear I will clean this part up!)
//        VkPhysicalDeviceMemoryProperties memory_properties;
//        vkGetPhysicalDeviceMemoryProperties(resources->physical_device, &memory_properties);
//        u32 mem_type = find_device_memory_type(&memory_properties, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//        if (mem_type == -1)
//        {
//            JDM_ERROR("No memory type supports the needed properties");
//            return GFX_RESULT_BAD_ALLOC;
//        }
//        VkMemoryAllocateInfo alloc_info =
//                {
//                .allocationSize = mem_req.size,
//                .memoryTypeIndex = mem_type,
//                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
//                };
//        vk_res = vkAllocateMemory(resources->device, &alloc_info, NULL, &dst_mem);
//        if (vk_res != VK_SUCCESS)
//        {
//            JDM_ERROR("Could not allocate memory for the output image");
//            vkDestroyImage(resources->device, dst_image, NULL);
//            return GFX_RESULT_BAD_ALLOC;
//        }
//        vk_res = vkBindImageMemory(resources->device, dst_image, dst_mem, 0);
//        if (vk_res != VK_SUCCESS)
//        {
//            JDM_ERROR("Could not bind device memory to output image");
//            vkFreeMemory(resources->device, dst_mem, NULL);
//            vkDestroyImage(resources->device, dst_image, NULL);
//            return GFX_RESULT_BAD_ALLOC;
//        }
//    }
//    //  Create a command buffer for moving commands
//    VkCommandBuffer cpy_buffer;
//
//    {
//        VkCommandBufferAllocateInfo alloc_info =
//                {
//                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
//                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
//                .commandBufferCount = 1,
//                .commandPool = resources->cmd_pool,
//                };
//        vk_res = vkAllocateCommandBuffers(resources->device, &alloc_info, &cpy_buffer);
//        if (vk_res != VK_SUCCESS)
//        {
//            JDM_ERROR("Could not allocate command buffer for copy/blit operation");
//            vkFreeMemory(resources->device, dst_mem, NULL);
//            vkDestroyImage(resources->device, dst_image, NULL);
//            return GFX_RESULT_BAD_ALLOC;
//        }
//        VkCommandBufferBeginInfo begin_info =
//                {
//                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
//                };
//        vk_res = vkBeginCommandBuffer(cpy_buffer, &begin_info);
//        if (vk_res != VK_SUCCESS)
//        {
//            JDM_ERROR("Could not begin command buffer for copy/blit operation");
//            vkFreeMemory(resources->device, dst_mem, NULL);
//            vkDestroyImage(resources->device, dst_image, NULL);
//            return GFX_RESULT_BAD_ALLOC;
//        }
//    }
//    //  Transition for output image from initial layout to destination layout
//    VkImageMemoryBarrier dst_initial_img_barrier =
//            {
//            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//            .subresourceRange =
//                    {
//                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                    .baseArrayLayer = 0,
//                    .baseMipLevel = 0,
//                    .layerCount = 1,
//                    .levelCount = 1,
//                    },
//                .srcAccessMask = 0,
//                .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
//                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
//                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                .srcQueueFamilyIndex = 0,
//                .dstQueueFamilyIndex = 0,
//                .image = dst_image,
//            };
//    //  Transition for input image from initial layout to destination layout
//    VkImageMemoryBarrier src_initial_img_barrier =
//            {
//                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//                    .subresourceRange =
//                            {
//                                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                                    .baseArrayLayer = 0,
//                                    .baseMipLevel = 0,
//                                    .layerCount = 1,
//                                    .levelCount = 1,
//                            },
//                    .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
//                    .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
//                    .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
//                    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//                    .srcQueueFamilyIndex = 0,
//                    .dstQueueFamilyIndex = 0,
//                    .image = src_image,
//            };
//    VkImageMemoryBarrier initial_img_barriers[2] = { src_initial_img_barrier, dst_initial_img_barrier};
//    vkCmdPipelineBarrier(cpy_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, NULL, 0, NULL, sizeof(initial_img_barriers) / sizeof(*initial_img_barriers), initial_img_barriers);
////    if (supports_blit)
//    if (0)
//    {
//        VkOffset3D size =
//                {
//                        .x = (int32_t)resources->extent.width,
//                        .y = (int32_t)resources->extent.height,
//                        .z = 1,
//                };
//        VkImageBlit blit_region =
//                {
//                .srcSubresource =
//                        {
//                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                        .layerCount = 1,
//                        },
//                .dstOffsets[1] = size,
//                .dstSubresource =
//                        {
//                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                        .layerCount = 1,
//                        },
//                .srcOffsets[1] = size,
//                };
//        vkCmdBlitImage(cpy_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_NEAREST);
//    }
//    else
//    {
//        VkImageCopy cpy =
//                {
//                .srcSubresource =
//                        {
//                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                        .layerCount = 1,
//                        },
//                .dstSubresource =
//                        {
//                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                        .layerCount = 1,
//                        },
//                .extent.width = resources->extent.width,
//                .extent.height = resources->extent.height,
//                .extent.depth = 1,
//                };
//        vkCmdCopyImage(cpy_buffer, src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &cpy);
//    }
//    //  Transition back to correct layouts
//
//    VkImageMemoryBarrier dst_final_img_barrier =
//            {
//                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//                    .subresourceRange =
//                            {
//                                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                                    .baseArrayLayer = 0,
//                                    .baseMipLevel = 0,
//                                    .layerCount = 1,
//                                    .levelCount = 1,
//                            },
//                    .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
//                    .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
//                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                    .newLayout = VK_IMAGE_LAYOUT_GENERAL,
//                    .srcQueueFamilyIndex = 0,
//                    .dstQueueFamilyIndex = 0,
//                    .image = dst_image,
//            };
//    //  Transition for input image from initial layout to destination layout
//    VkImageMemoryBarrier src_final_img_barrier =
//            {
//                    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
//                    .subresourceRange =
//                            {
//                                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//                                    .baseArrayLayer = 0,
//                                    .baseMipLevel = 0,
//                                    .layerCount = 1,
//                                    .levelCount = 1,
//                            },
//                    .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
//                    .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
//                    .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
//                    .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//                    .srcQueueFamilyIndex = 0,
//                    .dstQueueFamilyIndex = 0,
//                    .image = src_image,
//            };
//    VkImageMemoryBarrier final_img_barriers[2] = { src_final_img_barrier, dst_final_img_barrier};
//    vkCmdPipelineBarrier(cpy_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_DEVICE_GROUP_BIT, 0, NULL, 0, NULL, sizeof(final_img_barriers) / sizeof(*final_img_barriers), final_img_barriers);
//    vkEndCommandBuffer(cpy_buffer);
//    VkSubmitInfo submit_info =
//            {
//            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
//            .commandBufferCount = 1,
//            .pCommandBuffers = &cpy_buffer,
//            };
//    vk_res = vkWaitForFences(resources->device, 1, &state->fence_transfer_free, VK_TRUE, UINT64_MAX);
//    if (vk_res != VK_SUCCESS)
//    {
//        JDM_ERROR("Failed waiting for transfer fence");
//        vkFreeCommandBuffers(resources->device, resources->cmd_pool, 1, &cpy_buffer);
//        vkFreeMemory(resources->device, dst_mem, NULL);
//        vkDestroyImage(resources->device, dst_image, NULL);
//        return GFX_RESULT_BAD_FENCE_WAIT;
//    }
//    vk_res = vkResetFences(resources->device, 1, &state->fence_transfer_free);
//    if (vk_res != VK_SUCCESS)
//    {
//        JDM_ERROR("Failed resetting the transfer fence");
//        vkFreeCommandBuffers(resources->device, resources->cmd_pool, 1, &cpy_buffer);
//        vkFreeMemory(resources->device, dst_mem, NULL);
//        vkDestroyImage(resources->device, dst_image, NULL);
//        return GFX_RESULT_BAD_FENCE_RESET;
//    }
//    vkQueueSubmit(resources->queue_transfer, 1, &submit_info, state->fence_transfer_free);
//    vk_res = vkWaitForFences(resources->device, 1, &state->fence_transfer_free, VK_TRUE, UINT64_MAX);
//    if (vk_res != VK_SUCCESS)
//    {
//        JDM_ERROR("Failed waiting for transfer fence");
//        vkFreeCommandBuffers(resources->device, resources->cmd_pool, 1, &cpy_buffer);
//        vkFreeMemory(resources->device, dst_mem, NULL);
//        vkDestroyImage(resources->device, dst_image, NULL);
//        return GFX_RESULT_BAD_FENCE_WAIT;
//    }
//    vk_res = vkResetFences(resources->device, 1, &state->fence_transfer_free);
//    if (vk_res != VK_SUCCESS)
//    {
//        JDM_ERROR("Failed resetting the transfer fence");
//        vkFreeCommandBuffers(resources->device, resources->cmd_pool, 1, &cpy_buffer);
//        vkFreeMemory(resources->device, dst_mem, NULL);
//        vkDestroyImage(resources->device, dst_image, NULL);
//        return GFX_RESULT_BAD_FENCE_RESET;
//    }
//    VkImageSubresource subresource =
//            {
//            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
//            .arrayLayer = 0,
//            .mipLevel = 0,
//            };
//    VkSubresourceLayout layout;
//    vkGetImageSubresourceLayout(resources->device, dst_image, &subresource, &layout);
//    const uint8_t* data;
//    vkFreeCommandBuffers(resources->device, resources->cmd_pool, 1, &cpy_buffer);
//    vk_res = vkMapMemory(resources->device, dst_mem, 0, VK_WHOLE_SIZE, 0, (void**)&data);
//    if (vk_res != VK_SUCCESS)
//    {
//        JDM_ERROR("Could not map destination image memory");
//    }
//    data += layout.offset;
//
//    FILE* f_out = fopen(filename, "wb");
//    if (!f_out)
//    {
//        JDM_ERROR("Could not open a screenshot output file, reason: %s", JDM_ERRNO_MESSAGE);
//        vkFreeMemory(resources->device, dst_mem, NULL);
//        vkDestroyImage(resources->device, dst_image, NULL);
//        return GFX_RESULT_BAD_IO;
//    }
//    fprintf(f_out, "P6\n%"PRIu32"\n%"PRIu32"\n255\n", resources->extent.width, resources->extent.height);
//    bool swizzle = !supports_blit;
//    if (!supports_blit)
//    {
//        VkFormat formats[] =
//                {
//                VK_FORMAT_B8G8R8A8_UNORM,
//                VK_FORMAT_B8G8R8A8_SNORM,
//                VK_FORMAT_B8G8R8A8_UINT,
//                VK_FORMAT_B8G8R8A8_SINT,
//                VK_FORMAT_B8G8R8A8_SRGB,
//                };
//        for (u32 i = 0; i < sizeof(formats) / sizeof(*formats); ++i)
//        {
//            if (formats[i] == resources->surface_format.format)
//            {
//                swizzle = false;
//                break;
//            }
//        }
//    }
//
//    //  PPM pixel data
//    if (swizzle)
//    {
//        for (u32 y = 0; y < resources->extent.height; ++y)
//        {
//            const uint32_t* p_row = (const uint32_t*)data;
//            for (u32 x = 0; x < resources->extent.width; ++x)
//            {
//                size_t written = fwrite(((const uint8_t*)data) + 2, 1, 1, f_out);
//                if (written != 1)
//                {
//                    JDM_WARN("Write to file failed: %s", JDM_ERRNO_MESSAGE);
//                }
//                written = fwrite(((const uint8_t*)data) + 1, 1, 1, f_out);
//                if (written != 1)
//                {
//                    JDM_WARN("Write to file failed: %s", JDM_ERRNO_MESSAGE);
//                }
//                written = fwrite(((const uint8_t*)data) + 0, 1, 1, f_out);
//                if (written != 1)
//                {
//                    JDM_WARN("Write to file failed: %s", JDM_ERRNO_MESSAGE);
//                }
//            }
//            data += layout.rowPitch;
//        }
//    }
//    else
//    {
//        for (u32 y = 0; y < resources->extent.height; ++y)
//        {
//            const uint32_t* p_row = (const uint32_t*)data;
//            for (u32 x = 0; x < resources->extent.width; ++x)
//            {
//                size_t written = fwrite(data, 3, 1, f_out);
//                if (written != 1)
//                {
//                    JDM_WARN("Write to file failed: %s", JDM_ERRNO_MESSAGE);
//                }
//            }
//            data += layout.rowPitch;
//        }
//    }
//    fclose(f_out);
//    JDM_TRACE("Screenshot saved to \"%s\"\n", filename);
//    vkUnmapMemory(resources->device, dst_mem);
//    vkFreeMemory(resources->device, dst_mem, NULL);
//    vkDestroyImage(resources->device, dst_image, NULL);
//    return GFX_RESULT_SUCCESS;
//}

static jfw_res widget_draw(jfw_widget* this)
{
    jta_draw_state* const draw_state = jfw_widget_get_user_pointer(this);
    vk_state* const state = draw_state->vulkan_state;
    bool draw_good = draw_frame(
            state, jfw_window_get_vk_resources(this->window), state->mesh_count, state->mesh_array,
            &draw_state->camera) == GFX_RESULT_SUCCESS;
    if (draw_good && draw_state->screenshot)
    {
        //  Save the screenshot
//        save_screenshot(state, jfw_window_get_vk_resources(this->window), "screenshot.png");


        draw_state->screenshot = 0;
    }
    return draw_good ? jfw_res_success : jfw_res_error;
}

static jfw_res widget_dtor(jfw_widget* this)
{
    jfw_window* wnd = this->window;
    void* state = jfw_window_get_usr_ptr(wnd);
    jfw_window_vk_resources* vk_res = jfw_window_get_vk_resources(wnd);
    vkDeviceWaitIdle(vk_res->device);
    vk_state_destroy(state, vk_res);
    return jfw_res_success;
}

static void* VKAPI_PTR vk_aligned_allocation_fn(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
    void* ptr = aligned_jalloc((aligned_jallocator*) pUserData, alignment, size);
    if (alignment > 8)
    JDM_INFO("Called aligned_jalloc(%p, %zu, %zu) = %p\n", pUserData, alignment, size, ptr);
    return ptr;
}

static void* VKAPI_PTR vk_aligned_reallocation_fn(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
    void* ptr = aligned_jrealloc((aligned_jallocator*) pUserData, pOriginal, alignment, size);
    if (alignment > 8)
    JDM_INFO("Called aligned_rejalloc(%p, %p, %zu, %zu) = %p\n", pUserData, pOriginal, alignment, size, ptr);
    return ptr;
}

static void VKAPI_PTR vk_aligned_free_fn(void* pUserData, void* pMemory)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
//    JDM_INFO("Called aligned_jfree(%p, %p)", pUserData, pMemory);
    aligned_jfree(pUserData, pMemory);
}

static const char* vk_internal_allocation_type_to_str(VkInternalAllocationType allocation_type)
{
    switch (allocation_type)
    {
    case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE: return "VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE";
    default: return "UNKNOWN";
    }
}

static const char* vk_system_allocation_scope_to_str(VkSystemAllocationScope allocation_scope)
{
    switch (allocation_scope)
    {
    case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND: return "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND";
    case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT: return "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT";
    case VK_SYSTEM_ALLOCATION_SCOPE_CACHE: return "VK_SYSTEM_ALLOCATION_SCOPE_CACHE";
    case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE: return "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE";
    case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE: return "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE";
    default:return "UNKNOWN";
    }
}

static void VKAPI_PTR vk_internal_alloc_notif(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
    printf("Vulkan allocating %zu bytes of memory for type %s and scope %s", size, vk_internal_allocation_type_to_str(allocationType),
           vk_system_allocation_scope_to_str(allocationScope));
}

static void VKAPI_PTR vk_internal_free_notif(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
    printf("Vulkan freeing %zu bytes of memory for type %s and scope %s", size, vk_internal_allocation_type_to_str(allocationType),
           vk_system_allocation_scope_to_str(allocationScope));
}


static i32 jfw_error_hook_callback_function(const char* thread_name, u32 stack_trace_count, const char*const* stack_trace, jdm_message_level level, u32 line, const char* file, const char* function, const char* message, void* param)
{
    switch (level)
    {
    case JDM_MESSAGE_LEVEL_INFO:fprintf(stdout, "JDM_INFO (thread %s - %s): %s\n", thread_name, function, message);
        break;
    case JDM_MESSAGE_LEVEL_WARN:
        fprintf(stderr, "Warning issued from thread \"%s\", stack trace (%s", thread_name, stack_trace[0]);
        for (u32 i = 1; i < stack_trace_count; ++i)
        {
            fprintf(stderr, "-> %s", stack_trace[i]);
        }
        fprintf(stderr, ") in file \"%s\" on line \"%i\" (function \"%s\"): %s\n\n", file, line, function, message);
        break;

    case JDM_MESSAGE_LEVEL_ERR:
    default:
        fprintf(stderr, "Error occurred in thread \"%s\", stack trace (%s", thread_name, stack_trace[0]);
        for (u32 i = 1; i < stack_trace_count; ++i)
        {
            fprintf(stderr, "-> %s", stack_trace[i]);
        }
        fprintf(stderr, ") in file \"%s\" on line \"%i\" (function \"%s\"): %s\n\n", file, line, function, message);
        break;
    }
    (void)param;

    return 0;
}

static void double_free_hook(jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    JDM_ERROR("Double free detected by allocator %s (%p)", allocator->type, allocator);
    JDM_LEAVE_FUNCTION;
}

static void invalid_alloc(jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    JDM_ERROR("Bad allocation detected by allocator %s (%p)", allocator->type, allocator);
    JDM_LEAVE_FUNCTION;
}

static void jmem_trap(uint32_t idx, void* param)
{
    __builtin_trap();
}

static char* get_string_value_from_the_section(const jio_cfg_section* section, const char* const name)
{
    jio_cfg_value v;
    jio_result jio_res = jio_cfg_get_value_by_key(section, name, &v);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get value of key \"%s\" from section \"%.*s\", reason: %s", name, (int)section->name.len, section->name.begin,
                  jio_result_to_str(jio_res));
        return NULL;
    }
    if (v.type != JIO_CFG_TYPE_STRING)
    {
        JDM_ERROR("Type of value of key \"%s\" of section \"%.*s\" was not \"String\", instead it was \"%s\"", name, (int)section->name.len, section->name.begin, "Unknown");
        return NULL;
    }
    char* str = lin_jalloc(G_LIN_JALLOCATOR, v.value_string.len + 1);
    if (!str)
    {
        JDM_ERROR("Could not allocate %zu bytes of memory to copy value of key \"%s\" from section \"%.*s\"", (size_t)v.value_string.len + 1, name, (int)section->name.len, section->name.begin);
        return NULL;
    }
    memcpy(str, v.value_string.begin, v.value_string.len);
    str[v.value_string.len] = 0;
    return str;
}

int main(int argc, char* argv[argc])
{

    //  Create allocators
    //  Estimated to be 512 kB per small pool and 512 kB per med pool
    G_ALIGN_JALLOCATOR = aligned_jallocator_create(1, 1);
    if (!G_ALIGN_JALLOCATOR)
    {
        fputs("Could not create aligned allocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_LIN_JALLOCATOR = lin_jallocator_create(1 << 20);
    if (!G_LIN_JALLOCATOR)
    {
        fputs("Could not create linear allocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_JALLOCATOR = ill_jallocator_create(1 << 20, 1);
    if (!G_JALLOCATOR)
    {
        fputs("Could not create base jallocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_JALLOCATOR->double_free_callback = double_free_hook;
    G_JALLOCATOR->bad_alloc_callback = invalid_alloc;

    {
        jdm_allocator_callbacks jdm_callbacks =
                {
                .param = G_JALLOCATOR,
                .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                .free = (void (*)(void*, void*)) ill_jfree,
                };
        jdm_init_thread(
                "master",
#ifndef NEBUG
                JDM_MESSAGE_LEVEL_NONE,
#else
                JDM_MESSAGE_LEVEL_WARN,
#endif
                64,
                64,
                &jdm_callbacks);
    }
    JDM_ENTER_FUNCTION;
    jdm_set_hook(jfw_error_hook_callback_function, NULL);

    if (argc == 1)
    {
        printf("usage:\n    %s CFG_FILE\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    //  Load up the configuration
    char* pts_file_name;
    char* mat_file_name;
    char* pro_file_name;
    char* elm_file_name;
    char* nat_file_name;
    char* num_file_name;

    {
        jio_memory_file cfg_file;
        jio_result jio_res = jio_memory_file_create(argv[1], &cfg_file, 0, 0, 0);
        if (jio_res != JIO_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not open configuration file \"%s\", reason: %s", argv[1], jio_result_to_str(jio_res));
        }

        jio_cfg_section* cfg_root;
        {
            jio_allocator_callbacks cfg_callbacks =
                    {
                    .param = G_JALLOCATOR,
                    .free = (void (*)(void*, void*)) ill_jfree,
                    .realloc = (void* (*)(void*, void*, uint64_t)) ill_jrealloc,
                    .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                    };
            jio_res = jio_cfg_parse(&cfg_file, &cfg_root, &cfg_callbacks);
            if (jio_res != JIO_RESULT_SUCCESS)
            {
                JDM_FATAL("Could not parse configuration file \"%s\", reason: %s", argv[1], jio_result_to_str(jio_res));
            }
        }


        //  Parse the config file
        {
            jio_cfg_section* setup_section;
            jio_res = jio_cfg_get_subsection(cfg_root, "problem setup", &setup_section);
            if (jio_res != JIO_RESULT_SUCCESS)
            {
                JDM_FATAL("Could not find the configuration file's \"problem setup\" section, reason: %s",
                          jio_result_to_str(jio_res));
            }


            {
                jio_cfg_section* input_section;
                jio_res = jio_cfg_get_subsection(setup_section, "input files", &input_section);
                if (jio_res != JIO_RESULT_SUCCESS)
                {
                    JDM_FATAL(
                            "Could not find the configuration file's \"problem setup.input files\" section, reason: %s",
                            jio_result_to_str(jio_res));
                }
                //  Get the points file
                {
                    jio_cfg_value v;
                    jio_res = jio_cfg_get_value_by_key(input_section, "points", &v);
                    if (jio_res != JIO_RESULT_SUCCESS)
                    {
                        JDM_FATAL("Could not get the element of section \"problem setup.input files\" with key \"points\", reason: %s", jio_result_to_str(jio_res));
                    }
                    if (v.type != JIO_CFG_TYPE_STRING)
                    {
                        JDM_FATAL("Element \"points\" of section \"problem setup.input files\" was not a string");
                    }
                    pts_file_name = lin_jalloc(G_LIN_JALLOCATOR ,v.value_string.len + 1);
                    if (!pts_file_name)
                    {
                        JDM_FATAL("Could not allocate %zu bytes for point file string", (size_t)v.value_string.len + 1);
                    }
                    memcpy(pts_file_name, v.value_string.begin, v.value_string.len);
                    pts_file_name[v.value_string.len] = 0;
                }
                //  Get the material file
                mat_file_name = get_string_value_from_the_section(input_section, "materials");
                if (!mat_file_name)
                {
                    JDM_FATAL("Could not load materials");
                }
                //  Get the profile file
                pro_file_name = get_string_value_from_the_section(input_section, "profiles");
                if (!pro_file_name)
                {
                    JDM_FATAL("Could not load profiles");
                }
                //  Get the element file
                elm_file_name = get_string_value_from_the_section(input_section, "elements");
                if (!elm_file_name)
                {
                    JDM_FATAL("Could not load elements");
                }
                //  Get the natural boundary condition file
                nat_file_name = get_string_value_from_the_section(input_section, "natural_bcs");
                if (!nat_file_name)
                {
                    JDM_FATAL("Could not load natural bcs");
                }
                //  Get the numerical boundary condition file
                num_file_name = get_string_value_from_the_section(input_section, "numerical_bcs");
                if (!num_file_name)
                {
                    JDM_FATAL("Could not load numerical bcs");
                }
            }

            {
                jio_cfg_section* parameter_section;
                jio_res = jio_cfg_get_subsection(setup_section, "parameters", &parameter_section);
                if (jio_res != JIO_RESULT_SUCCESS)
                {
                    JDM_FATAL(
                            "Could not find the configuration file's \"problem setup.parameters\" section, reason: %s",
                            jio_result_to_str(jio_res));
                }
            }

            {
                jio_cfg_section* output_section;
                jio_res = jio_cfg_get_subsection(setup_section, "output files", &output_section);
                if (jio_res != JIO_RESULT_SUCCESS)
                {
                    JDM_FATAL(
                            "Could not find the configuration file's \"problem setup.output files\" section, reason: %s",
                            jio_result_to_str(jio_res));
                }
            }
        }

        jio_cfg_section_destroy(cfg_root);
        jio_memory_file_destroy(&cfg_file);
    }

//    ill_jallocator_set_debug_trap(G_JALLOCATOR, 35, jmem_trap, NULL);

    u32 n_materials;
    jio_memory_file file_points, file_materials, file_profiles, file_elements, file_nat, file_num;
    jta_point_list point_list;
    jta_material* materials;
    jta_profile_list profile_list;
    jta_element_list elements;
    jta_natural_boundary_condition_list natural_boundary_conditions;
    jta_numerical_boundary_condition_list numerical_boundary_conditions;

    jio_result jio_res = jio_memory_file_create(pts_file_name, &file_points, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open point file \"%s\"", pts_file_name);
    }
    jio_res = jio_memory_file_create(mat_file_name, &file_materials, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open material file \"%s\"", mat_file_name);
    }
    jio_res = jio_memory_file_create(pro_file_name, &file_profiles, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open profile file \"%s\"", pro_file_name);
    }
    jio_res = jio_memory_file_create(elm_file_name, &file_elements, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open element file \"%s\"", elm_file_name);
    }
    jio_res = jio_memory_file_create(nat_file_name, &file_nat, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open element file \"%s\"", nat_file_name);
    }
    jio_res = jio_memory_file_create(num_file_name, &file_num, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open element file \"%s\"", num_file_name);
    }
    lin_jfree(G_LIN_JALLOCATOR, num_file_name);
    lin_jfree(G_LIN_JALLOCATOR, nat_file_name);
    lin_jfree(G_LIN_JALLOCATOR, elm_file_name);
    lin_jfree(G_LIN_JALLOCATOR, pro_file_name);
    lin_jfree(G_LIN_JALLOCATOR, mat_file_name);
    lin_jfree(G_LIN_JALLOCATOR, pts_file_name);

    jta_result jta_res = jta_load_points(&file_points, &point_list);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load points");
    }
    if (point_list.count < 2)
    {
        JDM_FATAL("At least two points should be defined");
    }
    jta_res = jta_load_materials(&file_materials, &n_materials, &materials);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load materials");
    }
    if (n_materials < 1)
    {
        JDM_FATAL("At least one material should be defined");
    }
    jta_res = jta_load_profiles(&file_profiles, &profile_list);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load profiles");
    }
    if (profile_list.count < 1)
    {
        JDM_FATAL("At least one profile should be defined");
    }

    jta_res = jta_load_elements(&file_elements, &point_list, n_materials, materials, &profile_list, &elements);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load elements");
    }
    if (elements.count < 1)
    {
        JDM_FATAL("At least one profile should be defined");
    }

    jta_res = jta_load_natural_boundary_conditions(&file_nat, &point_list, &natural_boundary_conditions);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load natural boundary conditions");
    }

    jta_res = jta_load_numerical_boundary_conditions(&file_num, &point_list, &numerical_boundary_conditions);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load numerical boundary conditions");
    }

    //  Find the bounding box of the geometry
    vec4 geo_base;
    f32 geo_radius;
    gfx_find_bounding_sphere(&point_list, &geo_base, &geo_radius);



//#ifdef NDEBUG
//    VkAllocationCallbacks VK_ALLOCATION_CALLBACKS =
//            {
//            .pUserData = G_ALIGN_JALLOCATOR,
//            .pfnAllocation = vk_aligned_allocation_fn,
//            .pfnReallocation = vk_aligned_reallocation_fn,
//            .pfnFree = vk_aligned_free_fn,
//            .pfnInternalAllocation = vk_internal_alloc_notif,
//            .pfnInternalFree = vk_internal_free_notif,
//            };
//#endif

    jfw_ctx* jctx = NULL;
    jfw_window* jwnd = NULL;
    jfw_res jfw_result = jfw_context_create(&jctx,
//#ifndef NDEBUG
                                            NULL
//#else
//                                            &VK_ALLOCATION_CALLBACKS
//#endif
                                           );
    if (!jfw_success(jfw_result))
    {
        JDM_ERROR("Could not create jfw context, reason: %s", jfw_error_message(jfw_result));
        goto cleanup;
    }
    jfw_result = jfw_window_create(
            jctx, 1600, 900, "JANSYS - jta - 0.0.1", (jfw_color) { .a = 0xFF, .r = 0x80, .g = 0x80, .b = 0x80 },
            &jwnd, 0);
    if (!jfw_success(jfw_result))
    {
        JDM_ERROR("Could not create window");
        goto cleanup;
    }
    jfw_window_show(jctx, jwnd);
    VkResult vk_result;
    jfw_window_vk_resources* const vk_res = jfw_window_get_vk_resources(jwnd);
    jfw_vulkan_context* const vk_ctx = &jctx->vk_ctx;
    vk_state vulkan_state;
    gfx_result gfx_res = vk_state_create(&vulkan_state, vk_res);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create vulkan state");
        goto cleanup;
    }
    jta_mesh truss_mesh;
    jta_mesh sphere_mesh;
    jta_mesh cone_mesh;
    vulkan_state.point_list = &point_list;
    if ((gfx_res = mesh_init_truss(&truss_mesh, 1 << 12, &vulkan_state, vk_res)) != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }
    if ((gfx_res = mesh_init_sphere(&sphere_mesh, 7, &vulkan_state, vk_res)) != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }
    if ((gfx_res = mesh_init_cone(&cone_mesh, 1 << 4, &vulkan_state, vk_res)) != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create cone mesh: %s", gfx_result_to_str(gfx_res));
    }


    //  This is the truss mesh :)
    f32 radius_factor = 1.0f;  //  This could be a config option
    for (u32 i = 0; i < elements.count; ++i)
    {
        if ((gfx_res = truss_mesh_add_between_pts(
                &truss_mesh, (jfw_color) { .r = 0xD0, .g = 0xD0, .b = 0xD0, .a = 0xFF },
                radius_factor * profile_list.equivalent_radius[elements.i_profile[i]],
                VEC4(point_list.p_x[elements.i_point0[i]], point_list.p_y[elements.i_point0[i]],
                     point_list.p_z[elements.i_point0[i]]), VEC4(point_list.p_x[elements.i_point1[i]],
                                                             point_list.p_y[elements.i_point1[i]],
                                                             point_list.p_z[elements.i_point1[i]]), 0.0f, &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add element %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }
    //  These are the joints
    uint32_t* bcs_per_point = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*bcs_per_point) * point_list.count);
    memset(bcs_per_point, 0, sizeof(*bcs_per_point) * point_list.count);
    for (u32 i = 0; i < numerical_boundary_conditions.count; ++i)
    {
        bcs_per_point[numerical_boundary_conditions.i_point[i]] +=
                ((numerical_boundary_conditions.type[i] & JTA_NUMERICAL_BC_TYPE_X) != 0) +
                ((numerical_boundary_conditions.type[i] & JTA_NUMERICAL_BC_TYPE_Y) != 0) +
                ((numerical_boundary_conditions.type[i] & JTA_NUMERICAL_BC_TYPE_Z) != 0);
    }
    //  These could be a config options
    const jfw_color point_colors[4] =
            {
                    {.r = 0x80, .g = 0x80, .b = 0x80, .a = 0xFF},   //  0 - just gray
                    {.r = 0xFF, .g = 0xFF, .b = 0x00, .a = 0xFF},   //  1 - yellow
                    {.r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0xFF},   //  2 - purple
                    {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF},   //  3 (or somehow more) - red
            };
    const f32 point_scales[4] =
            {
                1.0f,//  0 - just gray
                2.0f,//  1 - yellow
                2.0f,//  2 - purple
                2.0f,//  3 (or somehow more) - red
            };

    for (u32 i = 0; i < point_list.count; ++i)
    {
        jfw_color c;
        f32 r;
        switch (bcs_per_point[i])
        {
        case 0:
            c = point_colors[0];
            r = point_scales[0];
            break;
        case 1:
            c = point_colors[1];
            r = point_scales[1];
            break;
        case 2:
            c = point_colors[2];
            r = point_scales[2];
            break;
        default:
            JDM_WARN("Point \"%.*s\" has %u numerical boundary conditions applied to it", (int)point_list.label[i].len, point_list.label[i].begin, bcs_per_point[i]);
        case 3:
            c = point_colors[3];
            r = point_scales[3];
            break;
        }

        if ((gfx_res = sphere_mesh_add(&sphere_mesh, c, r * point_list.max_radius[i], VEC4(point_list.p_x[i], point_list.p_y[i], point_list.p_z[i]), &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add node %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }
    lin_jfree(G_LIN_JALLOCATOR, bcs_per_point);
    //  These are the force vectors
    const f32 max_radius_scale = 0.3f;    //  This could be a config option
    const f32 arrow_cone_ratio = 0.5f;    //  This could be a config option
    const f32 max_length_scale = 0.3f;    //  This could be a config option
    for (u32 i = 0; i < natural_boundary_conditions.count; ++i)
    {
        vec4 base = VEC4(point_list.p_x[natural_boundary_conditions.i_point[i]],
                         point_list.p_y[natural_boundary_conditions.i_point[i]],
                         point_list.p_z[natural_boundary_conditions.i_point[i]]);
        float mag = hypotf(
                hypotf(natural_boundary_conditions.x[i], natural_boundary_conditions.y[i]),
                natural_boundary_conditions.z[i]);
        const f32 real_length = mag / natural_boundary_conditions.max_mag * elements.max_len * max_length_scale;
        vec4 direction = vec4_div_one(VEC4(natural_boundary_conditions.x[i], natural_boundary_conditions.y[i], natural_boundary_conditions.z[i]), mag);
        vec4 second = vec4_add(base, vec4_mul_one(direction, real_length * (1 - arrow_cone_ratio)));
        vec4 third = vec4_add(base, vec4_mul_one(direction, real_length));
        if ((gfx_res = cone_mesh_add_between_pts(&cone_mesh, (jfw_color){.b = 0xD0, .a = 0xFF}, 2 * max_radius_scale * profile_list.max_equivalent_radius,
                                                 second, third, &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add force %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
        if ((gfx_res = truss_mesh_add_between_pts(&truss_mesh, (jfw_color){.b = 0xD0, .a = 0xFF}, max_radius_scale * profile_list.max_equivalent_radius,
                                                 base, second, 0.0f, &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add force %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }
    jta_mesh* meshes[] = {
            &truss_mesh,
            &sphere_mesh,
            &cone_mesh
    };

    vulkan_state.mesh_count = 3;
    vulkan_state.mesh_array = meshes + 0;


    printf("Total of %"PRIu64" triangles in the mesh\n", mesh_polygon_count(&truss_mesh) + mesh_polygon_count(&sphere_mesh) + mesh_polygon_count(&cone_mesh));

    jfw_window_set_usr_ptr(jwnd, &vulkan_state);
    jfw_widget* jwidget;
    jfw_result = jfw_widget_create_as_base(jwnd, 1600, 900, 0, 0, &jwidget);
    if (!jfw_success(jfw_result))
    {
        JDM_ERROR("Could not create window's base widget");
        goto cleanup;
    }
    jwidget->dtor_fn = widget_dtor;
    jwidget->draw_fn = widget_draw;
    jwidget->functions.mouse_button_press = truss_mouse_button_press;
    jwidget->functions.mouse_button_release = truss_mouse_button_release;
    jwidget->functions.mouse_motion = truss_mouse_motion;
    jwidget->functions.button_up = truss_key_press;
    jta_camera_3d camera;
    jta_camera_set(
            &camera,                                    //  Camera
            geo_base,                                   //  View target
            geo_base,                                   //  Geometry center
            geo_radius,                                 //  Geometry radius
            VEC4(0, 0, -1),                             //  Down
            vec4_sub(geo_base, vec4_mul_one(VEC4(0, -1, 0), geo_radius)), //  Camera position
            4.0f,                                       //  Turn sensitivity
            1.0f                                        //  Move sensitivity
            );
#ifndef NDEBUG
    f32 min = +INFINITY, max = -INFINITY;
    for (u32 i = 0; i < point_list.count; ++i)
    {
        vec4 p = VEC4(point_list.p_x[i], point_list.p_y[i], point_list.p_z[i]);
        p = vec4_sub(p, camera.position);
        f32 d = vec4_dot(p, camera.uz);
        if (d < min)
        {
            min = d;
        }
        if (d > max)
        {
            max = d;
        }
    }
    f32 n, f;
    jta_camera_find_depth_planes(&camera, &n, &f);
//    assert(min >= n); assert(max <= f);
#endif
    jta_draw_state draw_state =
            {
            .vulkan_state = &vulkan_state,
            .camera = camera,
            .vulkan_resources = vk_res,
            };
    jfw_widget_set_user_pointer(jwidget, &draw_state);
    vulkan_state.view = jta_camera_to_view_matrix(&camera);

    i32 close = 0;
    while (jfw_success(jfw_context_wait_for_events(jctx)) && !close)
    {
        while (jfw_context_has_events(jctx) && !close)
        {
            close = !jfw_success(jfw_context_process_events(jctx));
        }
        if (!close)
        {
            jfw_window_force_redraw(jctx, jwnd);
        }
    }
//    vk_state_destroy(&vulkan_state, vk_res);
    jwnd = NULL;

cleanup:
    mesh_uninit(&truss_mesh);
    mesh_uninit(&sphere_mesh);
    mesh_uninit(&cone_mesh);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.type);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.i_point);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.x);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.y);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.z);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.i_point);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.x);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.y);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.z);
    ill_jfree(G_JALLOCATOR, elements.lengths);
    ill_jfree(G_JALLOCATOR, elements.labels);
    ill_jfree(G_JALLOCATOR, elements.i_material);
    ill_jfree(G_JALLOCATOR, elements.i_profile);
    ill_jfree(G_JALLOCATOR, elements.i_point0);
    ill_jfree(G_JALLOCATOR, elements.i_point1);
    ill_jfree(G_JALLOCATOR, profile_list.equivalent_radius);
    ill_jfree(G_JALLOCATOR, profile_list.area);
    ill_jfree(G_JALLOCATOR, profile_list.second_moment_of_area);
    ill_jfree(G_JALLOCATOR, profile_list.labels);
    ill_jfree(G_JALLOCATOR, materials);
    ill_jfree(G_JALLOCATOR, point_list.p_x);
    ill_jfree(G_JALLOCATOR, point_list.p_y);
    ill_jfree(G_JALLOCATOR, point_list.p_z);
    ill_jfree(G_JALLOCATOR, point_list.label);
    ill_jfree(G_JALLOCATOR, point_list.max_radius);
    jio_memory_file_destroy(&file_elements);
    jio_memory_file_destroy(&file_profiles);
    jio_memory_file_destroy(&file_materials);
    jio_memory_file_destroy(&file_points);
    if (jctx)
    {
        jfw_context_destroy(jctx);
        jctx = NULL;
    }
    JDM_LEAVE_FUNCTION;
    jdm_cleanup_thread();
    //  Clean up the allocators
    {
        jallocator* const allocator = G_JALLOCATOR;
        G_JALLOCATOR = NULL;
#ifndef NDEBUG
        uint_fast32_t leaked_block_indices[256];
        uint_fast32_t leaked_block_count = ill_jallocator_count_used_blocks(allocator, sizeof(leaked_block_indices) / sizeof(*leaked_block_indices), leaked_block_indices);
        for (u32 i = 0; i < leaked_block_count; ++i)
        {
            fprintf(stderr, "G_JALLOCATOR did not free block %"PRIuFAST32"\n", leaked_block_indices[i]);
        }
#endif
        ill_jallocator_destroy(allocator);
    }
    {
        jallocator* const allocator = G_LIN_JALLOCATOR;
        G_LIN_JALLOCATOR = NULL;
        lin_jallocator_destroy(allocator);
    }
    {
        printf("Total lifetime waste by aligned allocator: %zu\n", aligned_jallocator_lifetime_waste(G_ALIGN_JALLOCATOR));
        aligned_jallocator* const allocator = G_ALIGN_JALLOCATOR;
        G_ALIGN_JALLOCATOR = NULL;
        aligned_jallocator_destroy(allocator);
    }
    return 0;
}
