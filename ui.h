//
// Created by jan on 21.5.2023.
//

#ifndef JTA_UI_H
#define JTA_UI_H
#include "common/common.h"
#include "gfx/vk_state.h"
#include "gfx/camera.h"

typedef struct jta_draw_state_struct jta_draw_state;
struct jta_draw_state_struct
{
    vk_state* vulkan_state;
    jfw_window_vk_resources* vulkan_resources;
    jta_camera_3d camera;
    jta_camera_3d original_camera;
    unsigned long last_mmb_release;
    u32 track_turn;
    i32 track_move;
    i32 mv_x, mv_y;
    int screenshot;
};


jfw_res truss_mouse_button_press(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);

jfw_res truss_mouse_button_double_press(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);

jfw_res truss_mouse_button_release(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);

jfw_res truss_mouse_motion(jfw_widget* this, i32 x, i32 y, u32 mods);

jfw_res truss_key_press(jfw_widget* this, KeySym key_sym);

#endif //JTA_UI_H
