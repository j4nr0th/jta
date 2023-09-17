//
// Created by jan on 17.9.2023.
//

#include "top_menu.h"
#include <stdlib.h>
#include <jdm.h>

enum top_menu_button_T
{
    TOP_MENU_PROBLEM,
    TOP_MENU_DISPLAY,
    TOP_MENU_OUTPUT,
    TOP_MENU_MESH,
    TOP_MENU_SOLVE,
    TOP_MENU_COUNT,
};
typedef enum top_menu_button_T top_menu_button;

static const char* const TOP_MENU_LABELS[TOP_MENU_COUNT] =
        {
                [TOP_MENU_PROBLEM] = "top menu:problem",
                [TOP_MENU_DISPLAY] = "top menu:display",
                [TOP_MENU_OUTPUT] = "top menu:output",
                [TOP_MENU_MESH] = "top menu:mesh",
                [TOP_MENU_SOLVE] = "top menu:solve",
        };

static void submit_point_file(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_material_file(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_profile_file(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_natbc_file(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_numbc_file(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_element_file(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static jrui_result top_menu_problem_replace(jrui_context* ctx, jrui_widget_base* body)
{
    jrui_widget_create_info point_children[] =
            {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Point file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Not found", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:point status"}},

            };
    jrui_widget_create_info material_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Material file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Not found", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:material status"}},

            };
    jrui_widget_create_info profile_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Profile file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Not found", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:profile status"}},

            };
    jrui_widget_create_info natural_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Natural BC file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Not found", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:natural BC status"}},

            };
    jrui_widget_create_info numerical_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Numerical BC file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Not found", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:numerical BC status"}},

            };
    jrui_widget_create_info element_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Element file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Not found", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:element status"}},

            };
    jrui_widget_create_info col_entry_children[] =
            {
                    //  Point data
                    {
                        .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(point_children) / sizeof(*point_children), .children = point_children,},
                    },
                    {
                        .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_point_file},
                    },
                    //  Material data
                    {
                        .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(material_children) / sizeof(*material_children), .children = material_children,},
                    },
                    {
                        .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_material_file},
                    },
                    //  Profile data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(profile_children) / sizeof(*profile_children), .children = profile_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_profile_file},
                    },
                    //  Natural BCs data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(natural_children) / sizeof(*natural_children), .children = natural_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_natbc_file},
                    },
                    //  Numerical BCs data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(numerical_children) / sizeof(*numerical_children), .children = numerical_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_numbc_file},
                    },
                    //  Element data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(element_children) / sizeof(*element_children), .children = element_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_element_file},
                    },
            };
    jrui_widget_create_info col_section =
            {
            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .base_info.label = "body", .child_count = sizeof(col_entry_children) / sizeof(*col_entry_children), .children = col_entry_children},
            };

    return jrui_replace_widget(body, col_section);
}


