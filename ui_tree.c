//
// Created by jan on 22.8.2023.
//

#include "ui_tree.h"
#include <jdm.h>
#include <stdio.h>

static void bnt1_callback(void* param)
{
    (void) param;
    printf("Hi 1\n");
}
static void bnt2_callback(void* param)
{
    (void) param;
    printf("Hi 2\n");
}

static const jrui_widget_create_info bottom_row_elements[] =
        {
            [0] = {.button = {.type = JRUI_WIDGET_TYPE_BUTTON, .btn_param = NULL, .btn_callback = bnt1_callback}},
            [1] = {.type = JRUI_WIDGET_TYPE_EMPTY},
            [2] = {.button = {.type = JRUI_WIDGET_TYPE_BUTTON, .btn_param = NULL, .btn_callback = bnt2_callback}},
        };

static const jrui_widget_create_info bottom_row =
        {
        .row =
                {
                .type = JRUI_WIDGET_TYPE_ROW,
                .child_count = sizeof(bottom_row_elements) / sizeof(*bottom_row_elements),
                .children = bottom_row_elements
                }
        };

jrui_widget_create_info UI_ROOT =
        {
        .container =
                {
                .type = JRUI_WIDGET_TYPE_CONTAINER,
                .height = 0,
                .width = 0,
                .has_background = 0,
                .align_vertical = JRUI_ALIGN_BOTTOM,
                .align_horizontal = JRUI_ALIGN_CENTER,
                .child = &bottom_row
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
