//
// Created by jan on 6.7.2023.
//

#ifndef JTB_MESH_H
#define JTB_MESH_H
#include "../common/common.h"
#include "../mem/vk_mem_allocator.h"
#include "gfxerr.h"
#include "vk_state.h"


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

typedef struct jtb_model_struct jtb_model;
struct jtb_model_struct
{
    uint16_t vtx_count, idx_count;
    jtb_vertex* vtx_array;
    uint16_t* idx_array;
};

typedef struct jtb_mesh_struct jtb_mesh;
struct jtb_mesh_struct
{
    const char* name;                                               //  What the mesh is
    bool up_to_date;                                                //  When 0, instance data is not yet updated on the GPU side
    vk_buffer_allocation common_geometry_vtx, common_geometry_idx;  //  Memory allocations for common geometry (vtx and idx buffers)
    vk_buffer_allocation instance_memory;                           //  Memory allocation for instance geometry
    jtb_model model;                                                //  Actual model data
    u32 count;                                                      //  Number of instances in the mesh
    u32 capacity;
    jtb_model_data* model_data;
};

static inline uint64_t mesh_polygon_count(const jtb_mesh* mesh)
{
    return mesh->count * mesh->model.idx_count / 3;
}

gfx_result jtb_mesh_update_instance(jtb_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state);

gfx_result jtb_mesh_update_model(jtb_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state);

extern const u64 DEFAULT_MESH_CAPACITY;

gfx_result mesh_init_truss(jtb_mesh* mesh, u16 pts_per_side, vk_state* state, jfw_window_vk_resources* resources);

gfx_result
truss_mesh_add_between_pts(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll, vk_state* state);

gfx_result mesh_uninit(jtb_mesh* mesh);

gfx_result mesh_init_sphere(jtb_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources);

gfx_result sphere_mesh_add(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt, vk_state* state);

gfx_result sphere_mesh_add_deformed(
        jtb_mesh* mesh, jfw_color color, f32 radius_x, f32 radius_y, f32 radius_z, vec4 pt, vk_state* state);

gfx_result mesh_init_cone(jtb_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources);

gfx_result cone_mesh_add_between_pts(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, vk_state* state);

#endif //JTB_MESH_H
