//
// Created by jan on 21.5.2023.
//
#include "common.h"
#include "gfx_math.h"
#include "camera.h"
#include "drawing_3d.h"
#include "rotation_camera_test.c"

static inline void print_matrix(mtx4 matrix)
{
    printf("% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n", matrix.col0.s0, matrix.col1.s0, matrix.col2.s0, matrix.col3.s0, matrix.col0.s1, matrix.col1.s1, matrix.col2.s1, matrix.col3.s1, matrix.col0.s2, matrix.col1.s2, matrix.col2.s2, matrix.col3.s2, matrix.col0.s3, matrix.col1.s3, matrix.col2.s3, matrix.col3.s3);
}

static inline void print_vec(vec4 v)
{
    printf("% 4.4f\n% 4.4f\n% 4.4f\n% 4.4f\n", v.x, v.y, v.z, v.w);
}
#include <stdlib.h>
#define ASSERT(x) if (!(x)) {fprintf(stderr, "Failed assertion \"" #x "\"\n"); __builtin_trap(); exit(EXIT_FAILURE);}(void)0

int main()
{
    {
        vec4 v1;
        vec4 v2;
        vec4 v3;
        mtx4 m;

        //  Rotation about Z axis
        v1 = VEC4(0, 0, 1);

        //      Rotating X axis
        v2 = VEC4(1, 0, 0);
        m = mtx4_rotate_around_axis(v1, M_PI_2);    //  90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1, 0)));
        m = mtx4_rotate_around_axis(v1, M_PI_4);    //  45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1 / sqrtf(2), 1 / sqrtf(2), 0)));
        m = mtx4_rotate_around_axis(v1, -M_PI_2);    //  -90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, -1, 0)));
        m = mtx4_rotate_around_axis(v1, -M_PI_4);    //  -45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1 / sqrtf(2), -1 / sqrtf(2), 0)));
        //      Rotating Y axis
        v2 = VEC4(0, 1, 0);
        m = mtx4_rotate_around_axis(v1, M_PI_2);    //  90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(-1, 0, 0)));
        m = mtx4_rotate_around_axis(v1, M_PI_4);    //  45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(-1 / sqrtf(2), 1 / sqrtf(2), 0)));
        m = mtx4_rotate_around_axis(v1, -M_PI_2);    //  -90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1, 0, 0)));
        m = mtx4_rotate_around_axis(v1, -M_PI_4);    //  -45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1 / sqrtf(2), 1 / sqrtf(2), 0)));

        //  Rotation about Y axis
        v1 = VEC4(0, 1, 0);

        //      Rotating Z axis
        v2 = VEC4(0, 0, 1);
        m = mtx4_rotate_around_axis(v1, M_PI_2);    //  90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1, 0, 0)));
        m = mtx4_rotate_around_axis(v1, M_PI_4);    //  45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1 / sqrtf(2), 0, 1 / sqrtf(2))));
        m = mtx4_rotate_around_axis(v1, -M_PI_2);    //  -90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(-1, 0, 0)));
        m = mtx4_rotate_around_axis(v1, -M_PI_4);    //  -45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(-1 / sqrtf(2), 0, 1 / sqrtf(2))));
        //      Rotating X axis
        v2 = VEC4(1, 0, 0);
        m = mtx4_rotate_around_axis(v1, M_PI_2);    //  90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 0, -1)));
        m = mtx4_rotate_around_axis(v1, M_PI_4);    //  45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1 / sqrtf(2), 0, -1 / sqrtf(2))));
        m = mtx4_rotate_around_axis(v1, -M_PI_2);    //  -90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 0, 1)));
        m = mtx4_rotate_around_axis(v1, -M_PI_4);    //  -45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1 / sqrtf(2), 0, 1 / sqrtf(2))));

        //  Rotation about X axis
        v1 = VEC4(1, 0, 0);

        //      Rotating Y axis
        v2 = VEC4(0, 1, 0);
        m = mtx4_rotate_around_axis(v1, M_PI_2);    //  90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 0, 1)));
        m = mtx4_rotate_around_axis(v1, M_PI_4);    //  45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1 / sqrtf(2), 1 / sqrtf(2))));
        m = mtx4_rotate_around_axis(v1, -M_PI_2);    //  -90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 0, -1)));
        m = mtx4_rotate_around_axis(v1, -M_PI_4);    //  -45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1 / sqrtf(2), -1 / sqrtf(2))));
        //      Rotating Z axis
        v2 = VEC4(0, 0, 1);
        m = mtx4_rotate_around_axis(v1, M_PI_2);    //  90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, -1, 0)));
        m = mtx4_rotate_around_axis(v1, M_PI_4);    //  45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, -1 / sqrtf(2), 1 / sqrtf(2))));
        m = mtx4_rotate_around_axis(v1, -M_PI_2);    //  -90 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1, 0)));
        m = mtx4_rotate_around_axis(v1, -M_PI_4);    //  -45 degrees
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1 / sqrtf(2), 1 / sqrtf(2))));


        //  Rotating around axis (1, 1, 1)
        v1 = VEC4(1, 1, 1);

        //      Rotating X axis
        v2 = VEC4(1, 0, 0);
        m = mtx4_rotate_around_axis(v1, (f32)(2.0 * M_PI / 3.0));
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1, 0)));
        m = mtx4_rotate_around_axis(v1, (f32)(2.0 * 2.0 * M_PI / 3.0));
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 0, 1)));

        //      Rotating Y axis
        v2 = VEC4(0, 1, 0);
        m = mtx4_rotate_around_axis(v1, (f32)(2.0 * M_PI / 3.0));
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 0, 1)));
        m = mtx4_rotate_around_axis(v1, (f32)(2.0 * 2.0 * M_PI / 3.0));
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1, 0, 0)));
        //      Rotating Z axis
        v2 = VEC4(0, 0, 1);
        m = mtx4_rotate_around_axis(v1, (f32)(2.0 * M_PI / 3.0));
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(1, 0, 0)));
        m = mtx4_rotate_around_axis(v1, (f32)(2.0 * 2.0 * M_PI / 3.0));
        v3 = mtx4_vector_mul(m, v2);
        ASSERT(vec4_close(v3, VEC4(0, 1, 0)));
    }

    ASSERT(test_camera_rotation());
    jtb_camera_3d camera;

    //  Check that vectors are properly projected
    {
        mtx4 m = {};
        vec4 v = {}, v_res = {};
        jtb_camera_set(&camera, VEC4(1, 1, 1), VEC4(1, 1, 0), VEC4(0, -1, 0));
        m = jtb_camera_to_view_matrix(&camera);
        v = mtx4_vector_mul(m, VEC4(1.1, 0.9, 0.7));
        ASSERT(vec4_close(v, VEC4(-0.1, +0.1, 0.7)));
        v = mtx4_vector_mul(m, VEC4(0, 0, 0));
        ASSERT(vec4_close(v, VEC4(+1, +1, 0)));


        jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(1, 0, 0), VEC4(0, 0, 1));
        m = jtb_camera_to_view_matrix(&camera);
        v = mtx4_vector_mul(m, VEC4(0, 1, 0));
        ASSERT(vec4_close(v, VEC4(-1, 0, 1)));
        v = mtx4_vector_mul(m, VEC4(0, 0, 1));
        ASSERT(vec4_close(v, VEC4(0, 1, 1)));
        v = mtx4_vector_mul(m, VEC4(0, 0, 0));
        ASSERT(vec4_close(v, VEC4(0, 0, 1)));


        jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(0, 0, +1), VEC4(0, 1, 0));
        m = jtb_camera_to_view_matrix(&camera);
        v = mtx4_vector_mul(m, VEC4(0, 3, 0));
        ASSERT(vec4_close(v, VEC4(0, 3, 1)));
        v = mtx4_vector_mul(m, VEC4(2, 0, 0));
        ASSERT(vec4_close(v, VEC4(-2, 0, 1)));
        v = mtx4_vector_mul(m, VEC4(0, 0, 0));
        ASSERT(vec4_close(v, VEC4(0, 0, 1)));


        //  Test camera rotation around correct axis
        jtb_camera_rotate(&camera, VEC4(0, 1, 0), M_PI_2);
        m = jtb_camera_to_view_matrix(&camera);
        v = mtx4_vector_mul(m, VEC4(0, 1, 0));
        ASSERT(vec4_close(v, VEC4(0, 1, 1)));
        v = mtx4_vector_mul(m, VEC4(0, 0, 1));
        ASSERT(vec4_close(v, VEC4(1, 0, 1)));
        v = mtx4_vector_mul(m, VEC4(0, 0, 0));
        ASSERT(vec4_close(v, VEC4(0, 0, 1)));

        {
            vec4 ux = camera.ux, uy = camera.uy, uz = camera.uz;
            jtb_camera_set(&camera, VEC4(-10, -3, 2.3f), VEC4(3, -3, 2.3f), VEC4(0, 1, 0));
            ASSERT(vec4_close(ux, camera.ux));
            ASSERT(vec4_close(uy, camera.uy));
            ASSERT(vec4_close(uz, camera.uz));
        }
        jtb_camera_rotate(&camera, VEC4(1, 1, 1), -(f32)(M_PI * 2.0 / 3.0));
        ASSERT(vec4_close(camera.ux, VEC4(0, 1, 0)));
        ASSERT(vec4_close(camera.uy, VEC4(1, 0, 0)));
        ASSERT(vec4_close(camera.uz, VEC4(0, 0, -1)));


        exit(EXIT_SUCCESS);
    }

    jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(0, 0, 1), VEC4(0, 1, 0));
    mtx4 m = jtb_camera_to_view_matrix(&camera);
    print_matrix(m);
    printf("\n");
    vec4 v = VEC4(0, 0, -1);
    vec4 v_res = mtx4_vector_mul(m, v);
    ASSERT(vec4_close(v_res, VEC4(0, 0, 2)));
    v = VEC4(0, 0, -10);
    v_res = mtx4_vector_mul(m, v);
    ASSERT(vec4_close(v_res, VEC4(0, 0, 11)));
    v = VEC4(1, 0, 0);
    v_res = mtx4_vector_mul(m, v);
    ASSERT(vec4_close(v_res, VEC4(-1, 0, 1)));
    v = VEC4(0, 3, 0);
    v_res = mtx4_vector_mul(m, v);
    ASSERT(vec4_close(v_res, VEC4(0, 3, 1)));
    v = VEC4(1, 3, -10);
    v_res = mtx4_vector_mul(m, v);
    ASSERT(vec4_close(v_res, VEC4(-1, 3, 11)));


    jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(1, 1, 0), VEC4(1, -1, 0));
    m = jtb_camera_to_view_matrix(&camera);
    print_matrix(m);
    printf("\n");

    v = VEC4(0, 0, -10);
    v_res = mtx4_vector_mul(m, v);
    ASSERT(vec4_close(v_res, VEC4(10, 0, 1/sqrtf(2))));



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
