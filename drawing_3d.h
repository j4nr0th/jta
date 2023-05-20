//
// Created by jan on 18.5.2023.
//

#ifndef JTB_DRAWING_3D_H
#define JTB_DRAWING_3D_H
#include "mem/aligned_jalloc.h"
#include "mem/jalloc.h"
#include "mem/lin_jalloc.h"

#include "jfw/window.h"
#include "jfw/error_system/error_codes.h"
#include "jfw/error_system/error_stack.h"


#include "vk_state.h"

typedef struct jtb_truss_vertex_struct jtb_truss_vertex;
struct jtb_truss_vertex_struct
{
    f32 x, y, z;
};

typedef struct jtb_truss_model_struct jtb_truss_model;
struct jtb_truss_model_struct
{
    u16 vtx_count, idx_count, pts_per_side;
    jtb_truss_vertex* vtx_array;
    u16* idx_array;
};

typedef struct jtb_truss_mesh_struct jtb_truss_mesh;
struct jtb_truss_mesh_struct
{
    jtb_truss_model model;
    u32 count;
    u32 capacity;
    mtx4* model_matrices;
    jfw_color* colors;
};

jfw_res generate_truss_model(jtb_truss_model* p_out, u16 pts_per_side);

jfw_res clean_truss_model(jtb_truss_model* model);

jfw_res truss_mesh_init(jtb_truss_mesh* mesh, u16 pts_per_side);

jfw_res truss_mesh_add_new(jtb_truss_mesh* mesh, mtx4 model_transform, jfw_color color);

jfw_res truss_mesh_add_between_pts(jtb_truss_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll);

jfw_res truss_mesh_uninit(jtb_truss_mesh* mesh);

jfw_res draw_3d_scene(jfw_window* wnd, vk_state* state, jfw_window_vk_resources* vk_resources);


#endif //JTB_DRAWING_3D_H
