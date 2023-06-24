//
// Created by jan on 21.5.2023.
//

#ifndef JTB_CAMERA_H
#define JTB_CAMERA_H
#include "common.h"

typedef struct jtb_camera_3d_state_struct jtb_camera_3d;
struct jtb_camera_3d_state_struct
{
    vec4 target;
    vec4 position;
    vec4 ux, uy, uz;
    f32 near, far;
};

void jtb_camera_set(jtb_camera_3d* camera, vec4 target, vec4 camera_pos, vec4 down);

mtx4 jtb_camera_to_view_matrix(const jtb_camera_3d* camera);

void jtb_camera_zoom(jtb_camera_3d* camera, f32 fraction_change);

void jtb_camera_rotate(jtb_camera_3d* camera, vec4 axis_of_rotation, f32 angle);

#endif //JTB_CAMERA_H
