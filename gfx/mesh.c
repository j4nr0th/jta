//
// Created by jan on 6.7.2023.
//

#include "mesh.h"
#include "vk_state.h"
#include <jdm.h>

static gfx_result mesh_allocate_vulkan_memory(vk_state* state, jta_mesh* mesh, jfw_window_vk_resources* resources)
{
    vk_buffer_allocation vtx_allocation, idx_allocation, mod_allocation;
    const jta_model* const model = &mesh->model;
    i32 res = vk_buffer_allocate(state->buffer_allocator, 1, model->vtx_count * sizeof(*model->vtx_array), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_allocation);
    if (res < 0)
    {
        JDM_ERROR("Could not allocate buffer memory for mesh vertex buffer");
        return GFX_RESULT_BAD_ALLOC;
    }
    gfx_result gfx_res;
    if ((gfx_res = vk_transfer_memory_to_buffer(resources, state, &vtx_allocation, sizeof(jta_vertex) * mesh->model.vtx_count, mesh->model.vtx_array)) != GFX_RESULT_SUCCESS)
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

static gfx_result clean_mesh_model(jta_model* model)
{
    ill_jfree(G_JALLOCATOR, model->vtx_array);
    ill_jfree(G_JALLOCATOR, model->idx_array);
    memset(model, 0, sizeof*model);
    return GFX_RESULT_SUCCESS;
}

static gfx_result generate_truss_model(jta_model* const p_out, const u16 pts_per_side)
{
    jta_vertex* vertices;
    if (!(vertices = ill_jalloc(G_JALLOCATOR,2 * pts_per_side * sizeof*vertices)))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        return GFX_RESULT_BAD_ALLOC;
    }
    jta_vertex* const btm = vertices;
    jta_vertex* const top = vertices + pts_per_side;
    u16* indices;
    if (!(indices = ill_jalloc(G_JALLOCATOR, 3 * 2 * pts_per_side * sizeof*indices)))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        ill_jfree(G_JALLOCATOR, vertices);
        return GFX_RESULT_BAD_ALLOC;
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

gfx_result mesh_init_truss(jta_mesh* mesh, u16 pts_per_side, vk_state* state, jfw_window_vk_resources* resources)
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

    if (!(mesh->model_data = ill_jalloc(G_JALLOCATOR,mesh->capacity * sizeof(*mesh->model_data))))
    {
        JDM_ERROR("Could not allocate memory for mesh model array");
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }


    if ((gfx_res = mesh_allocate_vulkan_memory(state, mesh, resources)) != GFX_RESULT_SUCCESS)
    {
        clean_mesh_model(&mesh->model);
        ill_jfree(G_JALLOCATOR, mesh->model_data);
        JDM_ERROR("Could not allocate vulkan memory for the truss mesh");
        JDM_LEAVE_FUNCTION;
        return gfx_res;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result mesh_uninit(jta_mesh* mesh)
{
    ill_jfree(G_JALLOCATOR, mesh->model_data);
    clean_mesh_model(&mesh->model);
    return GFX_RESULT_SUCCESS;
}

static gfx_result mesh_add_new(jta_mesh* mesh, mtx4 model_transform, mtx4 normal_transform, jfw_color color, vk_state* state)
{
    mesh->up_to_date = 0;

    if (mesh->count == mesh->capacity)
    {
        //  Reallocate memory for data on host side
        const u32 new_capacity = mesh->capacity + 64;
        jta_model_data* const new_ptr = ill_jrealloc(G_JALLOCATOR, mesh->model_data, new_capacity * sizeof(*mesh->model_data));
        if (!new_ptr)
        {
            JDM_ERROR("Could not reallocate memory for mesh color array");
            return GFX_RESULT_BAD_ALLOC;
        }
        mesh->model_data = new_ptr;
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
truss_mesh_add_between_pts(jta_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll, vk_state* state)
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

static gfx_result generate_sphere_model(jta_model* const p_out, const u16 order)
{
    JDM_ENTER_FUNCTION;
    assert(order >= 1);
    u16* indices;
    const u32 max_index_count = 3 * ((1 << (2 * order)));  //  3 per triangle, increases by a factor of 4 for each level of refinement
    if (!(indices = ill_jalloc(G_JALLOCATOR, max_index_count * sizeof*indices)))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    jta_vertex* vertices;
    const u32 max_vertex_count = 3 * ((1 << (2 * order)));  //  This is an upper bound
    if (!(vertices = ill_jalloc(G_JALLOCATOR, max_vertex_count * sizeof*vertices)))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
        ill_jfree(G_JALLOCATOR, indices);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }


    typedef struct triangle_struct triangle;
    struct triangle_struct
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
                    .p1 = {.x = 0.0f,                    .y = 0.0f,                             .z = 1.0f      , .w = 1.0f}, //  Top
                    .p2 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom left
                    .p3 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom right
            };
    triangle_list[1] = (triangle)
            {
                    .p1 = {.x = 0.0f,                    .y = 0.0f,                             .z = 1.0f      , .w = 1.0f}, //  Top
                    .p2 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom left
                    .p3 = {.x =-2 * sqrtf(2) / 3,        .y = 0.0f,                             .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom right
            };
    triangle_list[2] = (triangle)
            {
                    .p1 = {.x = 0.0f,                    .y = 0.0f,                             .z = 1.0f      , .w = 1.0f}, //  Top
                    .p2 = {.x =-2 * sqrtf(2) / 3,        .y = 0.0f,                             .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom left
                    .p3 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom right
            };
    triangle_list[3] = (triangle)
            {
                    .p1 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * -sqrtf(3) / 2, .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom left
                    .p2 = {.x =-2 * sqrtf(2) / 3,        .y = 0.0f,                             .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom left
                    .p3 = {.x = 2 * sqrtf(2) / 3 * 0.5f, .y = 2 * sqrtf(2) / 3 * +sqrtf(3) / 2, .z = -1.0f/3.0f, .w = 1.0f}, //  Bottom left
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
            triangle t4 = {.p1 = t1.p2, .p2 = t2.p3, .p3 = t3.p1};
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

gfx_result mesh_init_sphere(jta_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources)
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

    if ((gfx_res = mesh_allocate_vulkan_memory(state, mesh, resources)) != GFX_RESULT_SUCCESS)
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

gfx_result sphere_mesh_add(jta_mesh* mesh, jfw_color color, f32 radius, vec4 pt, vk_state* state)
{
    JDM_ENTER_FUNCTION;
    mtx4 m = mtx4_enlarge(radius, radius, radius);
    m = mtx4_translate(m, pt);
    JDM_LEAVE_FUNCTION;
    return mesh_add_new(mesh, m, mtx4_identity, color, state);
}

gfx_result jta_mesh_update_instance(jta_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state)
{
    JDM_ENTER_FUNCTION;

    vk_transfer_memory_to_buffer(resources, state, &mesh->instance_memory, sizeof(jta_model_data) * mesh->count, mesh->model_data);
    mesh->up_to_date = 1;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result jta_mesh_update_model(jta_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state)
{
    JDM_ENTER_FUNCTION;

    vk_transfer_memory_to_buffer(resources, state, &mesh->common_geometry_vtx, sizeof(jta_vertex) *
                                                                               mesh->model.vtx_count,
                                                                               mesh->model.vtx_array);

    vk_transfer_memory_to_buffer(resources, state, &mesh->common_geometry_idx, sizeof(jta_vertex) *
                                                                               mesh->model.vtx_count,
                                                                               mesh->model.vtx_array);

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

}

static gfx_result generate_cone_model(jta_model* const model, const u16 order)
{
    assert(order >= 3);
    jta_vertex* vertices;
    u32 vtx_count = 2 * order + 2;
    if (!(vertices = (ill_jalloc(G_JALLOCATOR, vtx_count * sizeof*vertices))))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        return GFX_RESULT_BAD_ALLOC;
    }
    jta_vertex*const  top = vertices + 0;
    jta_vertex*const  btm = vertices + order;
    u16* indices;
    u32 idx_count = 3 * 2 * order;
    if (!(indices = (ill_jalloc(G_JALLOCATOR, idx_count * sizeof*indices))))
    {
        JDM_ERROR("Could not allocate memory for truss model");
        ill_jfree(G_JALLOCATOR, vertices);
        return GFX_RESULT_BAD_ALLOC;
    }


    const f32 d_omega = 2.0f * (f32)M_PI / (f32)order;
    //  Generate bottom side
    //  Top and bottom are at the end of the list
    vertices[2 * order + 0] = (jta_vertex) {.x = 0, .y = 0, .z = 1, .nx = 0, .ny = 0, .nz = 1};
    vertices[2 * order + 1] = (jta_vertex) {.x = 0, .y = 0, .z = 0, .nx = 0, .ny = 0, .nz =-1};
    for (u32 i = 0; i < order; ++i)
    {
        //    Positions
        top[i] = (jta_vertex) {.x = cosf(d_omega * (f32)i), .y = sinf(d_omega * (f32)i), .z = 0};
        btm[i] = (jta_vertex) {.x = cosf(d_omega * (f32)i), .y = sinf(d_omega * (f32)i), .z = 0};
        //    Normals
        top[i].nx = M_SQRT1_2 * top[i].x;
        top[i].ny = M_SQRT1_2 * top[i].y;
        top[i].nz = M_SQRT1_2;
        btm[i].nx = 0.0f;
        btm[i].ny = 0.0f;
        btm[i].nz = -1.0f;
        //  Top triangle connections
        indices[6 * i + 0] = i;
        if ((i32)i == order - 1)
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
        if ((i32)i == order - 1)
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

gfx_result mesh_init_cone(jta_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources)
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

    if (!(mesh->model_data = ill_jalloc(G_JALLOCATOR,mesh->capacity * sizeof(*mesh->model_data))))
    {
        JDM_ERROR("Could not allocate memory for mesh model array");
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }


    if ((gfx_res = mesh_allocate_vulkan_memory(state, mesh, resources)) != GFX_RESULT_SUCCESS)
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

gfx_result cone_mesh_add_between_pts(jta_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, vk_state* state)
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
    //  rotate about y-axis
    mtx4 normal_transform = mtx4_rotation_y(-rotation_y);
    //  rotate about z-axis again
    normal_transform = mtx4_multiply(mtx4_rotation_z(-rotation_z), normal_transform);
    model = mtx4_multiply(normal_transform, model);
    //  move the model to the point pt1
    model = mtx4_translate(model, (pt1));
    //  Take the sheer of the model into account for the normal transformation as well
    normal_transform = mtx4_multiply(normal_transform, mtx4_enlarge(1/radius, 1/radius, 1/len));

    return mesh_add_new(mesh, model, normal_transform, color, state);
}