static void submit_radius_scale(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_deformation_scale(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_deformation_color(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_DoF_color_0(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_DoF_color_1(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_DoF_color_2(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_DoF_color_3(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_DoF_scale(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static jrui_result top_menu_display_replace(jrui_context* ctx, jrui_widget_base* body)
{
    //  Radius scale
    jrui_widget_create_info radius_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Radius scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:radius", .submit_callback = submit_radius_scale}},
            };
    jrui_widget_create_info radius_row =
            {
            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(radius_elements) / sizeof(*radius_elements), .children = radius_elements},
            };
    jrui_widget_create_info radius_border =
            {
            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &radius_row},
            };


    //  Deformation scale
    jrui_widget_create_info deformation_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Deformation scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation scale", .submit_callback = submit_deformation_scale}},
            };
    jrui_widget_create_info deformation_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(deformation_elements) / sizeof(*deformation_elements), .children = deformation_elements},
            };
    jrui_widget_create_info deformation_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &deformation_row},
            };


    //  Deformation color
    jrui_widget_create_info deformation_color_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 0", .submit_callback = submit_deformation_color, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 1", .submit_callback = submit_deformation_color, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 2", .submit_callback = submit_deformation_color, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 3", .submit_callback = submit_deformation_color, .submit_param = (void*)3}},
            };
    jrui_widget_create_info deformation_color_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Deformation color", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(deformation_color_components) / sizeof(*deformation_color_components), .children = deformation_color_components}},
            };
    jrui_widget_create_info deformation_color_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(deformation_color_elements) / sizeof(*deformation_color_elements), .children = deformation_color_elements},
            };
    jrui_widget_create_info deformation_color_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &deformation_color_row},
            };

    //  Dof 0 color
    jrui_widget_create_info color_dof0_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 0", .submit_callback = submit_DoF_color_0, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 1", .submit_callback = submit_DoF_color_0, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 2", .submit_callback = submit_DoF_color_0, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 3", .submit_callback = submit_DoF_color_0, .submit_param = (void*)3}},
            };
    jrui_widget_create_info color_dof0_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 0 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof0_components) / sizeof(*color_dof0_components), .children = color_dof0_components}},
            };
    jrui_widget_create_info color_dof0_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof0_elements) / sizeof(*color_dof0_elements), .children = color_dof0_elements},
            };
    jrui_widget_create_info color_dof0_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof0_row},
            };

    
    //  Dof 1 color
    jrui_widget_create_info color_dof1_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 0", .submit_callback = submit_DoF_color_1, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 1", .submit_callback = submit_DoF_color_1, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 2", .submit_callback = submit_DoF_color_1, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 3", .submit_callback = submit_DoF_color_1, .submit_param = (void*)3}},
            };
    jrui_widget_create_info color_dof1_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 1 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof1_components) / sizeof(*color_dof1_components), .children = color_dof1_components}},
            };
    jrui_widget_create_info color_dof1_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof1_elements) / sizeof(*color_dof1_elements), .children = color_dof1_elements},
            };
    jrui_widget_create_info color_dof1_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof1_row},
            };


    //  Dof 2 color
    jrui_widget_create_info color_dof2_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 0", .submit_callback = submit_DoF_color_2, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 1", .submit_callback = submit_DoF_color_2, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 2", .submit_callback = submit_DoF_color_2, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 3", .submit_callback = submit_DoF_color_2, .submit_param = (void*)3}},
            };
    jrui_widget_create_info color_dof2_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 2 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof2_components) / sizeof(*color_dof2_components), .children = color_dof2_components}},
            };
    jrui_widget_create_info color_dof2_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof2_elements) / sizeof(*color_dof2_elements), .children = color_dof2_elements},
            };
    jrui_widget_create_info color_dof2_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof2_row},
            };


    //  Dof 3 color
    jrui_widget_create_info color_dof3_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 0", .submit_callback = submit_DoF_color_3, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 1", .submit_callback = submit_DoF_color_3, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 2", .submit_callback = submit_DoF_color_3, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 3", .submit_callback = submit_DoF_color_3, .submit_param = (void*)3}},
            };
    jrui_widget_create_info color_dof3_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 3 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof3_components) / sizeof(*color_dof3_components), .children = color_dof3_components}},
            };
    jrui_widget_create_info color_dof3_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof3_elements) / sizeof(*color_dof3_elements), .children = color_dof3_elements},
            };
    jrui_widget_create_info color_dof3_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof3_row},
            };


    //  Dof scale
    jrui_widget_create_info dof_scale_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 0", .submit_callback = submit_DoF_scale, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 1", .submit_callback = submit_DoF_scale, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 2", .submit_callback = submit_DoF_scale, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 3", .submit_callback = submit_DoF_scale, .submit_param = (void*)3}},
            };
    jrui_widget_create_info dof_scale_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF scales:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(dof_scale_components) / sizeof(*dof_scale_components), .children = dof_scale_components}},
            };
    jrui_widget_create_info dof_scale_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(dof_scale_elements) / sizeof(*dof_scale_elements), .children = dof_scale_elements},
            };
    jrui_widget_create_info dof_scale_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &dof_scale_row},
            };


    //  Force radius ratio
    jrui_widget_create_info force_radius_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Force radius scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:force radius", .submit_callback = submit_radius_scale}},
            };
    jrui_widget_create_info force_radius_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(force_radius_elements) / sizeof(*force_radius_elements), .children = force_radius_elements},
            };
    jrui_widget_create_info force_radius_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &force_radius_row},
            };


    //  Force head ratio
    jrui_widget_create_info force_head_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Force head scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:force head", .submit_callback = submit_radius_scale}},
            };
    jrui_widget_create_info force_head_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(force_head_elements) / sizeof(*force_head_elements), .children = force_head_elements},
            };
    jrui_widget_create_info force_head_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &force_head_row},
            };


    //  Force length ratio
    jrui_widget_create_info force_length_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Force length scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:force length", .submit_callback = submit_radius_scale}},
            };
    jrui_widget_create_info force_length_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(force_length_elements) / sizeof(*force_length_elements), .children = force_length_elements},
            };
    jrui_widget_create_info force_length_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &force_length_row},
            };

    jrui_widget_create_info body_entries[] =
            {
            radius_border, deformation_border, deformation_color_border, color_dof0_border, color_dof1_border, color_dof2_border, color_dof3_border, dof_scale_border, force_radius_border, force_head_border, force_length_border
            };

    jrui_widget_create_info replacement_body =
            {
            .column = {.base_info = {.type = JRUI_WIDGET_TYPE_COLUMN, .label = "body"}, .child_count = sizeof(body_entries) / sizeof(*body_entries), .children = body_entries}
            };
    return jrui_replace_widget(body, replacement_body);
}


