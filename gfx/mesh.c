//
// Created by jan on 6.7.2023.
//

#include "mesh.h"
#include <jdm.h>
#include "bounding_box.h"

static gfx_result mesh_allocate_vulkan_memory(jta_vulkan_window_context* ctx, jta_mesh* mesh)
{
    jvm_buffer_allocation* vtx_allocation, *idx_allocation, *mod_allocation;
    const jta_model* const model = &mesh->model;
    VkBufferCreateInfo vtx_buffer_create_info =
            {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = model->vtx_count * sizeof(*model->vtx_array),
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
    VkResult vk_result = jvm_buffer_create(ctx->vulkan_allocator, &vtx_buffer_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, &vtx_allocation);

    if (vk_result != VK_SUCCESS)
    {
        JDM_ERROR("Could not allocate buffer memory for mesh vertex buffer, reason: %s (%d)", vk_result_to_str(vk_result), vk_result);
        return GFX_RESULT_BAD_ALLOC;
    }
    gfx_result gfx_res;
    if ((
                gfx_res = jta_vulkan_memory_to_buffer(
                        ctx, 0, sizeof(jta_vertex) * mesh->model.vtx_count,
                        mesh->model.vtx_array, 0, vtx_allocation)) != GFX_RESULT_SUCCESS)
    {
        jvm_buffer_destroy(vtx_allocation);
        JDM_ERROR("Could not transfer model vertex data, reason: %s", gfx_result_to_str(gfx_res));
        return gfx_res;
    }
    VkBufferCreateInfo idx_create_info =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = model->idx_count * sizeof(*model->idx_array),
                .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
    vk_result = jvm_buffer_create(ctx->vulkan_allocator, &idx_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, &idx_allocation);
//            vk_buffer_allocate(
//            ctx->vulkan_allocator, 1, model->idx_count * sizeof(*model->idx_array),
//            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &idx_allocation);
    if (vk_result != VK_SUCCESS)
    {
        jvm_buffer_destroy(vtx_allocation);
        JDM_ERROR("Could not allocate buffer memory for mesh vertex buffer, reason: %s (%d)", vk_result_to_str(vk_result), vk_result);
        return GFX_RESULT_BAD_ALLOC;
    }
    gfx_res = jta_vulkan_memory_to_buffer(
            ctx, 0, model->idx_count * sizeof(*model->idx_array),
            model->idx_array, 0, idx_allocation);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        jvm_buffer_destroy(idx_allocation);
        jvm_buffer_destroy(vtx_allocation);
        JDM_ERROR("Could not transfer model index data, reason: %s", gfx_result_to_str(gfx_res));
        return gfx_res;
    }
    VkBufferCreateInfo vtx2_create_info =
            {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = mesh->capacity * sizeof(*mesh->model_data),
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            };
    vk_result = jvm_buffer_create(ctx->vulkan_allocator, &vtx2_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, &mod_allocation);
//            vk_buffer_allocate(
//            ctx->vulkan_allocator, 1, mesh->capacity * sizeof(*mesh->model_data), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//            VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &mod_allocation);
    if (vk_result != VK_SUCCESS)
    {
        jvm_buffer_destroy(idx_allocation);
        jvm_buffer_destroy(vtx_allocation);
        JDM_ERROR("Could not allocate buffer memory for model memory, reason: %s (%d)", vk_result_to_str(vk_result), vk_result);
        return GFX_RESULT_BAD_ALLOC;
    }
    mesh->instance_memory = mod_allocation;
    mesh->common_geometry_idx = idx_allocation;
    mesh->common_geometry_vtx = vtx_allocation;
    return GFX_RESULT_SUCCESS;
}

static void clean_mesh_model(jta_model* model)
{
    ill_jfree(G_JALLOCATOR, model->vtx_array);
    ill_jfree(G_JALLOCATOR, model->idx_array);
    memset(model, 0, sizeof *model);
}

