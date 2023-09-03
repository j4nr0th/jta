//
// Created by jan on 22.8.2023.
//

#include "ui_tree.h"
#include "jdm.h"
#include <stdio.h>
#include "ui_sim_and_sol.h"

static const jrui_widget_create_info config_display_window_text =
        {
        .text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Config info"},
        };

static jrui_widget_create_info config_display_window_info_list_row_0[2] =
        {
        [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Elements:"}},
        [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .text = "PLACEHOLDER"}},
        };

static jrui_widget_create_info config_display_window_info_list_row_1[2] =
        {
                [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Materials:"}},
                [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .text = "PLACEHOLDER"}},
        };

static jrui_widget_create_info config_display_window_info_list_row_2[2] =
        {
                [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Profiles:"}},
                [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .text = "PLACEHOLDER"}},
        };

static jrui_widget_create_info config_display_window_info_list_row_3[2] =
        {
                [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Points:"}},
                [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .text = "PLACEHOLDER"}},
        };

static jrui_widget_create_info config_display_window_info_list_row_4[2] =
        {
                [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Numerical BCs:"}},
                [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .text = "PLACEHOLDER"}},
        };

static jrui_widget_create_info config_display_window_info_list_row_5[2] =
        {
                [0] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text = "Natural BCs:"}},
                [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .text = "PLACEHOLDER"}},
        };

static const jrui_widget_create_info config_display_window_info_list[] =
        {
        [0] = {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(config_display_window_info_list_row_0) / sizeof(*config_display_window_info_list_row_0), .children = config_display_window_info_list_row_0}},
        [1] = {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(config_display_window_info_list_row_1) / sizeof(*config_display_window_info_list_row_1), .children = config_display_window_info_list_row_1}},
        [2] = {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(config_display_window_info_list_row_2) / sizeof(*config_display_window_info_list_row_2), .children = config_display_window_info_list_row_2}},
        [3] = {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(config_display_window_info_list_row_3) / sizeof(*config_display_window_info_list_row_3), .children = config_display_window_info_list_row_3}},
        [4] = {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(config_display_window_info_list_row_4) / sizeof(*config_display_window_info_list_row_4), .children = config_display_window_info_list_row_4}},
        [5] = {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(config_display_window_info_list_row_5) / sizeof(*config_display_window_info_list_row_5), .children = config_display_window_info_list_row_5}},
        };

static const jrui_widget_create_info config_display_window_coulmn =
        {
        .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = sizeof(config_display_window_info_list) / sizeof(*config_display_window_info_list), config_display_window_info_list},
        };

static const jrui_widget_create_info config_display_window_elements[] =
        {
        [0] =
                {
                .rect = {.base_info.type = JRUI_WIDGET_TYPE_RECT, .color = {.r = 0x30, .g = 0x30, .b = 0x30, .a = 0xFF}},
                },
        [1] =
                {
                .containerf = {.base_info.type = JRUI_WIDGET_TYPE_CONTAINERF, .width = 1.0f, .height = 0.2f, .align_vertical = JRUI_ALIGN_TOP,
                              .child = &config_display_window_text}
                },
        [2] =
                {
                .containerf = {.base_info.type = JRUI_WIDGET_TYPE_CONTAINERF, .width = 1.0f, .height = 0.8f, .align_vertical = JRUI_ALIGN_BOTTOM,
                        .child = &config_display_window_coulmn}
                },
        };

static const jrui_widget_create_info config_display_window_stack =
        {
        .stack =
                {
                .base_info.type = JRUI_WIDGET_TYPE_STACK,
                .child_count = sizeof(config_display_window_elements) / sizeof(*config_display_window_elements),
                .children = config_display_window_elements,
                }
        };

static const jrui_widget_create_info config_display_window_base =
        {
        .containerf =
                {
                .base_info.type = JRUI_WIDGET_TYPE_CONTAINERF,
                .has_background = 0,
                .width = 0.25f,
                .height = 0.8f,
                .align_vertical = JRUI_ALIGN_TOP,
                .align_horizontal = JRUI_ALIGN_RIGHT,
                .child = &config_display_window_stack,
                .base_info.label = "Info window",
                }
        };

