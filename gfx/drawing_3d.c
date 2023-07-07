//
// Created by jan on 18.5.2023.
//

#include <time.h>
#include <jdm.h>
#include "drawing_3d.h"
#include "camera.h"

gfx_result draw_3d_scene(
        jfw_window* wnd, vk_state* state, jfw_window_vk_resources* vk_resources, vk_buffer_allocation* p_buffer_geo,
        vk_buffer_allocation* p_buffer_mod, const jtb_truss_mesh* mesh, const jtb_camera_3d* camera)
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
        gfx_find_bounding_planes(&state->geometry, camera->position, camera->uz, &near, &far);
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

static gfx_result clean_truss_model(jtb_model* model)
{
    jfw_free(&model->vtx_array);
    jfw_free(&model->idx_array);
    memset(model, 0, sizeof*model);
    return GFX_RESULT_SUCCESS;
}

static gfx_result generate_truss_model(jtb_model* const p_out, const u16 pts_per_side)
{
    gfx_result res;
    jfw_res jfw_result;
    jtb_vertex* vertices;
    if (!jfw_success(jfw_result = (jfw_calloc(2 * pts_per_side, sizeof*vertices, &vertices))))
    {
        JDM_ERROR("Could not allocate memory for truss model, reason: %s", jfw_error_message(jfw_result));
        return GFX_RESULT_BAD_ALLOC;
    }
    jtb_vertex* const btm = vertices;
    jtb_vertex* const top = vertices + pts_per_side;
    u16* indices;
    if (!jfw_success(jfw_result = (jfw_calloc(3 * 2 * pts_per_side, sizeof*indices, &indices))))
    {
        JDM_ERROR("Could not allocate memory for truss model, reason: %s", jfw_error_message(jfw_result));
        jfw_free(&vertices);
        return res;
    }


    const f32 d_omega = M_PI / pts_per_side;
    //  Generate bottom side

    for (u32 i = 0; i < pts_per_side; ++i)
    {
        btm[i].nx = btm[i].x = cosf(d_omega * (f32)(2 * i));
        btm[i].ny = btm[i].y = sinf(d_omega * (f32)(2 * i));
        btm[i].nz = btm[i].z = 0.0f;

        top[i].nx = top[i].x = cosf(d_omega * (f32)(2 * i + 1));
        top[i].ny = top[i].y = sinf(d_omega * (f32)(2 * i + 1));
        top[i].z = 1.0f; top[i].nz = 0.0f;
        //  Add top and bottom triangles
        indices[3 * i + 0] = i;
        indices[3 * i + 1] = i + 1;
        indices[3 * i + 2] = i + pts_per_side;
        indices[3 * (i + pts_per_side) + 0] = i + pts_per_side;
        indices[3 * (i + pts_per_side) + 1] = i + 1;
        indices[3 * (i + pts_per_side) + 2] = i + pts_per_side + 1;
    }

    //  Correct the final triangles
    indices[3 * (pts_per_side - 1) + 0] = (pts_per_side - 1);
    indices[3 * (pts_per_side - 1) + 1] = 0;
    indices[3 * (pts_per_side - 1) + 2] = (pts_per_side - 1) + pts_per_side;
    indices[3 * ((pts_per_side - 1) + pts_per_side) + 0] = (pts_per_side - 1) + pts_per_side;
    indices[3 * ((pts_per_side - 1) + pts_per_side) + 1] = 0;//(pts_per_side - 1);
    indices[3 * ((pts_per_side - 1) + pts_per_side) + 2] = pts_per_side;

    p_out->vtx_count = 2 * pts_per_side;
    p_out->idx_count = 2 * 3 * pts_per_side;
    p_out->vtx_array = vertices;
    p_out->idx_array = indices;

    return GFX_RESULT_SUCCESS;
}

const u64 DEFAULT_MESH_CAPACITY = 64;

gfx_result truss_mesh_init(jtb_truss_mesh* mesh, u16 pts_per_side)
{
    jfw_res res;
    gfx_result gfx_res;
    if ((gfx_res = generate_truss_model(&mesh->model, pts_per_side)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not generate a truss model for a mesh");
        return gfx_res;
    }
    mesh->count = 0;
    mesh->capacity = DEFAULT_MESH_CAPACITY;
    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->colors), &mesh->colors)))
    {
        JDM_ERROR("Could not allocate memory for mesh color array, reason: %s", jfw_error_message(res));
        clean_truss_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->model_matrices), &mesh->model_matrices)))
    {
        JDM_ERROR("Could not allocate memory for mesh model matrix array, reason: %s", jfw_error_message(res));
        jfw_free(&mesh->colors);
        clean_truss_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->normal_matrices), &mesh->normal_matrices)))
    {
        JDM_ERROR("Could not allocate memory for mesh normal matrix array, reason: %s", jfw_error_message(res));
        jfw_free(&mesh->colors);
        jfw_free(&mesh->model_matrices);
        clean_truss_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    return GFX_RESULT_SUCCESS;
}

