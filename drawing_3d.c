//
// Created by jan on 18.5.2023.
//

#include <time.h>
#include "drawing_3d.h"

jfw_res draw_3d_scene(jfw_window* wnd, vk_state* state, jfw_window_vk_resources* vk_resources)
{

    const jfw_window_vk_resources* ctx = jfw_window_get_vk_resources(wnd);
    u32 i_frame = state->frame_index;
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
    case VK_ERROR_OUT_OF_DATE_KHR:JFW_ERROR("Swapchain was out of date");
        return jfw_res_error;
    default: { assert(0); } return jfw_res_error;
    }


    vk_res = vkResetFences(ctx->device, 1, ctx->swap_fences + i_frame);
    assert(vk_res == VK_SUCCESS);
    vk_res = vkResetCommandBuffer(ctx->cmd_buffers[i_frame], 0);
    assert(vk_res == VK_SUCCESS);

    //  issue drawing commands
    const VkCommandBuffer cmd_buffer = vk_resources->cmd_buffers[i_frame];
    {
        VkCommandBufferBeginInfo cmd_buffer_begin_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pInheritanceInfo = NULL,
                .flags = 0,
                };
        vk_res = vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info);
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
        VkRenderPassBeginInfo render_pass_info =
                {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = state->render_pass_3D,
                .renderArea.offset = {0,0},
                .renderArea.extent = vk_resources->extent,
                .clearValueCount = 2,
                .pClearValues = clear_array,
                .framebuffer = state->framebuffers[img_index],
                };

        vkCmdBeginRenderPass(cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(cmd_buffer, 0, 1, &state->viewport);
        vkCmdSetScissor(cmd_buffer, 0, 1, &state->scissor);
        vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->gfx_pipeline_3D);
        vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &state->buffer_vtx.buffer, &state->buffer_vtx.offset);
        vkCmdBindIndexBuffer(cmd_buffer, state->buffer_idx.buffer, state->buffer_idx.offset, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->layout_3D, 0, 1, state->desc_set + i_frame, 0, NULL);
        vkCmdDrawIndexed(cmd_buffer, 12, 1, 0, 0, 0);
        vkCmdEndRenderPass(cmd_buffer);

        vk_res = vkEndCommandBuffer(cmd_buffer);
        if (vk_res != VK_SUCCESS)
        {
            JFW_ERROR("Failed completing command buffer recording: %s", jfw_vk_error_msg(vk_res));
            return jfw_res_vk_fail;
        }
    }

    //  Update uniforms
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
        ubo_3d ubo =
                {
                        .model = euler_rotation_x(dt * 0.01),
                        .proj = mtx4_projection(M_PI_2, ((f32)vk_resources->extent.width)/((f32)vk_resources->extent.height), 0.5f * 1980.0f/(f32)vk_resources->extent.width),
                        .view = mtx4_view_look_at(
                                (vec4){.y = -3, .x = 0, .z = -10.0f},
                                (vec4){.x = 0.0f, .y = 0.0f, .z = 0.0f},
                                M_PI
                                                 ),
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
    state->frame_index = (i_frame + 1) % ctx->n_frames_in_flight;
    switch (vk_res)
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR: printf("Draw fn worked!\n"); return jfw_res_success;
    case VK_ERROR_OUT_OF_DATE_KHR:
    {
        JFW_ERROR("Swapchain became outdated");
    }
    return jfw_res_error;
    default: { assert(0); } return jfw_res_error;
    }

    return jfw_res_success;
}
