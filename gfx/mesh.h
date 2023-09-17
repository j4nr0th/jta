//
// Created by jan on 6.7.2023.
//

#ifndef JTA_MESH_H
#define JTA_MESH_H

#include <jvm.h>
#include "../common/common.h"
#include "gfxerr.h"
#include "../core/jtaproblem.h"
#include "../core/jtasolve.h"
#include "bounding_box.h"
#include "vk_resources.h"


typedef struct jta_vertex_T jta_vertex;
struct jta_vertex_T
{
    f32 x, y, z;
    f32 nx, ny, nz;
};

typedef struct jta_model_data_T jta_model_data;
struct jta_model_data_T
{
    f32 model_data[16];
    f32 normal_data[16];
    jta_color color;
};

typedef struct jta_model_T jta_model;
struct jta_model_T
{
    uint16_t vtx_count, idx_count;
    jta_vertex* vtx_array;
    uint16_t* idx_array;
};

typedef struct jta_mesh_T jta_mesh;
struct jta_mesh_T
{
    const char* name;                                               //  What the mesh is
    bool up_to_date;                                                //  When 0, instance data is not yet updated on the GPU side
    jvm_buffer_allocation* common_geometry_vtx, *common_geometry_idx;  //  Memory allocations for common geometry (vtx and idx buffers)
    jvm_buffer_allocation* instance_memory;                           //  Memory allocation for instance geometry
    jta_model model;                                                //  Actual model data
    u32 count;                                                      //  Number of instances in the mesh
    u32 capacity;                                                   //  How large the arrays are
    jta_model_data* model_data;                                     //  Array with per-instance model data
    jta_bounding_box bounding_box;                                  //  Mesh bounding box, which contains all primitives in it
};

static inline uint64_t mesh_polygon_count(const jta_mesh* mesh)
{
    return mesh->count * mesh->model.idx_count / 3;
}

struct jta_structure_meshes_T
{
    union
    {
        struct
        {
            jta_mesh cylinders;
            jta_mesh spheres;
            jta_mesh cones;
        };
        jta_mesh mesh_array[3];
    };
};

typedef struct jta_structure_meshes_T jta_structure_meshes;

gfx_result jta_structure_meshes_generate_undeformed(
        jta_structure_meshes* meshes, const jta_config_display* cfg, const jta_problem_setup* problem_setup,
        jta_vulkan_window_context* ctx);

gfx_result jta_structure_meshes_generate_deformed(
        jta_structure_meshes* meshes, const jta_config_display* cfg, const jta_problem_setup* problem_setup,
        const jta_solution* solution, jta_vulkan_window_context* ctx);

void jta_structure_meshes_destroy(jta_vulkan_window_context* ctx, jta_structure_meshes* meshes);

gfx_result jta_mesh_update_instance(jta_mesh* mesh, jta_vulkan_window_context* ctx);

gfx_result jta_mesh_update_model(jta_mesh* mesh, jta_vulkan_window_context* ctx);

extern const u64 DEFAULT_MESH_CAPACITY;

gfx_result mesh_init_truss(jta_mesh* mesh, u16 pts_per_side, jta_vulkan_window_context* ctx);

gfx_result
truss_mesh_add_between_pts(jta_mesh* mesh, jta_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll, jta_vulkan_window_context* ctx);

void mesh_destroy(const jta_vulkan_window_context* ctx, jta_mesh* mesh);

gfx_result mesh_init_sphere(jta_mesh* mesh, u16 order, jta_vulkan_window_context* ctx);

gfx_result sphere_mesh_add(jta_mesh* mesh, jta_color color, f32 radius, vec4 pt, jta_vulkan_window_context* ctx);

gfx_result sphere_mesh_add_deformed(
        jta_mesh* mesh, jta_color color, f32 radius_x, f32 radius_y, f32 radius_z, vec4 pt, jta_vulkan_window_context* ctx);

gfx_result mesh_init_cone(jta_mesh* mesh, u16 order, jta_vulkan_window_context* ctx);

gfx_result cone_mesh_add_between_pts(jta_mesh* mesh, jta_color color, f32 radius, vec4 pt1, vec4 pt2, jta_vulkan_window_context* ctx);

#endif //JTA_MESH_H
