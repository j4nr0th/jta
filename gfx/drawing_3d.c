//
// Created by jan on 18.5.2023.
//

#include <time.h>
#include <jdm.h>
#include "drawing_3d.h"
#include "camera.h"

gfx_result draw_3d_scene(
        jfw_window* wnd, vk_state* state, jfw_window_vk_resources* vk_resources, vk_buffer_allocation* p_buffer_geo,
        vk_buffer_allocation* p_buffer_mod, const jtb_mesh* mesh, const jtb_camera_3d* camera)
{
    assert(p_buffer_geo);
    assert(p_buffer_mod);
    assert(mesh);
    const jfw_window_vk_resources* ctx = jfw_window_get_vk_resources(wnd);
    u32 i_frame = state->frame_index;
    VkResult vk_res =  vkWaitForFences(ctx->device, 1, ctx->swap_fences + i_frame, VK_TRUE, UINT64_MAX);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed waiting for the fence for swapchain: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_SWAPCHAIN_WAIT;
    }
    u32 img_index;
    vk_res = vkAcquireNextImageKHR(ctx->device, ctx->swapchain, UINT64_MAX, ctx->sem_img_available[i_frame], VK_NULL_HANDLE, &img_index);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed acquiring next image in the swapchain: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_SWAPCHAIN_IMG;
    }
    switch (vk_res)
    {
    case VK_NOT_READY: return GFX_RESULT_SUCCESS;
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR: break;
    case VK_ERROR_OUT_OF_DATE_KHR:JDM_ERROR("Swapchain was out of date");
        return GFX_RESULT_SWAPCHAIN_OUT_OF_DATE;
    default: { assert(0); } return GFX_RESULT_UNEXPECTED;
    }


    vk_res = vkResetFences(ctx->device, 1, ctx->swap_fences + i_frame);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed resetting fence for swapchain: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_FENCE_RESET;
    }
    vk_res = vkResetCommandBuffer(ctx->cmd_buffers[i_frame], 0);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed resetting command buffer for swapchain: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_CMDBUF_RESET;
    }

    //  issue drawing commands
    VkCommandBuffer cmd_buffer = vk_resources->cmd_buffers[i_frame];
    {
        VkCommandBufferBeginInfo cmd_buffer_begin_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pInheritanceInfo = NULL,
                .flags = 0,
                };
        vk_res = vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info);
        assert(vk_res == VK_SUCCESS);
        VkClearValue clear_color =
                {
                .color = {0.0f, 0.0f, 0.0f, 0.0f},
                };
        VkClearValue clear_ds =
                {
                .depthStencil = {1.0f, 0},
                };
        VkClearValue clear_array[2] =
                {
                clear_color, clear_ds
                };
        VkRenderPassBeginInfo render_pass_3d_info =
                {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = state->render_pass_3D,
                .renderArea.offset = {0,0},
                .renderArea.extent = vk_resources->extent,
                .clearValueCount = 2,
                .pClearValues = clear_array,
                .framebuffer = state->framebuffers[img_index],
                };

        //  Drawing the truss model
        vkCmdBeginRenderPass(cmd_buffer, &render_pass_3d_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmd_buffer, 0, 1, &state->viewport);
        vkCmdSetScissor(cmd_buffer, 0, 1, &state->scissor);
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->gfx_pipeline_3D);
        VkBuffer buffers[2] = {p_buffer_geo->buffer, p_buffer_mod->buffer};
        VkDeviceSize offsets[2] = { p_buffer_geo->offset, p_buffer_mod->offset};
        vkCmdBindVertexBuffers(cmd_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(cmd_buffer, state->buffer_idx.buffer, state->buffer_idx.offset, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->layout_3D, 0, 1, state->desc_set + i_frame, 0, NULL);
        vkCmdDrawIndexed(cmd_buffer, mesh->model.idx_count, mesh->count, 0, 0, 0);
        vkCmdEndRenderPass(cmd_buffer);

        //  Drawing the coordinate frame

        VkRenderPassBeginInfo render_pass_cf_info =
                {
                        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                        .renderPass = state->render_pass_cf,
                        .clearValueCount = 2,
                        .pClearValues = clear_array,
                        .framebuffer = state->framebuffers[img_index],
                };
        VkExtent2D extent_cf = vk_resources->extent;
        VkOffset2D offset_cf = { (int32_t)vk_resources->extent.width, 0 };

        {
            extent_cf.width >>= 3;
            extent_cf.height >>= 3;
            if (extent_cf.width > extent_cf.height)
            {
                extent_cf.width = extent_cf.height;
            }
            else
            {
                extent_cf.height = extent_cf.width;
            }
            offset_cf.x -= (int32_t)extent_cf.width;

            //  Configure cf render pass
            render_pass_cf_info.renderArea.offset = offset_cf;
            render_pass_cf_info.renderArea.extent = extent_cf;
        }
        vkCmdBeginRenderPass(cmd_buffer, &render_pass_cf_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdEndRenderPass(cmd_buffer);

        vk_res = vkEndCommandBuffer(cmd_buffer);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed completing command buffer recording: %s", jfw_vk_error_msg(vk_res));
            return GFX_RESULT_BAD_CMDBUF_END;
        }
    }

    //  Update uniforms
    {
        f32 near, far;
//        jtb_camera_find_depth_planes(camera, &near, &far);
        gfx_find_bounding_planes(state->point_list, camera->position, camera->uz, &near, &far);
        ubo_3d ubo =
                {
                        .proj = mtx4_projection(M_PI_2, ((f32)vk_resources->extent.width)/((f32)vk_resources->extent.height), 1.0f, near, far),
                        .view = state->view,
                        .view_direction = camera->uz,
                };
        memcpy(state->p_mapped_array[i_frame], &ubo, sizeof(ubo));
    }

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
//    assert(vk_res == VK_SUCCESS);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not submit render queue, reason: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_QUEUE_SUBMIT;
    }
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
    state->frame_index = (i_frame + 1) % ctx->n_frames_in_flight;
    switch (vk_res)
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR: /*printf("Draw fn worked!\n");*/ return GFX_RESULT_SUCCESS;
    case VK_ERROR_OUT_OF_DATE_KHR:
    {
        JDM_ERROR("Swapchain became outdated");
    }
    return GFX_RESULT_SWAPCHAIN_OUT_OF_DATE;
    default: { assert(0); } return GFX_RESULT_UNEXPECTED;
    }

    return GFX_RESULT_SUCCESS;
}
