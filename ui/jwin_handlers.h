//
// Created by jan on 21.5.2023.
//

#ifndef JTA_JWIN_HANDLERS_H
#define JTA_JWIN_HANDLERS_H
#include "../common/common.h"
#include "../gfx/camera.h"
#include "../core/jtaproblem.h"
#include "../core/jtasolve.h"
#include "../gfx/mesh.h"
#include "../jwin/source/jwin.h"
#include "../gfx/textures.h"
#include "jrui.h"


typedef struct jta_draw_state_T jta_draw_state;
struct jta_draw_state_T
{
    jta_vulkan_context* vk_ctx;
    jta_vulkan_window_context* wnd_ctx;
    jta_camera_3d camera;
    jta_camera_3d original_camera;
    unsigned long last_mmb_release;
    u32 track_turn;
    i32 track_move;
    i32 mv_x, mv_y;
    int screenshot;
    jta_structure_meshes* undeformed_mesh;
    jta_structure_meshes* deformed_mesh;
    mtx4 view_matrix;
    int needs_redraw;
};

struct jta_event_handler_T
{
    jwin_event_type type;
    jwin_event_callback callback;
};
typedef struct jta_event_handler_T jta_event_handler;

extern const jta_event_handler JTA_HANDLER_ARRAY[];

extern const unsigned JTA_HANDLER_COUNT;

#endif //JTA_JWIN_HANDLERS_H
