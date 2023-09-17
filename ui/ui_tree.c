//
// Created by jan on 22.8.2023.
//

#include "ui_tree.h"
#include "jdm.h"
#include <stdio.h>
#include "ui_sim_and_sol.h"
#include "../jta_state.h"
#include "top_menu.h"

static jrui_widget_create_info UI_CHILDREN[2] =
        {
                { .type = JRUI_WIDGET_TYPE_COUNT + 345678 },    //  Placeholder to be swapped out with top menu at runtime
                { .base_info = {.type = JRUI_WIDGET_TYPE_EMPTY, .label = "body"}},
        };

static const float UI_CHILD_PROPORTIONS[2] =
        {
        1.0f, 19.0f,
        };
static_assert(sizeof(UI_CHILDREN) / sizeof(*UI_CHILDREN) == sizeof(UI_CHILD_PROPORTIONS) / sizeof(*UI_CHILD_PROPORTIONS));

jrui_widget_create_info UI_ROOT =
        {
        .adjustable_column =
                {
                .base_info = {.type = JRUI_WIDGET_TYPE_ADJCOL, .label = "ROOT"},
                .child_count = sizeof(UI_CHILDREN) / sizeof(*UI_CHILDREN),
                .children = UI_CHILDREN,
                .proportions = UI_CHILD_PROPORTIONS,
                }
        };

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

void jta_ui_init(jta_state* state)
{
    (void) state;
    UI_CHILDREN[0] = TOP_MENU_TREE;
}
