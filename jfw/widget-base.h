//
// Created by jan on 2.2.2023.
//

#ifndef JFW_WIDGET_BASE_H
#define JFW_WIDGET_BASE_H
#include "jfw_common.h"
#include "error_system/error_stack.h"
#include "window.h"

jfw_res jfw_widget_functions_set(jfw_widget* widget);

typedef struct jfw_widget_functions_struct
{
    //  Related to parent's state
    jfw_res (*parent_resized)(jfw_widget* this, jfw_widget* parent, u32 old_w, u32 old_h, u32 new_w, u32 new_h);
    jfw_res (*parent_destroyed)(jfw_widget* this, jfw_widget* parent);  // Not sure if needed, since this means death
    jfw_res (*parent_redrawn)(jfw_widget* this, jfw_widget* parent);
//    jfw_res (*parent_visibility_change)(jfw_widget* this, jfw_widget* parent, int visible);
    //  Related to children's states
    jfw_res (*child_resized)(jfw_widget* this, jfw_widget* child, u32 old_w, u32 old_h, u32 new_w, u32 new_h);
    jfw_res (*child_moved)(jfw_widget* this, jfw_widget* child, i32 old_x, i32 old_y, i32 new_x, i32 new_y);
    jfw_res (*child_destroyed)(jfw_widget* this, jfw_widget* child);
    jfw_res (*child_created)(jfw_widget* this, jfw_widget* child);
    jfw_res (*child_redrawn)(jfw_widget* this, jfw_widget* child);
    //  Related to mouse input
    jfw_res (*mouse_button_press)(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);
    jfw_res (*mouse_button_release)(jfw_widget* this, i32 x, i32 y, u32 button, u32 mods);
    jfw_res (*mouse_motion)(jfw_widget* this, i32 x, i32 y, u32 mods);
    jfw_res (*mouse_drag)(jfw_widget* this, i32 x, i32 y, u32 button_mask, u32 mods);
    jfw_res (*mouse_focus_get)(jfw_widget* this, jfw_widget* new_focus);
    jfw_res (*mouse_focus_lose)(jfw_widget* this, jfw_widget* old_focus);
    //  Related to keyboard input
    jfw_res (*button_down)(jfw_widget* this, u32 key_code);
    jfw_res (*button_up)(jfw_widget* this, u32 key_code);
    jfw_res (*char_input)(jfw_widget* this, const char* utf8);
    jfw_res (*keyboard_focus_get)(jfw_widget* this, jfw_widget* new_focus);
    jfw_res (*keyboard_focus_lose)(jfw_widget* this, jfw_widget* old_focus);
    //  Related to window changes
    jfw_res (*window_focused)(jfw_widget* this, jfw_window* window);
    jfw_res (*window_unfocused)(jfw_widget* this, jfw_window* window);
    //  Related to drawing
    jfw_res (*drawing_predraw)(jfw_widget* this);
    jfw_res (*drawing_postdraw)(jfw_widget* this);
} jfw_widget_functions;

struct jfw_widget_struct
{
    jfw_res (*draw_fn)(jfw_widget* this);
    jfw_res (*dtor_fn)(jfw_widget* this);
    jfw_widget_functions functions; //  functions which handle how a widget responds to different changes
    jfw_widget* parent;
    jfw_window* window;
    u32 children_count;
    u32 children_capacity;
    jfw_widget** children_array;
    u32 width; u32 height;
    i32 x; i32 y;
    int redraw;
    u0* user_pointer;
    jfw_res (*convert_to_string)(const jfw_widget* this, u64 max_bytes, char* buffer);
};

jfw_res jfw_widget_create_as_child(jfw_widget* parent, u32 w, u32 h, i32 x, i32 y, jfw_widget** pp_widget);

jfw_res jfw_widget_create_as_base(jfw_window* parent, u32 w, u32 h, i32 x, i32 y, jfw_widget** pp_widget);

jfw_res jfw_widget_destroy(jfw_widget* widget);

jfw_res jfw_widget_resize(jfw_widget* widget, u32 w, u32 h);

jfw_res jfw_widget_reposition(jfw_widget* widget, i32 x, i32 y);

jfw_res jfw_widget_ask_for_redraw(jfw_widget* widget);

u0* jfw_widget_get_user_pointer(jfw_widget* widget);

u0 jfw_widget_set_user_pointer(jfw_widget* widget, u0* user_pointer);

jfw_widget_functions* jfw_widget_get_function_table(jfw_widget* widget);

u0 jfw_widget_bounds(jfw_widget* widget, i32* p_x0, i32* p_y0, i32* p_x1, i32* p_y1);

static inline jfw_res jfw_widget_to_string(const jfw_widget* this, u64 max_bytes, char* buffer)
{
    if (this && this->convert_to_string)
    {
        return this->convert_to_string(this, max_bytes, buffer);
    }
    else
    {
        snprintf(buffer, max_bytes, "%p", this);
        return jfw_res_success;
    }
}

#endif //JFW_WIDGET_BASE_H
