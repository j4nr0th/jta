//
// Created by jan on 21.5.2023.
//

#include "camera.h"

void jtb_camera_set(jtb_camera_3d* camera, vec4 target, vec4 camera_pos, vec4 down)
{
    camera->target = target;
    camera->position = camera_pos;
    vec4 d = vec4_sub(target, camera_pos);
    assert(vec4_magnitude(d) > 0.0f);
    assert(vec4_magnitude(down) > 0.0f);
    vec4 right_vector = vec4_cross((down), d);
    assert(vec4_magnitude(right_vector) > 0.0f);
    vec4 unit_y = down;//vec4_cross(d, right_vector);
    //  normalize
    unit_y = vec4_unit(unit_y);
    vec4 unit_x = vec4_unit(right_vector);
    vec4 unit_z = vec4_unit(d);
    camera->ux = unit_x;
    camera->uy = unit_y;
    camera->uz = unit_z;
    camera->far = 1000.0f * vec4_magnitude(d);
    camera->near = 0.001f * vec4_magnitude(d);
}

mtx4 jtb_camera_to_view_matrix(const jtb_camera_3d* camera)
{
    return (mtx4)
            {
        .col0 = {.x = camera->ux.x, .y = camera->uy.x, .z = camera->uz.x},
        .col1 = {.x = camera->ux.y, .y = camera->uy.y, .z = camera->uz.y},
        .col2 = {.x = camera->ux.z, .y = camera->uy.z, .z = camera->uz.z},
        .col3 = {.x = camera->position.x, .y = camera->position.y, .z = camera->position.z, .w = 1.0f},
            };
}

void jtb_camera_zoom(jtb_camera_3d* camera, f32 fraction_change)
{
    vec4 relative = vec4_sub(camera->position, camera->target);
    relative = vec4_mul_one(relative, 1.0f + fraction_change);
    camera->far = 1000.0f * vec4_magnitude(relative);
    camera->near = 0.001f * vec4_magnitude(relative);
    camera->position = vec4_add(relative, camera->target);
}

void jtb_camera_rotate(jtb_camera_3d* camera, vec4 axis_of_rotation, f32 angle)
{
    vec4 relative = vec4_sub(camera->position, camera->target);
    mtx4 transformation = mtx4_rotate_around_axis(axis_of_rotation, angle);
    assert(vec4_close(vec4_unit(camera->uz), vec4_unit(vec4_negative(relative))));
    relative = mtx4_vector_mul(transformation, relative);
    camera->position = vec4_add(relative, camera->target);
    camera->ux = mtx4_vector_mul(transformation, camera->ux);
    camera->uy = mtx4_vector_mul(transformation, camera->uy);
    camera->uz = mtx4_vector_mul(transformation, camera->uz);
    assert(vec4_close(vec4_unit(camera->uz), vec4_unit(vec4_negative(relative))));

}