static void submit_point_output(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_element_output(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_general_output(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_matrix_output(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_figure_output(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static jrui_result top_menu_output_replace(jrui_context* ctx, jrui_widget_base* body)
{
    //  Point outputs
    jrui_widget_create_info point_children[2] =
            {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Point output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:point"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_point_output}},
            };
    jrui_widget_create_info point_row =
            {
            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(point_children) / sizeof(*point_children), .children = point_children},
            };
    jrui_widget_create_info border_point =
            {
            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &point_row}
            };
    
    //  Element outputs
    jrui_widget_create_info element_children[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Element output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:element"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_element_output}},
            };
    jrui_widget_create_info element_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(element_children) / sizeof(*element_children), .children = element_children},
            };
    jrui_widget_create_info border_element =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &element_row}
            };

    //  General outputs
    jrui_widget_create_info general_children[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "General output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:general"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_general_output}},
            };
    jrui_widget_create_info general_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(general_children) / sizeof(*general_children), .children = general_children},
            };
    jrui_widget_create_info border_general =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &general_row}
            };

    //  Matrix outputs
    jrui_widget_create_info matrix_children[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Matrix output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:matrix"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_matrix_output}},
            };
    jrui_widget_create_info matrix_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(matrix_children) / sizeof(*matrix_children), .children = matrix_children},
            };
    jrui_widget_create_info border_matrix =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &matrix_row}
            };

    //  Figure outputs
    jrui_widget_create_info figure_children[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Figure output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:figure"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_figure_output}},
            };
    jrui_widget_create_info figure_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(figure_children) / sizeof(*figure_children), .children = figure_children},
            };
    jrui_widget_create_info border_figure =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &figure_row}
            };

    jrui_widget_create_info contents[] =
            {
            border_point, border_element, border_general, border_matrix, border_figure
            };

    jrui_widget_create_info replacement_body =
            {
            .column = {.base_info = {.type = JRUI_WIDGET_TYPE_COLUMN, .label = "body"}, .child_count = sizeof(contents) / sizeof(*contents), .children = contents},
            };

    return jrui_replace_widget(body, replacement_body);
}


