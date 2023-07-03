//
// Created by jan on 21.5.2023.
//

#include "camera.h"

void jtb_camera_set(jtb_camera_3d* camera, vec4 target, vec4 camera_pos, vec4 down, f32 turn_sensitivity, f32 move_sensitivity)
{
    camera->target = target;
    camera->position = camera_pos;
    vec4 d = vec4_sub(target, camera_pos);          //  Vector from camera's origin to camera's target
    assert(vec4_magnitude(d) > 0.0f);
    assert(vec4_magnitude(down) > 0.0f);            //  Down vector can not be zero
    vec4 right_vector = vec4_cross((down), d);      //  Right unit vector, which will be ux, from k1 * uy and k2 * uz
    assert(vec4_magnitude(right_vector) > 0.0f);    //  Down unit vector can not be the same as target direction
    vec4 unit_y = vec4_cross(d, right_vector);      //  Obtain uy by  cross product of k1 * uz and k2 * ux
    //  normalize
    unit_y = vec4_unit(unit_y);
    vec4 unit_x = vec4_unit(right_vector);
    vec4 unit_z = vec4_unit(d);
    camera->ux = unit_x;
    camera->uy = unit_y;
    camera->uz = unit_z;
    camera->turn_sensitivity = turn_sensitivity;
    camera->move_sensitivity = move_sensitivity;
    camera->far = 1000.0f * vec4_magnitude(d);      //  Far is taken as 1000 times further away from the target
    camera->near = 0.001f * vec4_magnitude(d);      //  Near is taken as 1000 times closer than target
}

mtx4 jtb_camera_to_view_matrix(const jtb_camera_3d* camera)
{
    //  Position of camera must be transferred into camera's coordinate system
    mtx4 m = (mtx4)
            {
                    .col0 = { .x = camera->ux.x, .y = camera->uy.x, .z = camera->uz.x, .w = 0.0f },
                    .col1 = { .x = camera->ux.y, .y = camera->uy.y, .z = camera->uz.y, .w = 0.0f },
                    .col2 = { .x = camera->ux.z, .y = camera->uy.z, .z = camera->uz.z, .w = 0.0f },
                    .col3 = { .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f },
            };

    vec4 real_pos = mtx4_vector_mul(m, camera->position);
    m.col3 = vec4_negative(real_pos);
    assert(vec4_equal(VEC4(0, 0, 0), mtx4_vector_mul(m, camera->position)));
    return m;
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
    angle *= camera->turn_sensitivity;
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

void jtb_camera_set_turn_sensitivity(jtb_camera_3d* camera, f32 turn_sensitivity)
{
    camera->turn_sensitivity = turn_sensitivity;
}

void jtb_camera_set_move_sensitivity(jtb_camera_3d* camera, f32 move_sensitivity)
{
    camera->move_sensitivity = move_sensitivity;
}

void jtb_camera_move(jtb_camera_3d* camera, vec4 direction)
{
    vec4 m = vec4_mul_one(direction, camera->move_sensitivity);
    camera->position = vec4_add(camera->position, m);
    camera->target = vec4_add(camera->target, m);
}


