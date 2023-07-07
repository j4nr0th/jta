//
// Created by jan on 6.7.2023.
//

#ifndef JTB_MESH_H
#define JTB_MESH_H
#include "../common/common.h"
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
    jtb_model model;
    u32 count;
    u32 capacity;
    mtx4* model_matrices;
    mtx4* normal_matrices;
    jfw_color* colors;
};

static inline uint64_t mesh_polygon_count(const jtb_mesh* mesh)
{
    return mesh->count * mesh->model.idx_count / 3;
}



static inline jtb_model_data jtb_truss_convert_model_data(mtx4 model_transform, mtx4 normal_transform, jfw_color c)
{
    jtb_model_data data;
    memcpy(data.model_data, model_transform.data, sizeof(mtx4));
    memcpy(data.normal_data, normal_transform.data, sizeof(mtx4));
    data.color = c;
    return data;
}

extern const u64 DEFAULT_MESH_CAPACITY;

gfx_result mesh_init_truss(jtb_mesh* mesh, u16 pts_per_side);

gfx_result truss_mesh_add_between_pts(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt1, vec4 pt2, f32 roll);

gfx_result mesh_uninit(jtb_mesh* mesh);



gfx_result mesh_init_sphere(jtb_mesh* mesh, u16 nw, u16 nh);

gfx_result sphere_mesh_add(jtb_mesh* mesh, jfw_color color, f32 radius, vec4 pt);

gfx_result sphere_mesh_add_deformed(jtb_mesh* mesh, jfw_color color, f32 radius_x, f32 radius_y, f32 radius_z, vec4 pt);


#endif //JTB_MESH_H
