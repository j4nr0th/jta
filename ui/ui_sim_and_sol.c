//
// Created by jan on 3.9.2023.
//

#include <jdm.h>
#include <ctype.h>
#include "ui_sim_and_sol.h"
#include "ui_tree.h"

//static jrui_widget_create_info GRAVITY_ROW[] =
//        {
//        [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "gravity x"}},
//        [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "gravity y"}},
//        [2] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "gravity z"}},
//        };

static void text_input_gravity_button(jrui_widget_base* const widget, const char* const text, void* param)
{
    JDM_ENTER_FUNCTION;
    float* const p_value = param;
    //  Try and convert text to value
    char* end = NULL;
    const double v = strtod(text, &end);
    if (end != text)
    {
        int found_nonspace = 0;
        while (*end && end != text)
        {
            if (!isspace(*end))
            {
                JDM_WARN("Expected a single float, but received \"%s\"", text);
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "%g", *p_value);
                (void)jrui_update_text_input_text(widget, buffer);
                JDM_LEAVE_FUNCTION;
                return;
            }
        }
    }
    *p_value = (float)v;
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%g", v);
//    (void)jrui_update_text_input_text(widget, buffer);
    (void)jrui_update_text_input_hint(widget, buffer);
    JDM_LEAVE_FUNCTION;
}

static jrui_widget_create_info GRAVITY_ROW[] =
        {
                [0] = {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .base_info.label = "gravity x", .submit_callback = text_input_gravity_button}},
                [1] = {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .base_info.label = "gravity y", .submit_callback = text_input_gravity_button}},
                [2] = {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .base_info.label = "gravity z", .submit_callback = text_input_gravity_button}},
        };


static void drag_convergence_callback(jrui_widget_base* drag, float f, void* param)
{
    JDM_ENTER_FUNCTION;
    jrui_widget_base* converge_text = jrui_get_by_label(jrui_widget_get_context(drag), "convergence text");
    if (!converge_text)
    {
        JDM_ERROR("Could not get the widget with label \"convergence text\"");
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
        jrui_result res = jrui_update_text_h(converge_text, &updated_info);
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


static const jrui_widget_create_info GRAVITY_SECTION_ENTRIES[] =
        {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Gravity"}},
                {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(GRAVITY_ROW) / sizeof(*GRAVITY_ROW), .children = GRAVITY_ROW}},
        };

static const jrui_widget_create_info GRAVITY_SECTION_COLUMN =
        {
                .column = {.base_info = {.type = JRUI_WIDGET_TYPE_COLUMN}, .child_count = sizeof(GRAVITY_SECTION_ENTRIES) / sizeof(*GRAVITY_SECTION_ENTRIES), .children = GRAVITY_SECTION_ENTRIES},
        };

static const jrui_widget_create_info CONVERGENCE_SECTION_ENTRIES[] =
        {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Convergence criterion"}},
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "convergence text"}},
                {.drag_h = {.base_info.type = JRUI_WIDGET_TYPE_DRAG_H, .manual_changes = 0.05f, .btn_ratio = 0.05f, .initial_pos = 0.0f, .param = NULL, .callback = drag_convergence_callback}},
        };

static const jrui_widget_create_info CONVERGENCE_SECTION_COLUMN =
        {
                .column = {.base_info = {.type = JRUI_WIDGET_TYPE_COLUMN}, .child_count = sizeof(CONVERGENCE_SECTION_ENTRIES) / sizeof(*CONVERGENCE_SECTION_ENTRIES), .children = CONVERGENCE_SECTION_ENTRIES},
        };

static const jrui_widget_create_info ITERATION_SECTION_ENTRIES[] =
        {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Max Iterations"}},
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "iteration count"}},
        };

static const jrui_widget_create_info ITERATION_SECTION_COLUMN =
        {
                .column = {.base_info = {.type = JRUI_WIDGET_TYPE_COLUMN}, .child_count = sizeof(ITERATION_SECTION_ENTRIES) / sizeof(*ITERATION_SECTION_ENTRIES), .children = ITERATION_SECTION_ENTRIES},
        };

static const jrui_widget_create_info RELAXIATION_SECTION_ENTRIES[] =
        {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Relaxation factor"}},
                {.drag_h = {.base_info.type = JRUI_WIDGET_TYPE_DRAG_H, .manual_changes = 0.05f, .btn_ratio = 0.05f, .initial_pos = 0.0f, .param = NULL, .callback = drag_relax_callback}},
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .base_info.label = "relax factor"}},
        };

static const jrui_widget_create_info RELAXIATION_SECTION_COLUMN =
        {
                .column = {.base_info = {.type = JRUI_WIDGET_TYPE_COLUMN}, .child_count = sizeof(RELAXIATION_SECTION_ENTRIES) / sizeof(*RELAXIATION_SECTION_ENTRIES), .children = RELAXIATION_SECTION_ENTRIES},
        };

static const jrui_widget_create_info WINDOW_ENTRIES_BORDERS[] =
        {
        {.border = {.border_thickness = 1.0f, .child = &GRAVITY_SECTION_COLUMN, .base_info = {.type = JRUI_WIDGET_TYPE_BORDER}}},
        {.border = {.border_thickness = 1.0f, .child = &CONVERGENCE_SECTION_COLUMN, .base_info = {.type = JRUI_WIDGET_TYPE_BORDER}}},
        {.border = {.border_thickness = 1.0f, .child = &ITERATION_SECTION_COLUMN, .base_info = {.type = JRUI_WIDGET_TYPE_BORDER}}},
        {.border = {.border_thickness = 1.0f, .child = &RELAXIATION_SECTION_COLUMN, .base_info = {.type = JRUI_WIDGET_TYPE_BORDER}}},
        };

static const float WINDOW_ENTRIES_SIZES[] =
        {
        2.0f,
        3.0f,
        2.0f,
        3.0f,
        };

static_assert(sizeof(WINDOW_ENTRIES_SIZES) / sizeof(*WINDOW_ENTRIES_SIZES) == sizeof(WINDOW_ENTRIES_BORDERS) / sizeof(*WINDOW_ENTRIES_BORDERS), "Entries and sizes arrays must contain the exact same number of elements!");

static const jrui_widget_create_info WINDOW_ENTRIES_COLUMN =
        {
        .adjustable_column = {.base_info.type = JRUI_WIDGET_TYPE_ADJCOL, .child_count = sizeof(WINDOW_ENTRIES_BORDERS) / sizeof(*WINDOW_ENTRIES_BORDERS), .children = WINDOW_ENTRIES_BORDERS, .proportions = WINDOW_ENTRIES_SIZES},
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
