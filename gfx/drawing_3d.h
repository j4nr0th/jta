//
// Created by jan on 18.5.2023.
//

#ifndef JTB_DRAWING_3D_H
#define JTB_DRAWING_3D_H
#include "../mem/aligned_jalloc.h"
#include <jmem/jmem.h>

#include "../jfw/window.h"
//#include "../jfw/error_system/error_codes.h"


#include "vk_state.h"
#include "camera.h"
#include "gfxerr.h"

typedef struct jtb_vertex_struct jtb_vertex;
struct jtb_vertex_struct
{
    f32 x, y, z;
    f32 nx, ny, nz;
};

typedef struct jtb_model_data_struct jtb_model_data;
struct jtb_model_data_struct
{
    f32 model_data[16];
    f32 normal_data[16];
    jfw_color color;
};

static inline jtb_model_data jtb_truss_convert_model_data(mtx4 model_transform, mtx4 normal_transform, jfw_color c)
{
    jtb_model_data data;
    memcpy(data.model_data, model_transform.data, sizeof(mtx4));
    memcpy(data.normal_data, normal_transform.data, sizeof(mtx4));
    data.color = c;
    return data;
}

typedef struct jtb_model_struct jtb_model;
struct jtb_model_struct
{
    u16 vtx_count, idx_count;
    jtb_vertex* vtx_array;
    u16* idx_array;
};

typedef struct jtb_truss_mesh_struct jtb_truss_mesh;
struct jtb_truss_mesh_struct
{
    jtb_model model;
    u32 count;
    u32 capacity;
    mtx4* model_matrices;
    mtx4* normal_matrices;
    jfw_color* colors;
};

typedef struct jtb_sphere_mesh_struct jtb_sphere_mesh;
struct jtb_sphere_mesh_struct
{
    jtb_model model;
    u32 count;
    u32 capacity;
    mtx4* model_matrices;
    mtx4* normal_matrices;
    jfw_color* colors;
};

extern const u64 DEFAULT_MESH_CAPACITY;

gfx_result truss_mesh_init(jtb_truss_mesh* mesh, u16 pts_per_side);

gfx_result truss_mesh_add_new(jtb_truss_mesh* mesh, mtx4 model_transform, mtx4 normal_transform, jfw_color color);

gfx_result truss_mesh_add_between_pts(jtb_truss_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll);

gfx_result truss_mesh_uninit(jtb_truss_mesh* mesh);



gfx_result sphere_mesh_init(jtb_sphere_mesh* mesh, u16 nw, u16 nh);

gfx_result sphere_mesh_add_new(jtb_sphere_mesh* mesh, mtx4 model_transform, jfw_color color);

gfx_result sphere_mesh_add_at_location(jtb_sphere_mesh* mesh, jfw_color color, f32 radius, vec4 pt);

gfx_result sphere_mesh_uninit(jtb_sphere_mesh* mesh);


gfx_result draw_3d_scene(
        jfw_window* wnd, vk_state* state, jfw_window_vk_resources* vk_resources, vk_buffer_allocation* p_buffer_geo,
        vk_buffer_allocation* p_buffer_mod, const jtb_truss_mesh* mesh,
        const jtb_camera_3d* camera);


#endif //JTB_DRAWING_3D_H
