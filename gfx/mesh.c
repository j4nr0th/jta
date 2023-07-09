//
// Created by jan on 6.7.2023.
//

#include "mesh.h"
#include "vk_state.h"
#include <jdm.h>

static gfx_result mesh_allocate_vulkan_memory(vk_state* state, jtb_mesh* mesh, jfw_window_vk_resources* resources)
{
    vk_buffer_allocation vtx_allocation, idx_allocation, mod_allocation;
    const jtb_model* const model = &mesh->model;
    i32 res = vk_buffer_allocate(state->buffer_allocator, 1, model->vtx_count * sizeof(*model->vtx_array), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_allocation);
    if (res < 0)
    {
        JDM_ERROR("Could not allocate buffer memory for mesh vertex buffer");
        return GFX_RESULT_BAD_ALLOC;
    }
    gfx_result gfx_res;
    if ((gfx_res = vk_transfer_memory_to_buffer(resources, state, &vtx_allocation, sizeof(jtb_vertex) * mesh->model.vtx_count, mesh->model.vtx_array)) != GFX_RESULT_SUCCESS)
    {
        vk_buffer_deallocate(state->buffer_allocator, &vtx_allocation);
        JDM_ERROR("Could not transfer model vertex data, reason: %s", gfx_result_to_str(gfx_res));
        return gfx_res;
    }
    res = vk_buffer_allocate(state->buffer_allocator, 1, model->idx_count * sizeof(*model->idx_array), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &idx_allocation);
    if (res < 0)
    {
        vk_buffer_deallocate(state->buffer_allocator, &vtx_allocation);
        JDM_ERROR("Could not allocate buffer memory for mesh vertex buffer");
        return GFX_RESULT_BAD_ALLOC;
    }
    if ((gfx_res = vk_transfer_memory_to_buffer(resources, state, &idx_allocation, model->idx_count * sizeof(*model->idx_array), model->idx_array)) != GFX_RESULT_SUCCESS)
    {
        vk_buffer_deallocate(state->buffer_allocator, &idx_allocation);
        vk_buffer_deallocate(state->buffer_allocator, &vtx_allocation);
        JDM_ERROR("Could not transfer model index data, reason: %s", gfx_result_to_str(gfx_res));
        return gfx_res;
    }
    res = vk_buffer_allocate(state->buffer_allocator, 1, mesh->capacity * sizeof(*mesh->model_data), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &mod_allocation);
    if (res < 0)
    {
        vk_buffer_deallocate(state->buffer_allocator, &vtx_allocation);
        vk_buffer_deallocate(state->buffer_allocator, &idx_allocation);
        JDM_ERROR("Could not allocate buffer memory for model data buffer");
        return GFX_RESULT_BAD_ALLOC;
    }
    mesh->instance_memory = mod_allocation;
    mesh->common_geometry_idx = idx_allocation;
    mesh->common_geometry_vtx = vtx_allocation;
    return GFX_RESULT_SUCCESS;
}

static gfx_result clean_mesh_model(jtb_model* model)
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

