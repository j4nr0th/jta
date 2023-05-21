//
// Created by jan on 21.5.2023.
//

#include "camera.h"

void jtb_camera_set(jtb_camera_3d* camera, vec4 target, vec4 camera_pos, f32 roll)
{
    camera->target = target;
    camera->roll = roll;
    camera->position = camera_pos;
}

mtx4 jtb_camera_to_view_matrix(const jtb_camera_3d* camera)
{
    mtx4 result = mtx4_view_look_at(camera->position, camera->target, camera->roll);
    return result;
}

void jtb_camera_zoom(jtb_camera_3d* camera, f32 fraction_change)
{
    vec4 relative = vec4_sub(camera->position, camera->target);
    relative = vec4_mul_one(relative, 1.0f + fraction_change);
    camera->position = vec4_add(relative, camera->target);
}

void jtb_camera_rotate(jtb_camera_3d* camera, vec4 axis_of_rotation, f32 angle)
{
    vec4 relative = vec4_sub(camera->position, camera->target);
    mtx4 transformation = mtx4_rotate_around_axis(axis_of_rotation, angle);
    relative = mtx4_vector_mul(transformation, relative);
    camera->position = vec4_add(relative, camera->target);
}