static void submit_gravity(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_threads(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_convergence(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_convergence_drag(jrui_widget_base* widget, float v, void* param)
{
    // TODO: implement this
}

static void submit_iterations(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_relaxation(jrui_widget_base* widget, const char* string, void* param)
{
    // TODO: implement this
}

static void submit_relaxation_drag(jrui_widget_base* widget, float v, void* param)
{
    // TODO: implement this
}

static jrui_result top_menu_solve_replace(jrui_context* ctx, jrui_widget_base* body)
{
    //  gravity
    jrui_widget_create_info gravity_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 0", .submit_callback = submit_gravity, .submit_param = (void*)0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 1", .submit_callback = submit_gravity, .submit_param = (void*)1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 2", .submit_callback = submit_gravity, .submit_param = (void*)2}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 3", .submit_callback = submit_gravity, .submit_param = (void*)3}},
            };
    jrui_widget_create_info gravity_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Gravity:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(gravity_components) / sizeof(*gravity_components), .children = gravity_components}},
            };
    jrui_widget_create_info gravity_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(gravity_elements) / sizeof(*gravity_elements), .children = gravity_elements},
            };
    jrui_widget_create_info border_gravity =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &gravity_row},
            };
    
    //  threads
    jrui_widget_create_info threads_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Thread count:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:threads", .submit_callback = submit_threads}},
            };
    jrui_widget_create_info threads_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(threads_elements) / sizeof(*threads_elements), .children = threads_elements},
            };
    jrui_widget_create_info border_threads =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &threads_row},
            };

    //  convergence
    jrui_widget_create_info convergence_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Max residual:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:convergence", .submit_callback = submit_convergence}},
            };
    jrui_widget_create_info convergence_rows[] =
            {
                    { .row = { .base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(convergence_elements) /
                                                                                      sizeof(*convergence_elements), .children = convergence_elements }},
                    { .drag_h = {.base_info = {.type = JRUI_WIDGET_TYPE_DRAG_H, .label = "solve:drag convergence"}, .callback = submit_convergence_drag}},
            };
    jrui_widget_create_info convergence_col =
            {
                    .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = sizeof(convergence_rows) / sizeof(*convergence_rows), .children = convergence_rows},
            };
    jrui_widget_create_info border_convergence =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &convergence_col},
            };

    //  iterations
    jrui_widget_create_info iterations_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Thread count:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:iterations", .submit_callback = submit_iterations}},
            };
    jrui_widget_create_info iterations_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(iterations_elements) / sizeof(*iterations_elements), .children = iterations_elements},
            };
    jrui_widget_create_info border_iterations =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &iterations_row},
            };

    //  relaxation
    jrui_widget_create_info relaxation_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Relaxation factor:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:relaxation", .submit_callback = submit_relaxation}},
            };
    jrui_widget_create_info relaxation_rows[] =
            {
                    { .row = { .base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(relaxation_elements) /
                                                                                      sizeof(*relaxation_elements), .children = relaxation_elements }},
                    { .drag_h = {.base_info = {.type = JRUI_WIDGET_TYPE_DRAG_H, .label = "solve:drag relaxation"}, .callback = submit_relaxation_drag}},
            };
    jrui_widget_create_info relaxation_col =
            {
                    .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = sizeof(relaxation_rows) / sizeof(*relaxation_rows), .children = relaxation_rows},
            };
    jrui_widget_create_info border_relaxation =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &relaxation_col},
            };

    jrui_widget_create_info body_contents[] =
            {
            border_gravity, border_threads, border_convergence, border_iterations, border_relaxation
            };

    float body_proportions[] =
            {
            1.0f, 1.0f, 2.0f, 1.0f, 2.0f,
            };
    static_assert(sizeof(body_proportions) / sizeof(*body_proportions) == sizeof(body_contents) / sizeof(*body_contents));
    jrui_widget_create_info replacement_body =
            {
            .adjustable_column = {.base_info = {.type = JRUI_WIDGET_TYPE_ADJCOL, .label = "body"}, .child_count = sizeof(body_contents) / sizeof(*body_contents), .children = body_contents, .proportions = body_proportions}
            };
    return jrui_replace_widget(body, replacement_body);
}

