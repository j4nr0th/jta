//
// Created by jan on 3.9.2023.
//

#include <jdm.h>
#include "ui_sim_and_sol.h"
#include "ui_tree.h"

static jrui_widget_create_info GRAVITY_ROW[] =
        {
        [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "gravity x"}},
        [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "gravity y"}},
        [2] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "gravity z"}},
        };

static void drag_convergence_callback(jrui_widget_base* drag, float f, void* param)
{
    JDM_ENTER_FUNCTION;
    jrui_widget_base* converge_text = jrui_get_by_label(jrui_widget_get_context(drag), "convergence text");
    jrui_result res;
    if (!converge_text)
    {
        JDM_ERROR("Could not get the widget with label \"convergence text\", reason: %s (%s)", jrui_result_to_str(res), jrui_result_message(res));
        goto end;
    }

    const float converge_v = exp10f(6.0f * (f - 1.0f));
    if (1e-7f < converge_v && 1.0f >= converge_v)
    {
        p_cfg->problem.sim_and_sol.convergence_criterion = converge_v;

        char text_buffer[64] = {0};
        snprintf(text_buffer, sizeof(text_buffer), "%e", converge_v);
        jrui_text_h_create_info updated_info =
                {
                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "convergence text", .text = text_buffer
                };
        res = jrui_update_text_h(converge_text, &updated_info);
        if (res != JRUI_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not update convergence text widget, reason: %s (%s)", jrui_result_to_str(res), jrui_result_message(res));
        }
    }
    else
    {
        JDM_WARN("Value passed to drag_convergence_callback was %g, which results in converge_v of %e", f, converge_v);
    }
end:
    JDM_LEAVE_FUNCTION;
}
static void drag_relax_callback(jrui_widget_base* drag, float f, void* param)
{
    JDM_ENTER_FUNCTION;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%f", f);
    update_text_widget(jrui_widget_get_context(drag), "relax factor", buffer);

    JDM_LEAVE_FUNCTION;
}

static const jrui_widget_create_info WINDOW_ENTRIES_CHILDREN[] =
        {
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Gravity"}},
        {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(GRAVITY_ROW) / sizeof(*GRAVITY_ROW), .children = GRAVITY_ROW}},
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Convergence criterion"}},
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "convergence text"}},
        {.drag_h = {.base_info.type = JRUI_WIDGET_TYPE_DRAG_H, .manual_changes = 0.05f, .btn_ratio = 0.05f, .initial_pos = 0.0f, .param = NULL, .callback = drag_convergence_callback}},
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Max Iterations"}},
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "iteration count"}},
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Relaxation factor"}},
        {.drag_h = {.base_info.type = JRUI_WIDGET_TYPE_DRAG_H, .manual_changes = 0.05f, .btn_ratio = 0.05f, .initial_pos = 0.0f, .param = NULL, .callback = drag_relax_callback}},
        {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "relax factor"}},
        };

static const jrui_widget_create_info WINDOW_ENTRIES_COLUMN =
        {
        .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = sizeof(WINDOW_ENTRIES_CHILDREN) / sizeof(*WINDOW_ENTRIES_CHILDREN), .children = WINDOW_ENTRIES_CHILDREN},
        };

static const jrui_widget_create_info WINDOW_TITLE =
        {
        .text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Simulation and Solver"}
        };

static const jrui_widget_create_info STACK_CHILDREN[] =
        {
        [0] = {.containerf = {.base_info.type = JRUI_WIDGET_TYPE_CONTAINERF, .width = 1.0f, .height = 0.1f, .align_vertical = JRUI_ALIGN_TOP, .child = &WINDOW_TITLE, .has_background = 1}},
        [1] = {.containerf = {.base_info.type = JRUI_WIDGET_TYPE_CONTAINERF, .width = 1.0f, .height = 0.9f, .align_vertical = JRUI_ALIGN_BOTTOM, .child = &WINDOW_ENTRIES_COLUMN, .has_background = 1}},
        };

static const jrui_widget_create_info WINDOW_STACK =
        {
        .stack =
        {
                .base_info.type = JRUI_WIDGET_TYPE_STACK,
                .child_count = sizeof(STACK_CHILDREN) / sizeof(*STACK_CHILDREN),
                .children = STACK_CHILDREN,
        }
        };

jrui_widget_create_info SIM_AND_SOL_WINDOW =
        {
            .containerf =
                    {
                            .base_info.type = JRUI_WIDGET_TYPE_CONTAINERF,
                            .has_background = 0,
                            .width = 0.35f,
                            .height = 0.8f,
                            .align_vertical = JRUI_ALIGN_TOP,
                            .align_horizontal = JRUI_ALIGN_RIGHT,
                            .child = &WINDOW_STACK,
                            .base_info.label = "Info window",
                    }
        };
