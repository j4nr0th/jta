//
// Created by jan on 21.5.2023.
//

#ifndef JTB_UI_H
#define JTB_UI_H
#include "common.h"
#include "vk_state.h"
#include "camera.h"

typedef struct jtb_draw_state_struct jtb_draw_state;
struct jtb_draw_state_struct
{
    vk_state* vulkan_state;
    jfw_window_vk_resources* vulkan_resources;
    jtb_camera_3d camera;
    u32 track_move;
    i32 mv_x, mv_y;
};


jfw_res truss_mouse_button_press(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);

jfw_res truss_mouse_button_release(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);

jfw_res truss_mouse_motion(jfw_widget* this, i32 x, i32 y, u32 mods);

#endif //JTB_UI_H