gfx_result mesh_init_truss(jtb_mesh* mesh, u16 pts_per_side, vk_state* state, jfw_window_vk_resources* resources)
{
    JDM_ENTER_FUNCTION;
    jfw_res res;
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

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->model_data), &mesh->model_data)))
    {
        JDM_ERROR("Could not allocate memory for mesh model array, reason: %s", jfw_error_message(res));
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }


    if ((gfx_res = mesh_allocate_vulkan_memory(state, mesh, resources)) != GFX_RESULT_SUCCESS)
    {
        clean_mesh_model(&mesh->model);
        jfw_free(&mesh->model_data);
        JDM_ERROR("Could not allocate vulkan memory for the truss mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result mesh_uninit(jtb_mesh* mesh)
{
    jfw_free(&mesh->model_data);
    clean_mesh_model(&mesh->model);
    return GFX_RESULT_SUCCESS;
}

static gfx_result mesh_add_new(jtb_mesh* mesh, mtx4 model_transform, mtx4 normal_transform, jfw_color color, vk_state* state)
{
    mesh->up_to_date = 0;

    if (mesh->count == mesh->capacity)
    {
        //  Reallocate memory for data on host side
        const u32 new_capacity = mesh->capacity + 64;
        if (!jfw_success(jfw_realloc(new_capacity * sizeof(*mesh->model_data), &mesh->model_data)))
        {
            JDM_ERROR("Could not reallocate memory for mesh color array");
            return GFX_RESULT_BAD_ALLOC;
        }
        //  Reallocate memory on the GPU
        vk_buffer_allocation new_alloc;
        vk_buffer_deallocate(state->buffer_allocator, &mesh->instance_memory);
        i32 res = vk_buffer_allocate(state->buffer_allocator, 1, sizeof(*mesh->model_data) * new_capacity, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &new_alloc);
        if (res < 0)
        {
            JDM_ERROR("Could not reallocate vulkan memory for the instance data");
            res = vk_buffer_allocate(state->buffer_allocator, 1, sizeof(*mesh->model_data) * mesh->capacity, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &new_alloc);
            if (res < 0)
            {
                JDM_FATAL("Could not restore the mesh memory!");
            }
        }
        mesh->instance_memory = new_alloc;

        mesh->capacity = new_capacity;
    }

    memcpy(mesh->model_data[mesh->count].model_data, model_transform.data, sizeof(mesh->model_data[mesh->count].model_data));
    memcpy(mesh->model_data[mesh->count].normal_data, normal_transform.data, sizeof(mesh->model_data[mesh->count].normal_data));
    mesh->model_data[mesh->count].color = color;
    mesh->count += 1;

    return GFX_RESULT_SUCCESS;
}

gfx_result
truss_mesh_add_between_pts(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll, vk_state* state)
{
    assert(state);
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
    //  rotate about y-axis
    normal_transform = mtx4_multiply(mtx4_rotation_y(-rotation_y), normal_transform);
    //  rotate about z-axis again
    normal_transform = mtx4_multiply(mtx4_rotation_z(-rotation_z), normal_transform);
    model = mtx4_multiply(normal_transform, model);
    //  move the model to the point pt1
    model = mtx4_translate(model, (pt1));
    //  Take the sheer of the model into account for the normal transformation as well
    normal_transform = mtx4_multiply(normal_transform, mtx4_enlarge(1/radius, 1/radius, 1/len));

    return mesh_add_new(mesh, model, normal_transform, color, state);
}


static jfw_res clean_sphere_model(jtb_model* model)
{
    jfw_free(&model->vtx_array);
    jfw_free(&model->idx_array);
    memset(model, 0, sizeof*model);
    return jfw_res_success;
}
//  TODO: fix this one >:(
static jfw_res generate_sphere_model(jtb_model* const p_out, const u16 order)
{
    JDM_ENTER_FUNCTION;
    assert(order >= 1);
    jfw_res res;
    jtb_vertex* vertices;
    const u32 vertex_count = 3 * ((1 << (2 * order)));  //  This is an upper bound
    if (!jfw_success(res = (jfw_calloc(vertex_count, sizeof*vertices, &vertices))))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
        JDM_LEAVE_FUNCTION;
        return res;
    }

    u16* indices;
    const u32 index_count = 3 * ((1 << (2 * order)));  //  3 per triangle, increases by a factor of 4 for each level of refinement
    if (!jfw_success(res = (jfw_calloc(index_count, sizeof*indices, &indices))))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
        jfw_free(&vertices);
        JDM_LEAVE_FUNCTION;
        return res;
    }

    typedef struct point_struct point;
    struct point_struct
    {
        f32 x, y, z;
    };
    typedef struct triangle_struct triangle;
    struct triangle_struct
    {
        point p1, p2, p3;
    };
    triangle* triangle_list = lin_jalloc(G_LIN_JALLOCATOR, sizeof(triangle) * (1 << (order * 2)));
    //  Initial mesh
    triangle_list[0] = (triangle)
            {
                    .p1 = {.x = 0.0f,                    .y = 0.0f,                             .z = 1.0f},       //  Top
                    .p2 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f/3.0f}, //  Bottom left
                    .p3 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f/3.0f}, //  Bottom right
            };
    triangle_list[1] = (triangle)
            {
                    .p1 = {.x = 0.0f,                    .y = 0.0f,                             .z = 1.0f},       //  Top
                    .p2 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f/3.0f}, //  Bottom left
                    .p3 = {.x =-2 * sqrtf(2) / 3,        .y = 0.0f,                             .z = -1.0f/3.0f}, //  Bottom right
            };
    triangle_list[2] = (triangle)
            {
                    .p1 = {.x = 0.0f,                    .y = 0.0f,                             .z = 1.0f},       //  Top
                    .p2 = {.x =-2 * sqrtf(2) / 3,        .y = 0.0f,                             .z = -1.0f/3.0f}, //  Bottom left
                    .p3 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f/3.0f}, //  Bottom right
            };
    triangle_list[3] = (triangle)
            {
                    .p1 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f/3.0f}, //  Bottom left
                    .p2 = {.x =-2 * sqrtf(2) / 3,        .y = 0.0f,                             .z = -1.0f/3.0f}, //  Bottom left
                    .p3 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f/3.0f}, //  Bottom left
            };
    u32 triangle_count = 4;

    for (u32 i = 0; i < triangle_count; ++i)
    {
        vertices[3 * i + 0].x = triangle_list[i].p1.x;
        vertices[3 * i + 0].y = triangle_list[i].p1.y;
        vertices[3 * i + 0].z = triangle_list[i].p1.z;

        vertices[3 * i + 0].nx = triangle_list[i].p1.x;
        vertices[3 * i + 0].ny = triangle_list[i].p1.y;
        vertices[3 * i + 0].nz = triangle_list[i].p1.z;


        vertices[3 * i + 1].x = triangle_list[i].p2.x;
        vertices[3 * i + 1].y = triangle_list[i].p2.y;
        vertices[3 * i + 1].z = triangle_list[i].p2.z;

        vertices[3 * i + 1].nx = triangle_list[i].p2.x;
        vertices[3 * i + 1].ny = triangle_list[i].p2.y;
        vertices[3 * i + 1].nz = triangle_list[i].p2.z;


        vertices[3 * i + 2].x = triangle_list[i].p3.x;
        vertices[3 * i + 2].y = triangle_list[i].p3.y;
        vertices[3 * i + 2].z = triangle_list[i].p3.z;

        vertices[3 * i + 2].nx = triangle_list[i].p3.x;
        vertices[3 * i + 2].ny = triangle_list[i].p3.y;
        vertices[3 * i + 2].nz = triangle_list[i].p3.z;


        indices[3 * i + 0] = 3 * i + 0;
        indices[3 * i + 1] = 3 * i + 1;
        indices[3 * i + 2] = 3 * i + 2;
    }
    p_out->vtx_count = 12;
    p_out->idx_count = 12;
    lin_jfree(G_LIN_JALLOCATOR, triangle_list);

