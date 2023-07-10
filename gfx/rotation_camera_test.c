//
// Created by jan on 15.6.2023.
//

#include "../common/common.h"
#include "gfx_math.h"
#include "camera.h"
#include "drawing_3d.h"

static int test_camera_rotation(void)
{
    jta_camera_3d camera;
    //  Check if setting the camera will face the correct direction
    jta_camera_set(&camera, VEC4(0, 0, 0), VEC4(-1, 0, 0), VEC4(0, 0, -1), 1.0f, 1.0f);
    if (!vec4_equal(camera.uz, VEC4(1, 0, 0)))
    {
        return 0;
    }

    if (!vec4_equal(camera.uy, VEC4(0, 0, -1)))
    {
        return 0;
    }

    if (!vec4_equal(camera.ux, VEC4(0, -1, 0)))
    {
        return 0;
    }

    //  Check if the camera rotation around z-axis works as intended
    jta_camera_rotate(&camera, VEC4(0, 0, 1), M_PI_2);
    if (!vec4_close(camera.uz, VEC4(0, 1, 0)))
    {
        return 0;
    }

    if (!vec4_close(camera.uy, VEC4(0, 0, -1)))
    {
        return 0;
    }

    if (!vec4_close(camera.ux, VEC4(1, 0, 0)))
    {
        return 0;
    }

    jta_camera_set(&camera, VEC4(0, 0, 0), VEC4(-1, 0, 0), VEC4(0, 0, -1), 1.0f, 1.0f);

    //  Check if the camera rotation around y-axis works as intended
    jta_camera_rotate(&camera, VEC4(0, 1, 0), M_PI_2);
    if (!vec4_close(camera.uz, VEC4(0, 0, -1)))
    {
        return 0;
    }

    if (!vec4_close(camera.uy, VEC4(-1, 0, 0)))
    {
        return 0;
    }

    if (!vec4_close(camera.ux, VEC4(0, -1, 0)))
    {
        return 0;
    }

    jta_camera_set(&camera, VEC4(0, 0, 0), VEC4(-1, 0, 0), VEC4(0, 0, -1), 1.0f, 1.0f);

    //  Check if the camera rotation around x-axis works as intended
    jta_camera_rotate(&camera, VEC4(1, 0, 0), M_PI_2);
    if (!vec4_close(camera.uz, VEC4(1, 0, 0)))
    {
        return 0;
    }

    if (!vec4_close(camera.uy, VEC4(0, 1, 0)))
    {
        return 0;
    }

    if (!vec4_close(camera.ux, VEC4(0, 0, -1)))
    {
        return 0;
    }


    //  Combined rotation check
    jta_camera_set(&camera, VEC4(0, 0, 0), VEC4(1, 1, 1), VEC4(1 / sqrtf(6), 1 / sqrtf(6), -2 / sqrtf(6)), 1.0f, 1.0f);
    if (!vec4_close(camera.uz, VEC4(-1/sqrtf(3), -1/sqrtf(3), -1/sqrtf(3))))
    {
        return 0;
    }

    if (!vec4_close(camera.uy, VEC4(1/sqrtf(6), 1/sqrtf(6), -2/sqrtf(6))))
    {
        return 0;
    }

    if (!vec4_close(camera.ux, VEC4(-1/sqrtf(2), 1/sqrtf(2), 0)))
    {
        return 0;
    }

    jta_camera_rotate(&camera, VEC4(10, 10, 0), M_PI);
    if (!vec4_close(camera.uz, VEC4(-1/sqrtf(3), -1/sqrtf(3), +1/sqrtf(3))))
    {
        return 0;
    }

    if (!vec4_close(camera.uy, VEC4(1/sqrtf(6), 1/sqrtf(6), +2/sqrtf(6))))
    {
        return 0;
    }

    if (!vec4_close(camera.ux, VEC4(+1/sqrtf(2), -1/sqrtf(2), 0)))
    {
        return 0;
    }


    return 1;
}