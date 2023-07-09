//
// Created by jan on 18.5.2023.
//

#include <time.h>
#include <jdm.h>
#include "drawing_3d.h"
#include "camera.h"
#include <inttypes.h>

gfx_result
draw_frame(
        vk_state* state, jfw_window_vk_resources* vk_resources, u32 n_meshes, jtb_mesh** meshes,
        const jtb_camera_3d* camera)
{
    assert(meshes);
    for (u32 i = 0; i < n_meshes; ++i)
    {
        jtb_mesh* mesh = meshes[i];
        if (mesh->up_to_date == 0)
        {
            gfx_result res = jtb_mesh_update_instance(mesh, vk_resources, state);
            if (res != GFX_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not update mesh %"PRIu32" instance data, reason: %s", i, gfx_result_to_str(res));
            }
        }
    }

    u32 i_frame = state->frame_index;
    VkResult vk_res =  vkWaitForFences(vk_resources->device, 1, vk_resources->swap_fences + i_frame, VK_TRUE, UINT64_MAX);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed waiting for the fence for swapchain: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_SWAPCHAIN_WAIT;
    }
    u32 img_index;
    vk_res = vkAcquireNextImageKHR(vk_resources->device, vk_resources->swapchain, UINT64_MAX, vk_resources->sem_img_available[i_frame], VK_NULL_HANDLE, &img_index);
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


    vk_res = vkResetFences(vk_resources->device, 1, vk_resources->swap_fences + i_frame);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed resetting fence for swapchain: %s", jfw_vk_error_msg(vk_res));
        return GFX_RESULT_BAD_FENCE_RESET;
    }
    vk_res = vkResetCommandBuffer(vk_resources->cmd_buffers[i_frame], 0);
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
        for (u32 i = 0; i < n_meshes; ++i)
        {
            const jtb_mesh* mesh = meshes[i];
            if (mesh->count == 0) continue;
            VkBuffer buffers[2] = { mesh->common_geometry_vtx.buffer, mesh->instance_memory.buffer};
            VkDeviceSize offsets[2] = { mesh->common_geometry_vtx.offset, mesh->instance_memory.offset};
            vkCmdBindVertexBuffers(cmd_buffer, 0, 2, buffers, offsets);
            vkCmdBindIndexBuffer(cmd_buffer, mesh->common_geometry_idx.buffer, mesh->common_geometry_idx.offset, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->layout_3D, 0, 1, state->desc_set + i_frame, 0, NULL);
            vkCmdDrawIndexed(cmd_buffer, mesh->model.idx_count, mesh->count, 0, 0, 0);
        }
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
//        printf("Near %g, far %g\n", near, far);
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
                    .pCommandBuffers = vk_resources->cmd_buffers + i_frame,
                    .signalSemaphoreCount = 1,
                    .pSignalSemaphores = vk_resources->sem_present + i_frame,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = vk_resources->sem_img_available + i_frame,
                    .pWaitDstStageMask = stage_flags,
            };
    vk_res = vkQueueSubmit(vk_resources->queue_gfx, 1, &submit_info, vk_resources->swap_fences[i_frame]);
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
                    .pSwapchains = &vk_resources->swapchain,
                    .pImageIndices = &img_index,
                    .waitSemaphoreCount = 1,
                    .pWaitSemaphores = vk_resources->sem_present + i_frame,
            };
    vk_res = vkQueuePresentKHR(vk_resources->queue_present, &present_info);
    state->frame_index = (i_frame + 1) % vk_resources->n_frames_in_flight;
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
