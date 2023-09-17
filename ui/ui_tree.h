//
// Created by jan on 22.8.2023.
//

#ifndef JTA_UI_TREE_H
#define JTA_UI_TREE_H
#include "jrui.h"
#include "../gfx/vk_resources.h"
#include "../gfx/textures.h"
#include "../config/config_loading.h"

extern jrui_widget_create_info UI_ROOT;

gfx_result jta_ui_bind_font_texture(const jta_vulkan_window_context* ctx, jta_texture* texture);

void update_text_widget(jrui_context* ctx, const char* widget_label, const char* new_text);

#endif //JTA_UI_TREE_H
