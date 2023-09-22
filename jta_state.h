//
// Created by jan on 17.9.2023.
//

#ifndef JTA_JTA_STATE_H
#define JTA_JTA_STATE_H
#include "common/common.h"
#include "config/config_loading.h"
#include "gfx/vk_resources.h"
#include "ui/jwin_handlers.h"
#include <jrui.h>

typedef enum jta_display_state_T jta_display_state;
enum jta_display_state_T
{
    JTA_DISPLAY_NONE = 0,
    JTA_DISPLAY_UNDEFORMED = 1 << 0,
    JTA_DISPLAY_DEFORMED = 1 << 1,
};

typedef enum jta_problem_state_T jta_problem_state;
enum jta_problem_state_T
{
    JTA_PROBLEM_STATE_PROBLEM_LOADED = 1 << 0,  //  Set when an undeformed mesh is available for display
    JTA_PROBLEM_STATE_HAS_SOLUTION = 1 << 1,    //  Set when the problem was solved
};

typedef struct jta_ui_state_T jta_ui_state;
struct jta_ui_state_T
{
    jrui_context* ui_context;
    jta_texture* ui_font_texture;
    jvm_buffer_allocation* ui_vtx_buffer;
    jvm_buffer_allocation* ui_idx_buffer;
};

struct jta_state_T
{
    jta_config master_config;
    jta_draw_state draw_state;
    jta_ui_state ui_state;
    jwin_context* wnd_ctx;
    jwin_window* main_wnd;
    jta_problem_setup problem_setup;
    jta_solution problem_solution;
    jta_display_state display_state;
    jta_problem_state problem_state;
    jio_context* io_ctx;
};
typedef struct jta_state_T jta_state;

#endif //JTA_JTA_STATE_H