gfx_result truss_mesh_uninit(jtb_truss_mesh* mesh)
{
    jfw_free(&mesh->model_matrices);
    jfw_free(&mesh->colors);
    clean_truss_model(&mesh->model);
    return GFX_RESULT_SUCCESS;
}

gfx_result truss_mesh_add_new(jtb_truss_mesh* mesh, mtx4 model_transform, mtx4 normal_transform, jfw_color color)
{
    jfw_res res;
    if (mesh->count == mesh->capacity)
    {
        const u32 new_capacity = mesh->capacity + 64;
        if (!jfw_success(res = jfw_realloc(new_capacity * sizeof(*mesh->colors), &mesh->colors)))
        {
            JDM_ERROR("Could not reallocate memory for mesh color array");
            return GFX_RESULT_BAD_ALLOC;
        }
        if (!jfw_success(res = jfw_realloc(new_capacity * sizeof(*mesh->model_matrices), &mesh->model_matrices)))
        {
            JDM_ERROR("Could not reallocate memory for mesh model matrix array");
            return GFX_RESULT_BAD_ALLOC;
        }
        if (!jfw_success(res = jfw_realloc(new_capacity * sizeof(*mesh->normal_matrices), &mesh->normal_matrices)))
        {
            JDM_ERROR("Could not reallocate memory for mesh normal matrix array");
            return GFX_RESULT_BAD_ALLOC;
        }
        mesh->capacity = new_capacity;
    }

    mesh->colors[mesh->count] = color;
    mesh->model_matrices[mesh->count] = model_transform;
    mesh->normal_matrices[mesh->count] = normal_transform;
    mesh->count += 1;

    return GFX_RESULT_SUCCESS;
}

gfx_result truss_mesh_add_between_pts(jtb_truss_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll)
{
    assert(pt2.w == 1.0f);
    assert(pt1.w == 1.0f);
    vec4 d = vec4_sub(pt2, pt1);
    const f32 len2 = vec4_dot(d, d);
    const f32 len = sqrtf(len2);
    const f32 rotation_y = acosf(d.z / len);
    const f32 rotation_z = atan2f(d.y, d.x);

    //  Grouped all the matrix operations together in an effort to make them easier to optimize by the compiler

    // first deform the model
    mtx4 model = mtx4_enlarge(radius, radius, len);
    // roll the model
    mtx4 normal_transform = mtx4_rotation_z(roll);
//    model = mtx4_multiply(mtx4_rotation_z(roll), model);
    //  rotate about y-axis
    normal_transform = mtx4_multiply(mtx4_rotation_y(-rotation_y), normal_transform);
    //    model = mtx4_multiply(mtx4_rotation_y(-rotation_y), model);
    //  rotate about z-axis again
    normal_transform = mtx4_multiply(mtx4_rotation_z(-rotation_z), normal_transform);
    //    model = mtx4_multiply(mtx4_rotation_z(-rotation_z), model);
    model = mtx4_multiply(normal_transform, model);
    //  move the model to the point pt1
    model = mtx4_translate(model, (pt1));

    return truss_mesh_add_new(mesh, model, normal_transform, color);
}


static jfw_res clean_sphere_model(jtb_model* model)
{
    jfw_free(&model->vtx_array);
    jfw_free(&model->idx_array);
    memset(model, 0, sizeof*model);
    return jfw_res_success;
}