static void bnt1_callback(jrui_widget_base* widget, void* param)
{
    JDM_ENTER_FUNCTION;

    char buffer0[128] = {0};
    snprintf(buffer0, sizeof(buffer0), "%s", p_cfg->problem.definition.elements_file);
    char buffer1[128] = {0};
    snprintf(buffer1, sizeof(buffer1), "%s", p_cfg->problem.definition.materials_file);
    char buffer2[128] = {0};
    snprintf(buffer2, sizeof(buffer2), "%s", p_cfg->problem.definition.profiles_file);
    char buffer3[128] = {0};
    snprintf(buffer3, sizeof(buffer3), "%s", p_cfg->problem.definition.points_file);
    char buffer4[128] = {0};
    snprintf(buffer4, sizeof(buffer4), "%s", p_cfg->problem.definition.numerical_bcs_file);
    char buffer5[128] = {0};
    snprintf(buffer5, sizeof(buffer5), "%s", p_cfg->problem.definition.natural_bcs_file);
    config_display_window_info_list_row_0[1].text_h.text = buffer0;
    config_display_window_info_list_row_1[1].text_h.text = buffer1;
    config_display_window_info_list_row_2[1].text_h.text = buffer2;
    config_display_window_info_list_row_3[1].text_h.text = buffer3;
    config_display_window_info_list_row_4[1].text_h.text = buffer4;
    config_display_window_info_list_row_5[1].text_h.text = buffer5;

    jrui_widget_base* p_to_replace = jrui_get_by_label(jrui_widget_get_context(widget), "Info window");
    if (!p_to_replace)
    {
        JDM_ERROR("Could not find \"Info window\" widget");
        JDM_LEAVE_FUNCTION;
        return;
    }
    const jrui_result res = jrui_replace_widget(p_to_replace, config_display_window_base);

    if (res != JRUI_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not replace the \"Info window\" widget, reason: %s (%s)", jrui_result_to_str(res), jrui_result_message(res));
    }

    config_display_window_info_list_row_0[1].text_h.text = NULL;
    config_display_window_info_list_row_1[1].text_h.text = NULL;
    config_display_window_info_list_row_2[1].text_h.text = NULL;
    config_display_window_info_list_row_3[1].text_h.text = NULL;
    config_display_window_info_list_row_4[1].text_h.text = NULL;
    config_display_window_info_list_row_5[1].text_h.text = NULL;

    JDM_LEAVE_FUNCTION;
}

void update_text_widget(jrui_context* ctx, const char* widget_label, const char* new_text)
{
    jrui_widget_base* p_search = jrui_get_by_label(ctx, widget_label);
    if (!p_search)
    {
        JDM_ERROR("Could not find \"%s\" widget", widget_label);
    }
    else
    {
        jrui_text_h_create_info create_info =
                {
                        .base_info.type = JRUI_WIDGET_TYPE_TEXT_H,
                        .base_info.label = widget_label,
                        .text = new_text,
                };
        jrui_result res = jrui_update_text_h(p_search, &create_info);
        if (res != JRUI_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not update \"%s\" widget, reason: %s (%s)", widget_label, jrui_result_to_str(res), jrui_result_message(res));
        }
    }
}