//    p_out->vtx_count = vertex_count;
//    p_out->idx_count = index_count;
    p_out->vtx_array = vertices;
    p_out->idx_array = indices;

    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

gfx_result mesh_init_sphere(jtb_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources)
{
    JDM_ENTER_FUNCTION;
    jfw_res res;
    mesh->name = "sphere";
    if (!jfw_success(generate_sphere_model(&mesh->model, order)))
    {
        JDM_ERROR("Could not generate a sphere model for a mesh");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    mesh->count = 0;
    mesh->capacity = DEFAULT_MESH_CAPACITY;

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->model_data), &mesh->model_data)))
    {
        JDM_ERROR("Could not allocate memory for mesh model array, reason: %s", jfw_error_message(res));
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    gfx_result gfx_res;
    if ((gfx_res = mesh_allocate_vulkan_memory(state, mesh, resources)) != GFX_RESULT_SUCCESS)
    {
        clean_mesh_model(&mesh->model);
        jfw_free(&mesh->model_data);
        JDM_ERROR("Could not allocate vulkan memory for the sphere mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result sphere_mesh_add(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt, vk_state* state)
{
    JDM_ENTER_FUNCTION;
    mtx4 m = mtx4_enlarge(radius, radius, radius);
    m = mtx4_translate(m, pt);
    JDM_LEAVE_FUNCTION;
    return mesh_add_new(mesh, m, mtx4_identity, color, state);
}

gfx_result jtb_mesh_update_instance(jtb_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state)
{
    JDM_ENTER_FUNCTION;

    vk_transfer_memory_to_buffer(resources, state, &mesh->instance_memory, sizeof(jtb_model_data) * mesh->count, mesh->model_data);
    mesh->up_to_date = 1;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result jtb_mesh_update_model(jtb_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state)
{
    JDM_ENTER_FUNCTION;

    vk_transfer_memory_to_buffer(resources, state, &mesh->common_geometry_vtx, sizeof(jtb_vertex) *
                                                                               mesh->model.vtx_count,
                                                                               mesh->model.vtx_array);

    vk_transfer_memory_to_buffer(resources, state, &mesh->common_geometry_idx, sizeof(jtb_vertex) *
                                                                               mesh->model.vtx_count,
                                                                               mesh->model.vtx_array);

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

}