static gfx_result generate_truss_model(jta_model* const p_out, const u16 pts_per_side)
{
    jta_vertex* vertices;
    if (!(vertices = ill_jalloc(G_JALLOCATOR, (uint_fast64_t)2 * pts_per_side * sizeof *vertices)))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        return GFX_RESULT_BAD_ALLOC;
    }
    jta_vertex* const btm = vertices;
    jta_vertex* const top = vertices + pts_per_side;
    u16* indices;
    if (!(indices = ill_jalloc(G_JALLOCATOR, (uint_fast64_t)3 * 2 * pts_per_side * sizeof *indices)))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        ill_jfree(G_JALLOCATOR, vertices);
        return GFX_RESULT_BAD_ALLOC;
    }


    const f32 d_omega = M_PI / pts_per_side;
    //  Generate bottom side

    for (u32 i = 0; i < pts_per_side; ++i)
    {
        btm[i].nx = btm[i].x = cosf(d_omega * (f32) (2 * i));
        btm[i].ny = btm[i].y = sinf(d_omega * (f32) (2 * i));
        btm[i].nz = btm[i].z = 0.0f;

        top[i].nx = top[i].x = cosf(d_omega * (f32) (2 * i + 1));
        top[i].ny = top[i].y = sinf(d_omega * (f32) (2 * i + 1));
        top[i].z = 1.0f;
        top[i].nz = 0.0f;
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

gfx_result mesh_init_truss(jta_mesh* mesh, u16 pts_per_side, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;
    mesh->name = "truss";
    mesh->up_to_date = false;
    gfx_result gfx_res;
    if ((gfx_res = generate_truss_model(&mesh->model, pts_per_side)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not generate a truss model for a mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }
    mesh->count = 0;
    mesh->capacity = DEFAULT_MESH_CAPACITY;

    if (!(mesh->model_data = ill_jalloc(G_JALLOCATOR, mesh->capacity * sizeof(*mesh->model_data))))
    {
        JDM_ERROR("Could not allocate memory for mesh model array");
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }


    if ((gfx_res = mesh_allocate_vulkan_memory(ctx, mesh)) != GFX_RESULT_SUCCESS)
    {
        clean_mesh_model(&mesh->model);
        ill_jfree(G_JALLOCATOR, mesh->model_data);
        JDM_ERROR("Could not allocate vulkan memory for the truss mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }

    mesh->bounding_box.max_x = -INFINITY;
    mesh->bounding_box.max_y = -INFINITY;
    mesh->bounding_box.max_z = -INFINITY;
    mesh->bounding_box.min_x = +INFINITY;
    mesh->bounding_box.min_y = +INFINITY;
    mesh->bounding_box.min_z = +INFINITY;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

void mesh_destroy(const jta_vulkan_window_context* ctx, jta_mesh* mesh)
{
    ill_jfree(G_JALLOCATOR, mesh->model_data);
    clean_mesh_model(&mesh->model);

    jta_vulkan_context_enqueue_destroy_buffer(ctx, mesh->instance_memory);
    jta_vulkan_context_enqueue_destroy_buffer(ctx, mesh->common_geometry_idx);
    jta_vulkan_context_enqueue_destroy_buffer(ctx, mesh->common_geometry_vtx);
}

static gfx_result
mesh_add_new(
        jta_mesh* mesh, mtx4 model_transform, mtx4 normal_transform, jta_color color, jta_vulkan_window_context* ctx)
{
    mesh->up_to_date = 0;

    if (mesh->count == mesh->capacity)
    {
        //  Reallocate memory for data on host side
        const u32 new_capacity = mesh->capacity + 64;
        jta_model_data* const new_ptr = ill_jrealloc(
                G_JALLOCATOR, mesh->model_data, new_capacity * sizeof(*mesh->model_data));
        if (!new_ptr)
        {
            JDM_ERROR("Could not reallocate memory for mesh color array");
            return GFX_RESULT_BAD_ALLOC;
        }
        mesh->model_data = new_ptr;
        //  Reallocate memory on the GPU
        jvm_buffer_allocation* new_alloc;
        jvm_buffer_destroy(mesh->instance_memory);
        VkBufferCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                .size = sizeof(*mesh->model_data) * new_capacity,
                };
        VkResult vk_res = jvm_buffer_create(ctx->vulkan_allocator, &create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, &new_alloc);
//                vk_buffer_allocate(
//                ctx->vulkan_allocator, 1, sizeof(*mesh->model_data) * new_capacity,
//                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
//                &new_alloc);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not reallocate vulkan memory for the instance data, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
            create_info.size = sizeof(*mesh->model_data) * mesh->capacity;
            vk_res = jvm_buffer_create(ctx->vulkan_allocator, &create_info, 0, 0, 0, &new_alloc);
//                    vk_buffer_allocate(
//                    ctx->vulkan_allocator, 1, sizeof(*mesh->model_data) * mesh->capacity,
//                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE,
//                    &new_alloc);
            if (vk_res != VK_SUCCESS)
            {
                JDM_FATAL("Could not restore the mesh memory, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
            }
            mesh->instance_memory = new_alloc;
            return GFX_RESULT_BAD_ALLOC;
        }
        mesh->instance_memory = new_alloc;

        mesh->capacity = new_capacity;
    }

    memcpy(
            mesh->model_data[mesh->count].model_data, model_transform.data,
            sizeof(mesh->model_data[mesh->count].model_data));
    memcpy(
            mesh->model_data[mesh->count].normal_data, normal_transform.data,
            sizeof(mesh->model_data[mesh->count].normal_data));
    mesh->model_data[mesh->count].color = color;
    mesh->count += 1;

    return GFX_RESULT_SUCCESS;
}

gfx_result
truss_mesh_add_between_pts(
        jta_mesh* mesh, jta_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll, jta_vulkan_window_context* ctx)
{
    assert(ctx);
    assert(pt2.w == 1.0f);
    assert(pt1.w == 1.0f);
    vec4 d = vec4_sub(pt2, pt1);
    const f32 len2 = vec4_dot(d, d);
    const f32 len = sqrtf(len2);
    const f32 rotation_y = acosf(d.z / len);
    const f32 rotation_z = atan2f(d.y, d.x);

    //  Update bounding box
    jta_bounding_box_add_point(&mesh->bounding_box, pt1, radius);
    jta_bounding_box_add_point(&mesh->bounding_box, pt2, radius);

    //  Grouped all the matrix operations together in an effort to make them easier to optimize by the compiler

    // first deform the model
    mtx4 model = mtx4_enlarge(radius, radius, len);
    // roll the model
    mtx4 normal_transform = mtx4_rotation_z(roll);
    //  rotate about y-axis
    normal_transform = mtx4_multiply(mtx4_rotation_y(-rotation_y), normal_transform);
    //  rotate about z-axis again
    normal_transform = mtx4_multiply(mtx4_rotation_z(-rotation_z), normal_transform);
    model = mtx4_multiply(normal_transform, model);
    //  move the model to the point pt1
    model = mtx4_translate(model, (pt1));
    //  Take the sheer of the model into account for the normal transformation as well
    normal_transform = mtx4_multiply(normal_transform, mtx4_enlarge(1 / radius, 1 / radius, 1 / len));


    return mesh_add_new(mesh, model, normal_transform, color, ctx);
}

static gfx_result generate_sphere_model(jta_model* const p_out, const u16 order)
{
    JDM_ENTER_FUNCTION;
    assert(order >= 1);
    u16* indices;
    const u32 max_index_count =
            3 * ((1 << (2 * order)));  //  3 per triangle, increases by a factor of 4 for each level of refinement
    if (!(indices = ill_jalloc(G_JALLOCATOR, max_index_count * sizeof *indices)))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    jta_vertex* vertices;
    const u32 max_vertex_count = 3 * ((1 << (2 * order)));  //  This is an upper bound
    if (!(vertices = ill_jalloc(G_JALLOCATOR, max_vertex_count * sizeof *vertices)))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
        ill_jfree(G_JALLOCATOR, indices);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }


    typedef struct triangle_T triangle;
    struct triangle_T
    {
        vec4 p1, p2, p3;
    };
    triangle* triangle_list = lin_jalloc(G_LIN_JALLOCATOR, sizeof(triangle) * (1 << (order * 2)));
    if (!triangle_list)
    {
        JDM_ERROR("Could not allocate buffer for sphere triangle generation");
        ill_jfree(G_JALLOCATOR, vertices);
        ill_jfree(G_JALLOCATOR, indices);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    //  Initial mesh
    triangle_list[0] = (triangle)
            {
                    .p1 = { .x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 1.0f }, //  Top
                    .p2 = { .x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f /
                                                                                                      3.0f, .w = 1.0f }, //  Bottom left
                    .p3 = { .x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f /
                                                                                                      3.0f, .w = 1.0f }, //  Bottom right
            };
    triangle_list[1] = (triangle)
            {
                    .p1 = { .x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 1.0f }, //  Top
                    .p2 = { .x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f /
                                                                                                      3.0f, .w = 1.0f }, //  Bottom left
                    .p3 = { .x =-2 * sqrtf(2) / 3, .y = 0.0f, .z = -1.0f / 3.0f, .w = 1.0f }, //  Bottom right
            };
    triangle_list[2] = (triangle)
            {
                    .p1 = { .x = 0.0f, .y = 0.0f, .z = 1.0f, .w = 1.0f }, //  Top
                    .p2 = { .x =-2 * sqrtf(2) / 3, .y = 0.0f, .z = -1.0f / 3.0f, .w = 1.0f }, //  Bottom left
                    .p3 = { .x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f /
                                                                                                      3.0f, .w = 1.0f }, //  Bottom right
            };
    triangle_list[3] = (triangle)
            {
                    .p1 = { .x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f /
                                                                                                      3.0f, .w = 1.0f }, //  Bottom left
                    .p2 = { .x =-2 * sqrtf(2) / 3, .y = 0.0f, .z = -1.0f / 3.0f, .w = 1.0f }, //  Bottom left
                    .p3 = { .x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f /
                                                                                                      3.0f, .w = 1.0f }, //  Bottom left
            };
    u32 triangle_count = 4;

    //  Refine the mesh
    for (u32 i_order = 1; i_order < order; ++i_order)
    {
        //  Loop over every triangle and break it up into four smaller ones
        for (u32 i = 0; i < triangle_count; ++i)
        {
            const triangle base_triangle = triangle_list[i];
            triangle t1 = base_triangle;
            t1.p2 = vec4_div_one(vec4_add(t1.p1, t1.p2), 2);
            t1.p3 = vec4_div_one(vec4_add(t1.p1, t1.p3), 2);
            triangle t2 = base_triangle;
            t2.p3 = vec4_div_one(vec4_add(t2.p2, t2.p3), 2);
            t2.p1 = vec4_div_one(vec4_add(t2.p2, t2.p1), 2);
            triangle t3 = base_triangle;
            t3.p1 = vec4_div_one(vec4_add(t3.p3, t3.p1), 2);
            t3.p2 = vec4_div_one(vec4_add(t3.p3, t3.p2), 2);
            triangle t4 = { .p1 = t1.p2, .p2 = t2.p3, .p3 = t3.p1 };
            triangle_list[i] = t4;
            triangle_list[triangle_count + 3 * i + 0] = t1;
            triangle_list[triangle_count + 3 * i + 1] = t2;
            triangle_list[triangle_count + 3 * i + 2] = t3;
        }
        triangle_count *= 4;
    }

    //  Normalize the triangles (projecting them on to the unit sphere)
    for (u32 i = 0; i < triangle_count; ++i)
    {
        triangle_list[i].p1 = vec4_unit(triangle_list[i].p1);
        triangle_list[i].p2 = vec4_unit(triangle_list[i].p2);
        triangle_list[i].p3 = vec4_unit(triangle_list[i].p3);
    }

    //  Remove duplicated values
    u32 vertex_count = 0, index_count = 0;
    for (u32 i = 0; i < triangle_count; ++i)
    {
        const triangle t = triangle_list[i];
        u32 j;
        for (j = 0; j < vertex_count; ++j)
        {
            if (t.p1.x == vertices[j].x
                && t.p1.y == vertices[j].y
                && t.p1.z == vertices[j].z)
            {
                break;
            }
        }
        if (j == vertex_count)
        {
            vertices[vertex_count].x = (vertices[vertex_count].nx = t.p1.x);
            vertices[vertex_count].y = (vertices[vertex_count].ny = t.p1.y);
            vertices[vertex_count].z = (vertices[vertex_count].nz = t.p1.z);
            vertex_count += 1;
        }
        indices[index_count++] = j;

        for (j = 0; j < vertex_count; ++j)
        {
            if (t.p2.x == vertices[j].x
                && t.p2.y == vertices[j].y
                && t.p2.z == vertices[j].z)
            {
                break;
            }
        }
        if (j == vertex_count)
        {
            vertices[vertex_count].x = (vertices[vertex_count].nx = t.p2.x);
            vertices[vertex_count].y = (vertices[vertex_count].ny = t.p2.y);
            vertices[vertex_count].z = (vertices[vertex_count].nz = t.p2.z);
            vertex_count += 1;
        }
        indices[index_count++] = j;

        for (j = 0; j < vertex_count; ++j)
        {
            if (t.p3.x == vertices[j].x
                && t.p3.y == vertices[j].y
                && t.p3.z == vertices[j].z)
            {
                break;
            }
        }
        if (j == vertex_count)
        {
            vertices[vertex_count].x = (vertices[vertex_count].nx = t.p3.x);
            vertices[vertex_count].y = (vertices[vertex_count].ny = t.p3.y);
            vertices[vertex_count].z = (vertices[vertex_count].nz = t.p3.z);
            vertex_count += 1;
        }
        indices[index_count++] = j;
    }
    lin_jfree(G_LIN_JALLOCATOR, triangle_list);
    jta_vertex* const new_ptr = ill_jrealloc(G_JALLOCATOR, vertices, vertex_count * sizeof(*vertices));
    if (!new_ptr)
    {
        JDM_WARN("Could not shrink memory used by vertex list");
    }
    else
    {
        vertices = new_ptr;
    }

    p_out->vtx_count = vertex_count;
    p_out->idx_count = index_count;
    p_out->vtx_array = vertices;
    p_out->idx_array = indices;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result mesh_init_sphere(jta_mesh* mesh, u16 order, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;
    gfx_result gfx_res;

    mesh->name = "sphere";
    if ((gfx_res = generate_sphere_model(&mesh->model, order)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not generate a sphere model for a mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }
    mesh->count = 0;
    mesh->capacity = DEFAULT_MESH_CAPACITY;

    if (!(mesh->model_data = ill_jalloc(G_JALLOCATOR, mesh->capacity * sizeof(*mesh->model_data))))
    {
        JDM_ERROR("Could not allocate memory for mesh model array");
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    if ((gfx_res = mesh_allocate_vulkan_memory(ctx, mesh)) != GFX_RESULT_SUCCESS)
    {
        clean_mesh_model(&mesh->model);
        ill_jfree(G_JALLOCATOR, mesh->model_data);
        JDM_ERROR("Could not allocate vulkan memory for the sphere mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result sphere_mesh_add(jta_mesh* mesh, jta_color color, f32 radius, vec4 pt, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;
    jta_bounding_box_add_point(&mesh->bounding_box, pt, radius);
    mtx4 m = mtx4_enlarge(radius, radius, radius);
    m = mtx4_translate(m, pt);
    JDM_LEAVE_FUNCTION;
    return mesh_add_new(mesh, m, mtx4_identity, color, ctx);
}

gfx_result jta_mesh_update_instance(jta_mesh* mesh, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;

    jta_vulkan_memory_to_buffer(
            ctx, 0, sizeof(jta_model_data) * mesh->count, mesh->model_data, 0, mesh->instance_memory);
    mesh->up_to_date = 1;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result jta_mesh_update_model(jta_mesh* mesh, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;

    jta_vulkan_memory_to_buffer(
            ctx, 0, sizeof(jta_vertex) * mesh->model.vtx_count, mesh->model.vtx_array, 0, mesh->common_geometry_vtx);

    jta_vulkan_memory_to_buffer(
            ctx, 0, sizeof(jta_vertex) * mesh->model.vtx_count, mesh->model.vtx_array, 0, mesh->common_geometry_idx);

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

}

static gfx_result generate_cone_model(jta_model* const model, const u16 order)
{
    assert(order >= 3);
    jta_vertex* vertices;
    u32 vtx_count = 2 * order + 2;
    if (!(vertices = (ill_jalloc(G_JALLOCATOR, vtx_count * sizeof *vertices))))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        return GFX_RESULT_BAD_ALLOC;
    }
    jta_vertex* const top = vertices + 0;
    jta_vertex* const btm = vertices + order;
    u16* indices;
    u32 idx_count = 3 * 2 * order;
    if (!(indices = (ill_jalloc(G_JALLOCATOR, idx_count * sizeof *indices))))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        ill_jfree(G_JALLOCATOR, vertices);
        return GFX_RESULT_BAD_ALLOC;
    }


    const f32 d_omega = 2.0f * (f32) M_PI / (f32) order;
    //  Generate bottom side
    //  Top and bottom are at the end of the list
    vertices[2 * order + 0] = (jta_vertex) { .x = 0, .y = 0, .z = 1, .nx = 0, .ny = 0, .nz = 1 };
    vertices[2 * order + 1] = (jta_vertex) { .x = 0, .y = 0, .z = 0, .nx = 0, .ny = 0, .nz =-1 };
    for (u32 i = 0; i < order; ++i)
    {
        //    Positions
        top[i] = (jta_vertex) { .x = cosf(d_omega * (f32) i), .y = sinf(d_omega * (f32) i), .z = 0 };
        btm[i] = (jta_vertex) { .x = cosf(d_omega * (f32) i), .y = sinf(d_omega * (f32) i), .z = 0 };
        //    Normals
        top[i].nx = M_SQRT1_2 * top[i].x;
        top[i].ny = M_SQRT1_2 * top[i].y;
        top[i].nz = M_SQRT1_2;
        btm[i].nx = 0.0f;
        btm[i].ny = 0.0f;
        btm[i].nz = -1.0f;
        //  Top triangle connections
        indices[6 * i + 0] = i;
        if ((i32) i == order - 1)
        {
            indices[6 * i + 1] = 0;
        }
        else
        {
            indices[6 * i + 1] = i + 1;
        }
        indices[6 * i + 2] = 2 * order + 0;
        //  Bottom triangle connections
        indices[6 * i + 3] = i + order;
        indices[6 * i + 4] = 2 * order + 1;
        if ((i32) i == order - 1)
        {
            indices[6 * i + 5] = order;
        }
        else
        {
            indices[6 * i + 5] = i + order + 1;
        }
    }


    model->vtx_count = vtx_count;
    model->idx_count = idx_count;
    model->vtx_array = vertices;
    model->idx_array = indices;

    return GFX_RESULT_SUCCESS;
}

gfx_result mesh_init_cone(jta_mesh* mesh, u16 order, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;
    mesh->name = "cone";
    mesh->up_to_date = false;
    gfx_result gfx_res;
    if ((gfx_res = generate_cone_model(&mesh->model, order)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not generate a truss model for a mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }
    mesh->count = 0;
    mesh->capacity = DEFAULT_MESH_CAPACITY;

    if (!(mesh->model_data = ill_jalloc(G_JALLOCATOR, mesh->capacity * sizeof(*mesh->model_data))))
    {
        JDM_ERROR("Could not allocate memory for mesh model array");
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }


    if ((gfx_res = mesh_allocate_vulkan_memory(ctx, mesh)) != GFX_RESULT_SUCCESS)
    {
        clean_mesh_model(&mesh->model);
        ill_jfree(G_JALLOCATOR, mesh->model_data);
        JDM_ERROR("Could not allocate vulkan memory for the cone mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result cone_mesh_add_between_pts(
        jta_mesh* mesh, jta_color color, f32 radius, vec4 pt1, vec4 pt2, jta_vulkan_window_context* ctx)
{
    assert(ctx);
    assert(pt2.w == 1.0f);
    assert(pt1.w == 1.0f);
    vec4 d = vec4_sub(pt2, pt1);
    const f32 len2 = vec4_dot(d, d);
    const f32 len = sqrtf(len2);
    const f32 rotation_y = acosf(d.z / len);
    const f32 rotation_z = atan2f(d.y, d.x);


    jta_bounding_box_add_point(&mesh->bounding_box, pt1, radius);
    jta_bounding_box_add_point(&mesh->bounding_box, pt2, 0.0f);

    //  Grouped all the matrix operations together in an effort to make them easier to optimize by the compiler

    // first deform the model
    mtx4 model = mtx4_enlarge(radius, radius, len);
    //  rotate about y-axis
    mtx4 normal_transform = mtx4_rotation_y(-rotation_y);
    //  rotate about z-axis again
    normal_transform = mtx4_multiply(mtx4_rotation_z(-rotation_z), normal_transform);
    model = mtx4_multiply(normal_transform, model);
    //  move the model to the point pt1
    model = mtx4_translate(model, (pt1));
    //  Take the sheer of the model into account for the normal transformation as well
    normal_transform = mtx4_multiply(normal_transform, mtx4_enlarge(1 / radius, 1 / radius, 1 / len));

    return mesh_add_new(mesh, model, normal_transform, color, ctx);
}

static gfx_result add_force_arrow(
        jta_structure_meshes* const meshes, const vec4 pos, const vec4 direction, const float length,
        const float radius, jta_vulkan_window_context* ctx, jta_color color, float arrow_cone_ratio)
{
    gfx_result gfx_res;

    vec4 second = vec4_add(pos, vec4_mul_one(direction, length * (1 - arrow_cone_ratio)));
    vec4 third = vec4_add(pos, vec4_mul_one(direction, length));
    if ((
                gfx_res = cone_mesh_add_between_pts(
                        &meshes->cones, color, 2 * radius,
                        second, third, ctx)) != GFX_RESULT_SUCCESS)
    {
        return gfx_res;
    }
    if ((
                gfx_res = truss_mesh_add_between_pts(
                        &meshes->cylinders, color, radius,
                        pos, second, 0.0f, ctx)) != GFX_RESULT_SUCCESS)
    {
        return gfx_res;
    }

    return GFX_RESULT_SUCCESS;
}

static inline vec4 point_position(const jta_point_list* list, const uint32_t idx)
{
    assert(idx < list->count);
    return VEC4(list->p_x[idx], list->p_y[idx], list->p_z[idx]);
}

gfx_result jta_structure_meshes_generate_undeformed(
        jta_structure_meshes* meshes, const jta_config_display* cfg, const jta_problem_setup* problem_setup,
        jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;

    gfx_result res = GFX_RESULT_SUCCESS;
    void* const base = lin_jallocator_save_state(G_LIN_JALLOCATOR);

    //  Preprocess point positions
    const jta_point_list* point_list = &problem_setup->point_list;
    vec4* const positions = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*positions) * point_list->count);
    if (!positions)
    {
        JDM_ERROR("Could not allocate array to store preprocessed point positions in");
        res = GFX_RESULT_BAD_ALLOC;
        goto end;
    }
    for (uint32_t i_pt = 0; i_pt < point_list->count; ++i_pt)
    {
        positions[i_pt] = point_position(point_list, i_pt);
    }

    //  Generate truss members

    const jta_element_list* element_list = &problem_setup->element_list;
    for (uint32_t i_element = 0; i_element < element_list->count; ++i_element)
    {
        const uint32_t i_pt0 = element_list->i_point0[i_element];
        const uint32_t i_pt1 = element_list->i_point1[i_element];
        const uint32_t i_mat = element_list->i_material[i_element];
        const uint32_t i_pro = element_list->i_profile[i_element];

        const vec4 pt0 = positions[i_pt0];
        const vec4 pt1 = positions[i_pt1];

        (void) i_mat;
        const jta_color c = {{ .r = 0xD0, .g = 0xD0, .b = 0xD0, .a = 0xFF }};   //  Here a colormap lookup could be done

        const float radius = problem_setup->profile_list.equivalent_radius[i_pro] * cfg->radius_scale;

        if ((res = truss_mesh_add_between_pts(&meshes->cylinders, c, radius, pt0, pt1, 0, ctx)) !=
            GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add truss element number %u between \"%.*s\" and \"%.*s\", reason: %s", i_element + 1,
                      (int) point_list->label[i_pt0].len, point_list->label[i_pt0].begin,
                      (int) point_list->label[i_pt1].len, point_list->label[i_pt1].begin, gfx_result_to_str(res));
            goto end;
        }
    }

    uint_fast8_t* const bcs_per_point = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*bcs_per_point) * point_list->count);
    if (!bcs_per_point)
    {
        JDM_ERROR("Could not allocate buffer for counting point DoFs");
        res = GFX_RESULT_BAD_ALLOC;
        goto end;
    }

    //  Count point DoFs
    memset(bcs_per_point, 0, sizeof(*bcs_per_point) * point_list->count);
    const jta_numerical_boundary_condition_list* num_bcs = &problem_setup->numerical_bcs;
    for (uint32_t i_bc = 0; i_bc < num_bcs->count; ++i_bc)
    {
        const jta_numerical_boundary_condition_type type = num_bcs->type[i_bc];
        const uint32_t i_pt = num_bcs->i_point[i_bc];
        bcs_per_point[i_pt] += (type & JTA_NUMERICAL_BC_TYPE_X) != 0;
        bcs_per_point[i_pt] += (type & JTA_NUMERICAL_BC_TYPE_Y) != 0;
        bcs_per_point[i_pt] += (type & JTA_NUMERICAL_BC_TYPE_Z) != 0;
        if (bcs_per_point[i_pt] > 3)
        {
            JDM_ERROR("Point \"%.*s\" has too %u constraints", (int) point_list->label[i_pt].len,
                      point_list->label[i_pt].begin, bcs_per_point[i_pt]);
            res = GFX_RESULT_BAD_ARG;
            goto end;
        }
    }

    //  Generate the joints
    for (uint32_t i_pt = 0; i_pt < point_list->count; ++i_pt)
    {
        jta_color c = cfg->dof_point_colors[bcs_per_point[i_pt]];
        float r = cfg->dof_point_scales[bcs_per_point[i_pt]] * point_list->max_radius[i_pt] * cfg->radius_scale;
        vec4 pos = positions[i_pt];

        if ((res = sphere_mesh_add(&meshes->spheres, c, r, pos, ctx)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add point \"%.*s\", reason: %s", (int) point_list->label[i_pt].len,
                      point_list->label[i_pt].begin,
                      gfx_result_to_str(res));
            goto end;
        }
    }

    lin_jfree(G_LIN_JALLOCATOR, bcs_per_point);

    //  Generate force arrows for natural BCs

    const jta_natural_boundary_condition_list* nat_bcs = &problem_setup->natural_bcs;
    for (uint32_t i_bc = 0; i_bc < nat_bcs->count; ++i_bc)
    {
        const uint32_t i_pt = nat_bcs->i_point[i_bc];
        vec4 pos = positions[i_pt];
        vec4 force = VEC4(nat_bcs->x[i_bc],
                          nat_bcs->y[i_bc],
                          nat_bcs->z[i_bc]);
        float mag = vec4_magnitude(force);
        vec4 unit_dir = vec4_div_one(force, mag);

        float length = mag / nat_bcs->max_mag * element_list->max_len * cfg->force_length_ratio;
        float radius = cfg->force_radius_ratio * problem_setup->profile_list.max_equivalent_radius;

        jta_color c = {{ .r = 0x00, .g = 0x00, .b = 0xD0, .a = 0xFF }};   //  This could be a config option

        if ((res = add_force_arrow(meshes, pos, unit_dir, length, radius, ctx, c, cfg->force_head_ratio)) !=
            GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add the force %u at point \"%.*s\", reason: %s", i_bc + 1,
                      (int) point_list->label[i_pt].len, point_list->label[i_pt].begin, gfx_result_to_str(res));
            goto end;
        }
    }

    lin_jfree(G_LIN_JALLOCATOR, positions);
    end:
    lin_jallocator_restore_current(G_LIN_JALLOCATOR, base);
    JDM_LEAVE_FUNCTION;
    return res;
}

gfx_result jta_structure_meshes_generate_deformed(
        jta_structure_meshes* meshes, const jta_config_display* cfg, const jta_problem_setup* problem_setup,
        const jta_solution* solution, jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;

    gfx_result res = GFX_RESULT_SUCCESS;
    void* const base = lin_jallocator_save_state(G_LIN_JALLOCATOR);

    //  Preprocess point positions
    const jta_point_list* point_list = &problem_setup->point_list;
    vec4* const positions = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*positions) * point_list->count);
    if (!positions)
    {
        JDM_ERROR("Could not allocate array to store preprocessed point positions in");
        res = GFX_RESULT_BAD_ALLOC;
        goto end;
    }
    for (uint32_t i_pt = 0; i_pt < point_list->count; ++i_pt)
    {
        //  Add point displacements
        positions[i_pt] = VEC4(
                point_list->p_x[i_pt] + cfg->deformation_scale * solution->point_displacements[3 * i_pt + 0],
                point_list->p_y[i_pt] + cfg->deformation_scale * solution->point_displacements[3 * i_pt + 1],
                point_list->p_z[i_pt] + cfg->deformation_scale * solution->point_displacements[3 * i_pt + 2]);
    }

    //  Generate truss members

    const jta_element_list* element_list = &problem_setup->element_list;
    for (uint32_t i_element = 0; i_element < element_list->count; ++i_element)
    {
        const uint32_t i_pt0 = element_list->i_point0[i_element];
        const uint32_t i_pt1 = element_list->i_point1[i_element];
        const uint32_t i_pro = element_list->i_profile[i_element];

        const vec4 pt0 = positions[i_pt0];
        const vec4 pt1 = positions[i_pt1];

        const jta_color c = {{ .r = 0xFF, .g = 0x00, .b = 0x00, .a = 0x80 }};   //  Here a colormap lookup could be done

        const float radius = problem_setup->profile_list.equivalent_radius[i_pro] * cfg->radius_scale;

        if ((res = truss_mesh_add_between_pts(&meshes->cylinders, c, radius, pt0, pt1, 0, ctx)) !=
            GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add truss element number %u between \"%.*s\" and \"%.*s\", reason: %s", i_element + 1,
                      (int) point_list->label[i_pt0].len, point_list->label[i_pt0].begin,
                      (int) point_list->label[i_pt1].len, point_list->label[i_pt1].begin, gfx_result_to_str(res));
            goto end;
        }
    }

    jta_numerical_boundary_condition_type* const bcs_per_point = lin_jalloc(
            G_LIN_JALLOCATOR, sizeof(*bcs_per_point) * point_list->count);
    if (!bcs_per_point)
    {
        JDM_ERROR("Could not allocate buffer for counting point DoFs");
        res = GFX_RESULT_BAD_ALLOC;
        goto end;
    }

    //  Count point DoFs
    memset(bcs_per_point, 0, sizeof(*bcs_per_point) * point_list->count);
    const jta_numerical_boundary_condition_list* num_bcs = &problem_setup->numerical_bcs;
    for (uint32_t i_bc = 0; i_bc < num_bcs->count; ++i_bc)
    {
        const jta_numerical_boundary_condition_type type = num_bcs->type[i_bc];
        const uint32_t i_pt = num_bcs->i_point[i_bc];
        if ((type & JTA_NUMERICAL_BC_TYPE_X) && (bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_X))
        {
            JDM_ERROR("Point \"%.*s\" has two constraints for it's X position", (int) point_list->label[i_pt].len,
                      point_list->label[i_pt].begin);
            res = GFX_RESULT_BAD_ARG;
            goto end;
        }
        bcs_per_point[i_pt] += (type & JTA_NUMERICAL_BC_TYPE_X);

        if ((type & JTA_NUMERICAL_BC_TYPE_Y) && (bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_Y))
        {
            JDM_ERROR("Point \"%.*s\" has two constraints for it's Y position", (int) point_list->label[i_pt].len,
                      point_list->label[i_pt].begin);
            res = GFX_RESULT_BAD_ARG;
            goto end;
        }
        bcs_per_point[i_pt] += (type & JTA_NUMERICAL_BC_TYPE_Y);
        if ((type & JTA_NUMERICAL_BC_TYPE_Z) && (bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_Z))
        {
            JDM_ERROR("Point \"%.*s\" has two constraints for it's X position", (int) point_list->label[i_pt].len,
                      point_list->label[i_pt].begin);
            res = GFX_RESULT_BAD_ARG;
            goto end;
        }
        bcs_per_point[i_pt] += (type & JTA_NUMERICAL_BC_TYPE_Z);
    }

    //  Generate the joints
    for (uint32_t i_pt = 0; i_pt < point_list->count; ++i_pt)
    {
        const uint32_t dof_count = ((bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_X) != 0) +
                                   ((bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_Y) != 0) +
                                   ((bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_Z) != 0);
        jta_color c = cfg->dof_point_colors[dof_count];
        float r = cfg->dof_point_scales[dof_count] * point_list->max_radius[i_pt] * cfg->radius_scale;
        vec4 pos = positions[i_pt];

        if ((res = sphere_mesh_add(&meshes->spheres, c, r, pos, ctx)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add point \"%.*s\", reason: %s", (int) point_list->label[i_pt].len,
                      point_list->label[i_pt].begin,
                      gfx_result_to_str(res));
            goto end;
        }
    }

    //  Generate force arrows for reaction forces


    float* const reaction_magnitudes = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*reaction_magnitudes) * point_list->count);
    if (!reaction_magnitudes)
    {
        JDM_ERROR("Could not allocate memory for reaction forces");
        res = GFX_RESULT_BAD_ALLOC;
        goto end;
    }

    //  Find both the largest single magnitude, along with the total solution magnitude
    float maximum_mag = 0;
    for (uint32_t i = 0; i < point_list->count; ++i)
    {
        float h = 0;
        h += solution->point_reactions[3 * i + 0] * solution->point_reactions[3 * i + 0];
        h += solution->point_reactions[3 * i + 1] * solution->point_reactions[3 * i + 1];
        h += solution->point_reactions[3 * i + 2] * solution->point_reactions[3 * i + 2];
        reaction_magnitudes[i] = sqrtf(h);
        if (reaction_magnitudes[i] > maximum_mag)
        {
            maximum_mag = reaction_magnitudes[i];
        }
    }

    for (uint32_t i_pt = 0; i_pt < point_list->count; ++i_pt)
    {
        //  Check if it should be skipped (reactions only occur at BCs
        if (bcs_per_point[i_pt] == 0)
        {
            continue;
        }

        vec4 pos = positions[i_pt];
        vec4 force = VEC4(0, 0, 0);
        if ((bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_X))
        {
            force.x = solution->point_reactions[3 * i_pt + 0];
        }
        if ((bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_Y))
        {
            force.y = solution->point_reactions[3 * i_pt + 1];
        }
        if ((bcs_per_point[i_pt] & JTA_NUMERICAL_BC_TYPE_Z))
        {
            force.z = solution->point_reactions[3 * i_pt + 2];
        }
        float mag = vec4_magnitude(force);
        vec4 unit_dir = vec4_div_one(force, mag);

        float length = mag / maximum_mag * element_list->max_len * cfg->force_length_ratio;
        float radius = cfg->force_radius_ratio * problem_setup->profile_list.max_equivalent_radius;

        jta_color c = {{ .r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF }};   //  This could be a config option

        if ((res = add_force_arrow(meshes, pos, unit_dir, length, radius, ctx, c, cfg->force_head_ratio)) !=
            GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add the reaction force at point \"%.*s\", reason: %s",
                      (int) point_list->label[i_pt].len, point_list->label[i_pt].begin, gfx_result_to_str(res));
            goto end;
        }
    }

    lin_jfree(G_LIN_JALLOCATOR, reaction_magnitudes);
    lin_jfree(G_LIN_JALLOCATOR, bcs_per_point);
    lin_jfree(G_LIN_JALLOCATOR, positions);
    end:
    lin_jallocator_restore_current(G_LIN_JALLOCATOR, base);
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_structure_meshes_destroy(jta_vulkan_window_context* ctx, jta_structure_meshes* meshes)
{
    JDM_ENTER_FUNCTION;

    mesh_destroy(ctx, &meshes->cylinders);
    mesh_destroy(ctx, &meshes->cones);
    mesh_destroy(ctx, &meshes->spheres);

    JDM_LEAVE_FUNCTION;
}

