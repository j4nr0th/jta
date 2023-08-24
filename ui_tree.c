//
// Created by jan on 22.8.2023.
//

#include "ui_tree.h"
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
