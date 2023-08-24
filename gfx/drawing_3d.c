//
// Created by jan on 18.5.2023.
//

#include <time.h>
#include <jdm.h>
#include "drawing_3d.h"
#include "camera.h"
#include <inttypes.h>
#include <jdm.h>

gfx_result jta_draw_frame(
        jta_vulkan_window_context* wnd_ctx, jta_ui_state* ui_state, mtx4 view_matrix, jta_structure_meshes* meshes,
        const jta_camera_3d* camera)
{
    jta_timer timer_render;
    jta_timer_set(&timer_render);
    JDM_ENTER_FUNCTION;
    assert(meshes);
    for (u32 i = 0; i < 3; ++i)
    {
        jta_mesh* mesh = meshes->mesh_array + i;
        if (mesh->up_to_date == 0)
        {
            gfx_result res = jta_mesh_update_instance(mesh, wnd_ctx);
            if (res != GFX_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not update mesh %"PRIu32" instance data, reason: %s", i, gfx_result_to_str(res));
            }
        }
    }

    //  issue drawing commands
    VkCommandBuffer cmd_buffer;
    unsigned i_img;
    gfx_result res = jta_vulkan_begin_draw(wnd_ctx, &cmd_buffer, &i_img);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not begin draw of next frame");
        JDM_LEAVE_FUNCTION;
        return res;
    }


    VkClearValue clear_color =
            {
                    .color = {.float32={0.0f, 0.0f, 0.0f, 0.0f}},
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
                    .renderPass = wnd_ctx->render_pass_mesh,
                    .renderArea.offset = {0,0},
                    .renderArea.extent = wnd_ctx->swapchain.window_extent,
                    .clearValueCount = 2,
                    .pClearValues = clear_array,
                    .framebuffer = wnd_ctx->pass_mesh.framebuffers[i_img],
            };

    //  Drawing the truss model
    vkCmdBeginRenderPass(cmd_buffer, &render_pass_3d_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(cmd_buffer, 0, 1, &wnd_ctx->viewport);
    vkCmdSetScissor(cmd_buffer, 0, 1, &wnd_ctx->scissor);
    //  Update uniforms
    {
        float n = INFINITY, f = FLT_EPSILON;
        for (uint32_t i = 0; i < 3; ++i)
        {
            const jta_mesh* const mesh = meshes->mesh_array + i;
            if (mesh->count == 0)
            {
                continue;
            }
            const jta_bounding_box* const bb = &mesh->bounding_box;
            vec4 positions[8] =
                    {
                            VEC4(bb->min_x, bb->min_y, bb->min_z),
                            VEC4(bb->max_x, bb->min_y, bb->min_z),

                            VEC4(bb->min_x, bb->max_y, bb->min_z),
                            VEC4(bb->max_x, bb->max_y, bb->min_z),

                            VEC4(bb->min_x, bb->min_y, bb->max_z),
                            VEC4(bb->max_x, bb->min_y, bb->max_z),

                            VEC4(bb->min_x, bb->max_y, bb->max_z),
                            VEC4(bb->max_x, bb->max_y, bb->max_z),
                    };
            for (uint32_t j = 0; j < 8; ++j)
            {
                float d = vec4_distance_between_in_direction(positions[j], camera->position, camera->uz);
                if (d < n)
                {
                    n = d;
                }
                if (d > f)
                {
                    f = d;
                }
            }
        }

        if (n < 0.01f)
        {
            n = 0.01f;
        }
        if (f > n * 10000)
        {
            n = f / 10000.0f;
        }

        ubo_3d ubo =
                {
                        .proj = mtx4_projection(
                                M_PI_2 * (1), ((f32) wnd_ctx->swapchain.window_extent.width) /
                                              ((f32) wnd_ctx->swapchain.window_extent.height), 1.0f, n, f),
                        .view = view_matrix,
                        .view_direction = camera->uz,
                };
        vkCmdPushConstants(cmd_buffer, wnd_ctx->layout_mesh, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ubo), &ubo);
    }
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd_ctx->pipeline_mesh);
    for (u32 i = 0; i < 3; ++i)
    {
        const jta_mesh* mesh = meshes->mesh_array + i;
        if (mesh->count == 0) continue;
        VkBuffer buffers[2] = { mesh->common_geometry_vtx.buffer, mesh->instance_memory.buffer };
        VkDeviceSize offsets[2] = { mesh->common_geometry_vtx.offset, mesh->instance_memory.offset };
        vkCmdBindVertexBuffers(cmd_buffer, 0, 2, buffers, offsets);
        vkCmdBindIndexBuffer(
                cmd_buffer, mesh->common_geometry_idx.buffer, mesh->common_geometry_idx.offset,
                VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd_buffer, mesh->model.idx_count, mesh->count, 0, 0, 0);
    }

    vkCmdEndRenderPass(cmd_buffer);
    //  Drawing the coordinate frame

    VkRenderPassBeginInfo render_pass_cf_info =
            {
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .renderPass = wnd_ctx->render_pass_cf,
                    .clearValueCount = 2,
                    .pClearValues = clear_array,
                    .framebuffer = wnd_ctx->pass_cf.framebuffers[i_img],
            };
    VkExtent2D extent_cf = wnd_ctx->swapchain.window_extent;
    VkOffset2D offset_cf = { (int32_t) extent_cf.width, 0 };

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
        offset_cf.x -= (int32_t) extent_cf.width;

        //  Configure cf render pass
        render_pass_cf_info.renderArea.offset = offset_cf;
        render_pass_cf_info.renderArea.extent = extent_cf;
    }
    vkCmdBeginRenderPass(cmd_buffer, &render_pass_cf_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(cmd_buffer);

    VkRenderPassBeginInfo render_pass_ui_info =
            {
                    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .renderPass = wnd_ctx->render_pass_ui,
                    .clearValueCount = 1,
                    .pClearValues = clear_array,
                    .framebuffer = wnd_ctx->pass_ui.framebuffers[i_img],
            };
    vkCmdBeginRenderPass(cmd_buffer, &render_pass_ui_info, VK_SUBPASS_CONTENTS_INLINE);
    //  Draw ui
    if (ui_state->ui_vtx_buffer.size && ui_state->ui_idx_buffer.size)
    {
        size_t n_elements;
        jrui_render_element* elements;
        jrui_receive_element_data(ui_state->ui_context, &n_elements, &elements);
        if (n_elements)
        {
            vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wnd_ctx->pipeline_ui);

            const ubo_ui push_const =
                    {
                    .offset_x = +(float)wnd_ctx->swapchain.window_extent.width / 2,
                    .offset_y = +(float)wnd_ctx->swapchain.window_extent.height / 2,
                    .scale_x = 2/(float)wnd_ctx->swapchain.window_extent.width,
                    .scale_y = 2/(float)wnd_ctx->swapchain.window_extent.height,
                    };
            vkCmdSetViewport(cmd_buffer, 0, 1, &wnd_ctx->viewport);
            vkCmdSetScissor(cmd_buffer, 0, 1, &wnd_ctx->scissor);
            vkCmdPushConstants(cmd_buffer, wnd_ctx->layout_ui, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_const), &push_const);
            vkCmdBindVertexBuffers(cmd_buffer, 0, 1, &ui_state->ui_vtx_buffer.buffer, &ui_state->ui_vtx_buffer.offset);
            vkCmdBindIndexBuffer(cmd_buffer, ui_state->ui_idx_buffer.buffer, ui_state->ui_idx_buffer.offset, VK_INDEX_TYPE_UINT16);
            for (size_t i = 0; i < n_elements; ++i)
            {
                const jrui_render_element* e = elements + i;
                VkRect2D scissor =
                        {
                        .offset = {(int32_t)e->clip_region.x0, (int32_t)e->clip_region.y0},
                        .extent = {(uint32_t)(e->clip_region.x1 - e->clip_region.x0), (uint32_t)(e->clip_region.y1 - e->clip_region.y0)}
                        };
//                vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
                vkCmdDrawIndexed(cmd_buffer, e->count_idx, 1, e->first_idx, 0, 0);
            }
        }
    }

    vkCmdEndRenderPass(cmd_buffer);

    res = jta_vulkan_end_draw(wnd_ctx, cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not end drawing of the frame");
    }

//    JDM_TRACE("Time taken for rendering commands: %g ms", 1e-3 * jta_timer_get(&timer_render));
    JDM_LEAVE_FUNCTION;
    return res;
}
