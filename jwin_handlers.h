//
// Created by jan on 21.5.2023.
//

#ifndef JTA_JWIN_HANDLERS_H
#define JTA_JWIN_HANDLERS_H
#include "common/common.h"
#include "gfx/camera.h"
#include "core/jtaproblem.h"
#include "core/jtasolve.h"
#include "gfx/mesh.h"
#include "jwin/source/jwin.h"
#include <jrui.h>

typedef struct jta_ui_state_struct jta_ui_state;
struct jta_ui_state_struct
{
    jrui_context* ui_context;
    vk_buffer_allocation ui_vtx_buffer;
    vk_buffer_allocation ui_idx_buffer;
};

typedef struct jta_draw_state_struct jta_draw_state;
struct jta_draw_state_struct
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
    jta_problem_setup* p_problem;
    jta_solution* p_solution;
    jta_config* config;
    jta_structure_meshes meshes;
    mtx4 view_matrix;
    int needs_redraw;
    jta_ui_state ui_state;
};

struct jta_event_handler_struct
{
    jwin_event_type type;
    jwin_event_callback callback;
};
typedef struct jta_event_handler_struct jta_event_handler;

extern const jta_event_handler JTA_HANDLER_ARRAY[];

extern const unsigned JTA_HANDLER_COUNT;

#endif //JTA_JWIN_HANDLERS_H