static void bnt2_callback(jrui_widget_base* widget, void* param)
{
    JDM_ENTER_FUNCTION;


    jrui_widget_base* p_to_replace = jrui_get_by_label(jrui_widget_get_context(widget), "Info window");
    if (!p_to_replace)
    {
        JDM_ERROR("Could not find \"Info window\" widget");
        JDM_LEAVE_FUNCTION;
        return;
    }
    jrui_result res;
    res = jrui_replace_widget(p_to_replace, SIM_AND_SOL_WINDOW);

    if (res != JRUI_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not replace the \"Info window\" widget, reason: %s (%s)", jrui_result_to_str(res), jrui_result_message(res));
    }

    char buffer[64];

    snprintf(buffer, sizeof(buffer), "%g", p_cfg->problem.sim_and_sol.gravity[0]);
    update_text_widget(jrui_widget_get_context(widget), "gravity x", buffer);
    snprintf(buffer, sizeof(buffer), "%g", p_cfg->problem.sim_and_sol.gravity[1]);
    update_text_widget(jrui_widget_get_context(widget), "gravity y", buffer);
    snprintf(buffer, sizeof(buffer), "%g", p_cfg->problem.sim_and_sol.gravity[2]);
    update_text_widget(jrui_widget_get_context(widget), "gravity z", buffer);

    snprintf(buffer, sizeof(buffer), "%u", p_cfg->problem.sim_and_sol.max_iterations);
    update_text_widget(jrui_widget_get_context(widget), "iteration count", buffer);

    snprintf(buffer, sizeof(buffer), "%g", p_cfg->problem.sim_and_sol.relaxation_factor);
    update_text_widget(jrui_widget_get_context(widget), "relax factor", buffer);

    snprintf(buffer, sizeof(buffer), "%g", p_cfg->problem.sim_and_sol.convergence_criterion);
    update_text_widget(jrui_widget_get_context(widget), "convergence text", buffer);

    snprintf(buffer, sizeof(buffer), "%g", p_cfg->problem.sim_and_sol.relaxation_factor);
    update_text_widget(jrui_widget_get_context(widget), "relax factor", buffer);

    JDM_LEAVE_FUNCTION;
}

static const jrui_widget_create_info left_stack_children[2] =
        {
            [0] = {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_param = NULL, .btn_callback = bnt1_callback}},
            [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Show config info"}}
        };

static const jrui_widget_create_info right_stack_children[2] =
        {
                [0] = { .button = { .base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_param = NULL, .btn_callback = bnt2_callback }},
                [1] = {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Button 2"}}
        };

static const jrui_widget_create_info bottom_row_elements[] =
        {
            [0] = {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = sizeof(left_stack_children) / sizeof(*left_stack_children), .children = left_stack_children}},
            [1] = {.type = JRUI_WIDGET_TYPE_EMPTY},
            [2] = {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = sizeof(right_stack_children) / sizeof(*right_stack_children), .children = right_stack_children}},
        };

static const jrui_widget_create_info bottom_row =
        {
        .row =
                {
                .base_info.type = JRUI_WIDGET_TYPE_ROW,
                .child_count = sizeof(bottom_row_elements) / sizeof(*bottom_row_elements),
                .children = bottom_row_elements
                }
        };

jrui_widget_create_info UI_ROOT_CHILDREN[] =
        {
                [0] = {.containerf =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_CONTAINERF,
                                .height = 0.1f,
                                .width = 1.0f,
                                .has_background = 0,
                                .align_vertical = JRUI_ALIGN_BOTTOM,
                                .align_horizontal = JRUI_ALIGN_CENTER,
                                .child = &bottom_row
                        }},

                [1] = {.base_info = {.type = JRUI_WIDGET_TYPE_EMPTY, .label = "Info window"}}
        };

jrui_widget_create_info UI_ROOT =
        {
        .stack =
                {
                .base_info.type = JRUI_WIDGET_TYPE_STACK,
                .children = UI_ROOT_CHILDREN,
                .child_count = sizeof(UI_ROOT_CHILDREN) / sizeof(*UI_ROOT_CHILDREN),
                }
        };

gfx_result jta_ui_bind_font_texture(const jta_vulkan_window_context* ctx, jta_texture* texture)
{
    JDM_ENTER_FUNCTION;

    VkDescriptorImageInfo info =
            {
            .sampler = texture->sampler,
            .imageView = texture->view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

    VkWriteDescriptorSet write_set =
            {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = ctx->descriptor_ui,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &info,
            };

    vkUpdateDescriptorSets(ctx->device, 1, &write_set, 0, NULL);

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}
