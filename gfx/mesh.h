//
// Created by jan on 6.7.2023.
//

#ifndef JTA_MESH_H
#define JTA_MESH_H
#include "../common/common.h"
#include "../mem/vk_mem_allocator.h"
#include "gfxerr.h"
#include "vk_state.h"


typedef struct jta_vertex_struct jta_vertex;
struct jta_vertex_struct
{
    f32 x, y, z;
    f32 nx, ny, nz;
};

typedef struct jta_model_data_struct jta_model_data;
struct jta_model_data_struct
{
    f32 model_data[16];
    f32 normal_data[16];
    jfw_color color;
};

typedef struct jta_model_struct jta_model;
struct jta_model_struct
{
    uint16_t vtx_count, idx_count;
    jta_vertex* vtx_array;
    uint16_t* idx_array;
};

typedef struct jta_mesh_struct jta_mesh;
struct jta_mesh_struct
{
    const char* name;                                               //  What the mesh is
    bool up_to_date;                                                //  When 0, instance data is not yet updated on the GPU side
    vk_buffer_allocation common_geometry_vtx, common_geometry_idx;  //  Memory allocations for common geometry (vtx and idx buffers)
    vk_buffer_allocation instance_memory;                           //  Memory allocation for instance geometry
    jta_model model;                                                //  Actual model data
    u32 count;                                                      //  Number of instances in the mesh
    u32 capacity;
    jta_model_data* model_data;
};

static inline uint64_t mesh_polygon_count(const jta_mesh* mesh)
{
    return mesh->count * mesh->model.idx_count / 3;
}

gfx_result jta_mesh_update_instance(jta_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state);

gfx_result jta_mesh_update_model(jta_mesh* mesh, jfw_window_vk_resources* resources, vk_state* state);

extern const u64 DEFAULT_MESH_CAPACITY;

gfx_result mesh_init_truss(jta_mesh* mesh, u16 pts_per_side, vk_state* state, jfw_window_vk_resources* resources);

gfx_result
truss_mesh_add_between_pts(jta_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll, vk_state* state);

gfx_result mesh_uninit(jta_mesh* mesh);

gfx_result mesh_init_sphere(jta_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources);

gfx_result sphere_mesh_add(jta_mesh* mesh, jfw_color color, f32 radius, vec4 pt, vk_state* state);

gfx_result sphere_mesh_add_deformed(
        jta_mesh* mesh, jfw_color color, f32 radius_x, f32 radius_y, f32 radius_z, vec4 pt, vk_state* state);

gfx_result mesh_init_cone(jta_mesh* mesh, u16 order, vk_state* state, jfw_window_vk_resources* resources);

gfx_result cone_mesh_add_between_pts(jta_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, vk_state* state);

#endif //JTA_MESH_H
