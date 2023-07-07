//
// Created by jan on 6.7.2023.
//

#include "mesh.h"
#include <jdm.h>


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

gfx_result mesh_init_truss(jtb_mesh* mesh, u16 pts_per_side)
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
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->model_matrices), &mesh->model_matrices)))
    {
        JDM_ERROR("Could not allocate memory for mesh model matrix array, reason: %s", jfw_error_message(res));
        jfw_free(&mesh->colors);
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    if (!jfw_success(res = jfw_calloc(mesh->capacity, sizeof(*mesh->normal_matrices), &mesh->normal_matrices)))
    {
        JDM_ERROR("Could not allocate memory for mesh normal matrix array, reason: %s", jfw_error_message(res));
        jfw_free(&mesh->colors);
        jfw_free(&mesh->model_matrices);
        clean_mesh_model(&mesh->model);
        return GFX_RESULT_BAD_ALLOC;
    }

    return GFX_RESULT_SUCCESS;
}

gfx_result mesh_uninit(jtb_mesh* mesh)
{
    jfw_free(&mesh->model_matrices);
    jfw_free(&mesh->colors);
    clean_mesh_model(&mesh->model);
    return GFX_RESULT_SUCCESS;
}

static gfx_result truss_mesh_add_new(jtb_mesh* mesh, mtx4 model_transform, mtx4 normal_transform, jfw_color color)
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

gfx_result truss_mesh_add_between_pts(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll)
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
        JDM_ERROR("Could not allocate memory for sphere model");
        return res;
    }

    u16* indices;
    const u32 index_count = 3 * 2 * nw * (nh + 1);
    if (!jfw_success(res = (jfw_calloc(index_count, sizeof*indices, &indices))))
    {
        JDM_ERROR("Could not allocate memory for sphere model");
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

gfx_result mesh_init_sphere(jtb_mesh* mesh, u16 nw, u16 nh)
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

static gfx_result sphere_mesh_add_new(jtb_mesh* mesh, mtx4 model_transform, jfw_color color)
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

gfx_result sphere_mesh_add(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt)
{
    mtx4 m = mtx4_enlarge(radius, radius, radius);
    m = mtx4_translate(m, pt);
    return sphere_mesh_add_new(mesh, m, color);
}

