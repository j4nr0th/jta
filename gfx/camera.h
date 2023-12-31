//
// Created by jan on 21.5.2023.
//

#ifndef JTA_CAMERA_H
#define JTA_CAMERA_H
#include "../common/common.h"

typedef struct jta_camera_3d_state_T jta_camera_3d;
struct jta_camera_3d_state_T
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

void jta_camera_find_depth_planes(const jta_camera_3d* camera, f32* p_near, f32* p_far);

void jta_camera_set(jta_camera_3d* camera, vec4 target, vec4 geo_o, f32 geo_r, vec4 camera_pos, vec4 down, f32 turn_sensitivity, f32 move_sensitivity);

void jta_camera_set_turn_sensitivity(jta_camera_3d* camera, f32 turn_sensitivity);

void jta_camera_set_move_sensitivity(jta_camera_3d* camera, f32 move_sensitivity);

mtx4 jta_camera_to_view_matrix(const jta_camera_3d* camera);

void jta_camera_zoom(jta_camera_3d* camera, f32 fraction_change);

void jta_camera_rotate(jta_camera_3d* camera, vec4 axis_of_rotation, f32 angle);

void jta_camera_move(jta_camera_3d* camera, vec4 direction);

#endif //JTA_CAMERA_H
