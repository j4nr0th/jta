//
// Created by jan on 21.5.2023.
//
#include "common.h"
#include "jfw/gfx_math.h"
#include "camera.h"
#include "drawing_3d.h"


static inline void print_matrix(mtx4 matrix)
{
    printf("% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n", matrix.col0.s0, matrix.col1.s0, matrix.col2.s0, matrix.col3.s0, matrix.col0.s1, matrix.col1.s1, matrix.col2.s1, matrix.col3.s1, matrix.col0.s2, matrix.col1.s2, matrix.col2.s2, matrix.col3.s2, matrix.col0.s3, matrix.col1.s3, matrix.col2.s3, matrix.col3.s3);
}

static inline void print_vec(vec4 v)
{
    printf("% 4.4f\n% 4.4f\n% 4.4f\n% 4.4f\n", v.x, v.y, v.z, v.w);
}

int main()
{
    jtb_camera_3d camera;
    jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(0, 0, -1), VEC4(0, 1, 0));
    mtx4 m = jtb_camera_to_view_matrix(&camera);
    print_matrix(m);
    printf("\n");
    vec4 v = VEC4(0, 0, 1);
    print_vec(v);
    printf("\n");
    print_vec(mtx4_vector_mul(m, v));
    printf("\n");

    jtb_truss_mesh mesh;
    truss_mesh_init(&mesh, 8);
    truss_mesh_add_between_pts(&mesh, (jfw_color){}, 0.001f, VEC4(0, 0, 0), VEC4(0, 0, 0.01), 0.0f);
    print_matrix(mesh.model_matrices[0]);
    printf("\n");
    print_vec(mtx4_vector_mul(mesh.model_matrices[0], VEC4(0, 0, 1)));
    printf("\n");
    print_vec(mtx4_vector_mul(m, mtx4_vector_mul(mesh.model_matrices[0], VEC4(0, 0, 1))));

    return 0;
}
