//
// Created by jan on 21.5.2023.
//

#ifndef JTB_CAMERA_H
#define JTB_CAMERA_H
#include "../common/common.h"

typedef struct jtb_camera_3d_state_struct jtb_camera_3d;
struct jtb_camera_3d_state_struct
{
    vec4 target;            //  Position of camera target in global coordinate system
    vec4 position;          //  Position of camera's origin in global coordinate system
    vec4 ux, uy, uz;        //  Unit vectors of camera in the global coordinate system
    f32 turn_sensitivity;   //  Factor by which look angles are multiplied with
    f32 move_sensitivity;   //  Factor by which move distances are multiplied with

    //  Actual geometry can be bound by a sphere located at geo_origin and radius of geo_radius
    vec4 geo_origin;
    f32 geo_radius;
};

void jtb_camera_find_depth_planes(const jtb_camera_3d* camera, f32* p_near, f32* p_far);

void jtb_camera_set(jtb_camera_3d* camera, vec4 target, vec4 geo_o, f32 geo_r, vec4 camera_pos, vec4 down, f32 turn_sensitivity, f32 move_sensitivity);

void jtb_camera_set_turn_sensitivity(jtb_camera_3d* camera, f32 turn_sensitivity);

void jtb_camera_set_move_sensitivity(jtb_camera_3d* camera, f32 move_sensitivity);

mtx4 jtb_camera_to_view_matrix(const jtb_camera_3d* camera);

void jtb_camera_zoom(jtb_camera_3d* camera, f32 fraction_change);

void jtb_camera_rotate(jtb_camera_3d* camera, vec4 axis_of_rotation, f32 angle);

void jtb_camera_move(jtb_camera_3d* camera, vec4 direction);

#endif //JTB_CAMERA_H