static jfw_res generate_sphere_model(jtb_model* const p_out, const u16 nw, const u16 nh)
{
    assert(nw >= 3);
    assert(nh >= 1);
    jfw_res res;
    jtb_vertex* vertices;
    const u32 vertex_count = 2 + nw * nh;
    if (!jfw_success(res = (jfw_calloc(vertex_count, sizeof*vertices, &vertices))))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        return res;
    }

    u16* indices;
    const u32 index_count = 3 * 2 * nw * (nh + 1);
    if (!jfw_success(res = (jfw_calloc(index_count, sizeof*indices, &indices))))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        jfw_free(&vertices);
        return res;
    }


    const f32 d_theta = M_2_PI / nw;
    const f32 d_phi = M_PI / (nh - 2);
    u32 i_vtx = 0, i_idx = 0;
    //  Generate the vertices
    for (u32 i_ring = 0; i_ring < nh; ++i_ring)
    {
        const f32 phi = d_phi * ((f32) i_ring + 1.0f);
        const f32 z = cosf(phi);
        const f32 sp = sinf(phi);
        for (u32 i_column = 0; i_column < nw; ++i_column)
        {
            const f32 theta = (f32)i_column * d_theta;
            vertices[i_vtx++] = (jtb_vertex)
                    {
                .x = cosf(theta) * sp,
                .y = sinf(theta) * sp,
                .z = z,
                    };
        }
    }
    assert(i_vtx == nw * nh);
    //  Add top and bottom indices
    const u32 i_top = i_vtx++;
    const u32 i_btm = i_vtx++;
    vertices[i_top] = (jtb_vertex){.x=0.0f, .y=0.0f, .z=+1.0f};
    vertices[i_btm] = (jtb_vertex){.x=0.0f, .y=0.0f, .z=-1.0f};
    assert(i_vtx == vertex_count);

    //  Now generate the triangles for the bottom part of the ring connections
    for (u32 i_ring = 0; i_ring < nh - 1; ++i_ring)
    {
        for (u32 i_column = 0; i_column < nw; ++i_column)
        {
            const u32 i_pt = i_column + i_ring * nw;
            indices[i_idx++] = i_pt;
            assert(i_idx < index_count);
            indices[i_idx++] = i_pt + nw;
            assert(i_idx < index_count);
            indices[i_idx++] = (i_pt + 1) % nw;
            assert(i_idx < index_count);
        }
    }

    //  Generate the triangles for the top part of the ring connections
    for (u32 i_ring = 1; i_ring < nh; ++i_ring)
    {
        for (u32 i_column = 0; i_column < nw; ++i_column)
        {
            const u32 i_pt = i_column + i_ring * nw;
            indices[i_idx++] = i_pt;
            assert(i_idx < index_count);
            indices[i_idx++] = (i_pt + 1) % nw;
            assert(i_idx < index_count);
            indices[i_idx++] = i_pt - nw;
            assert(i_idx < index_count);
        }
    }

    //  Generate the triangles for the very top of the sphere
    for (u32 i_column = 0; i_column < nw; ++i_column)
    {
        indices[i_idx++] = i_top;
        assert(i_idx < index_count);
        indices[i_idx++] = i_column;
        assert(i_idx < index_count);
        indices[i_idx++] = (i_column + 1) % nw;
        assert(i_idx < index_count);
    }
    //  Generate the triangles for the very bottom of the sphere
    for (u32 i_column = 0; i_column < nw; ++i_column)
    {
        const u32 i_pt = i_column + nw * (nh - 1);
        indices[i_idx++] = i_btm;
        assert(i_idx < index_count);
        indices[i_idx++] = (i_pt + 1) % nw;
        assert(i_idx < index_count);
        indices[i_idx++] = i_pt;
    }

    assert(i_idx == index_count);
    

    p_out->vtx_count = vertex_count;
    p_out->idx_count = index_count;
    p_out->vtx_array = vertices;
    p_out->idx_array = indices;

    return jfw_res_success;
}

gfx_result sphere_mesh_init(jtb_sphere_mesh* mesh, u16 nw, u16 nh)
{
    jfw_res res;
    if (!jfw_success(generate_sphere_model(&mesh->model, nw, nh)))
    {
        JDM_ERROR("Could not generate a sphere model for a mesh");
        return GFX_RESULT_BAD_ALLOC;
    }
    mesh->count = 0;
    mesh->capacity = DEFAULT_MESH_CAPACITY;
    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->colors), &mesh->colors)))
    {
        JDM_ERROR("Could not allocate memory for mesh color array");
        clean_sphere_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->model_matrices), &mesh->model_matrices)))
    {
        JDM_ERROR("Could not allocate memory for mesh model matrix array");
        jfw_free(&mesh->colors);
        clean_sphere_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    return GFX_RESULT_SUCCESS;
}

gfx_result sphere_mesh_add_new(jtb_sphere_mesh* mesh, mtx4 model_transform, jfw_color color)
{
    if (mesh->count == mesh->capacity)
    {
        const u32 new_capacity = mesh->capacity + 64;
        if (!jfw_success(jfw_realloc(new_capacity * sizeof(*mesh->colors), &mesh->colors)))
        {
            JDM_ERROR("Could not reallocate memory for mesh color array");
            return GFX_RESULT_BAD_ALLOC;
        }
        if (!jfw_success(jfw_realloc(new_capacity * sizeof(*mesh->model_matrices), &mesh->model_matrices)))
        {
            JDM_ERROR("Could not reallocate memory for mesh model matrix array");
            return GFX_RESULT_BAD_ALLOC;
        }
        mesh->capacity = new_capacity;
    }

    mesh->colors[mesh->count] = color;
    mesh->model_matrices[mesh->count] = model_transform;
    mesh->count += 1;

    return GFX_RESULT_SUCCESS;
}

gfx_result sphere_mesh_uninit(jtb_sphere_mesh* mesh)
{
    jfw_free(&mesh->model_matrices);
    jfw_free(&mesh->colors);
    clean_truss_model(&mesh->model);
    return GFX_RESULT_SUCCESS;
}

gfx_result sphere_mesh_add_at_location(jtb_sphere_mesh* mesh, jfw_color color, f32 radius, vec4 pt)
{
    mtx4 m = mtx4_enlarge(radius, radius, radius);
    m = mtx4_translate(m, pt);
    return sphere_mesh_add_new(mesh, m, color);
}