static jrui_result top_menu_mesh_replace(jrui_context* ctx, jrui_widget_base* body)
{

    jrui_widget_create_info replacement_body =
            {
            .base_info = {.type = JRUI_WIDGET_TYPE_EMPTY, .label = "body"}
            };
    return jrui_replace_widget(body, replacement_body);
}

static void top_menu_toggle_on_callback(jrui_widget_base* widget, int pressed, void* param)
{
    const top_menu_button button = (top_menu_button)(uintptr_t)param;
    if (!pressed || button < TOP_MENU_PROBLEM || button >= TOP_MENU_COUNT)
    {
        return;
    }
    JDM_ENTER_FUNCTION;
    jrui_context* ctx = jrui_widget_get_context(widget);
    //  Set other button's states to zero
    for (top_menu_button btn = 0; btn < TOP_MENU_COUNT; ++btn)
    {
        if (button != btn)
        {
            jrui_update_toggle_button_state_by_label(ctx, TOP_MENU_LABELS[btn], 0);
        }
    }
    
    // Replace the body of the UI with what the actual menu entry wants
    jrui_widget_base* const body = jrui_get_by_label(ctx, "body");
    jrui_result replace_res;
    switch (button)
    {
    case TOP_MENU_PROBLEM:
        replace_res = top_menu_problem_replace(ctx, body);
        break;
    case TOP_MENU_DISPLAY:
        replace_res = top_menu_display_replace(ctx, body);
        break;
    case TOP_MENU_OUTPUT:
        replace_res = top_menu_output_replace(ctx, body);
        break;
    case TOP_MENU_SOLVE:
        replace_res = top_menu_solve_replace(ctx, body);
        break;
    case TOP_MENU_MESH:
        replace_res = top_menu_mesh_replace(ctx, body);
        break;
    default:assert(0);
    }
    if (replace_res != JRUI_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not replace UI body widget, reason: %s (%s)", jrui_result_to_str(replace_res), jrui_result_message(replace_res));
    }
    JDM_LEAVE_FUNCTION;
}

static void close_callback(jrui_widget_base* widget, void* param) { exit(EXIT_SUCCESS); }
static const jrui_widget_create_info TOP_MENU_BUTTONS[2 * (TOP_MENU_COUNT + 1)] =
        {
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:problem"}, 
                        .toggle_param = (void*)TOP_MENU_PROBLEM, 
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                    {
                        .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                        .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Problem"
                    }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:display"},
                        .toggle_param = (void*)TOP_MENU_DISPLAY,
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Display"
                        }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:output"},
                        .toggle_param = (void*)TOP_MENU_OUTPUT,
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Output"
                        }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:mesh"},
                        .toggle_param = (void*)TOP_MENU_MESH, 
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Mesh"
                        }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:solve"},
                        .toggle_param = (void*)TOP_MENU_SOLVE, 
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Solve"
                        }
                },
                {.button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_BUTTON, .label = "top menu:close"}, 
                        .btn_callback = close_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Close"
                        }
                },
        };
static const jrui_widget_create_info TOP_MENU_BUTTON_STACKS[TOP_MENU_COUNT + 1] =
        {
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 0)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 1)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 2)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 3)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 4)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 5)}},
        };

const jrui_widget_create_info TOP_MENU_TREE =
        {
        .row = {.base_info = {.label = "top menu", .type = JRUI_WIDGET_TYPE_ROW}, .children = TOP_MENU_BUTTON_STACKS, .child_count = TOP_MENU_COUNT + 1},
        };
