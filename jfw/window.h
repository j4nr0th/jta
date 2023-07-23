//
// Created by jan on 16.1.2023.
//

#ifndef JFW_WINDOW_H
#define JFW_WINDOW_H

#include "jfw_common.h"
//  Include one platform header
#include "platform.h"


typedef struct jfw_window_functions_struct
{
    //  Related to mouse input
    jfw_result (*mouse_button_press)(jfw_window* this, i32 x, i32 y, u32 button, u32 mods);
    jfw_result (*mouse_button_double_press)(jfw_window* this, i32 x, i32 y, u32 button, u32 mods);
    jfw_result (*mouse_button_release)(jfw_window* this, i32 x, i32 y, u32 button, u32 mods);
    jfw_result (*mouse_motion)(jfw_window* this, i32 x, i32 y, u32 mods);
    jfw_result (*mouse_focus_get)(jfw_window* this, jfw_window* new_focus);
    jfw_result (*mouse_focus_lose)(jfw_window* this, jfw_window* old_focus);
    //  Related to keyboard input
    jfw_result (*button_down)(jfw_window* this, KeySym key_code);
    jfw_result (*button_up)(jfw_window* this, KeySym key_code);
    jfw_result (*char_input)(jfw_window* this, const char* utf8);
    jfw_result (*keyboard_focus_get)(jfw_window* this);
    jfw_result (*keyboard_focus_lose)(jfw_window* this);
    //  Related to window changes
    jfw_result (*window_focused)(jfw_window* this);
    jfw_result (*window_unfocused)(jfw_window* this);
    //  Related to drawing
    jfw_result (*drawing_predraw)(jfw_window* this);
    jfw_result (*draw)(jfw_window* this);
    jfw_result (*drawing_postdraw)(jfw_window* this);
    jfw_result (*dtor_fn)(jfw_window* this);
    //  Related to motion and resizing
    jfw_result (*on_move)(jfw_window* this, i32 old_x, i32 old_y, u32 new_x, u32 new_y); //  Called when the window is moved
    jfw_result (*on_show)(jfw_window* this);                                                //  Called when the window is shown
    jfw_result (*on_hide)(jfw_window* this);                                                //  Called when the window is hidden
    jfw_result (*on_close)(jfw_window* this);                                               //  Called when the window is requested to close
    jfw_result (*on_resize)(jfw_window* this, u32 old_w, u32 old_h, u32 new_w, u32 new_h);  //  Called when the window is resized
} jfw_window_functions;


struct jfw_window_struct
{
    jfw_ctx* ctx;                   //  context for interacting with the whole WM state and such
    u32 w, h;                       //  window dimensions
    char* title;                    //  window title
    jfw_platform platform;          //  platform-specific window state and handles
    int redraw;                     //  set to non-zero value if it should be redrawn next time it is possible
    jfw_window_functions functions; //  functions called when something happens to the window
    void* user_ptr;
};

jfw_result jfw_window_create(jfw_ctx* ctx, u32 w, u32 h, char* title, jfw_color color, jfw_window** p_wnd, i32 fixed);

jfw_result jfw_window_show(jfw_ctx* ctx, jfw_window* wnd);

jfw_result jfw_window_hide(jfw_ctx* ctx, jfw_window* wnd);

jfw_result jfw_window_destroy(jfw_ctx* ctx, jfw_window* wnd);

jfw_result jfw_window_ask_for_redraw(jfw_window* wnd);

jfw_result jfw_window_redraw(jfw_ctx* ctx, jfw_window* wnd);

jfw_result jfw_window_force_redraw(jfw_ctx* ctx, jfw_window* wnd);

//jfw_result jfw_window_set_color_scheme(jfw_window* wnd, jfw_color_scheme color_scheme);

jfw_window_functions* jfw_window_function_table_ptr(jfw_window* wnd);

void* jfw_window_get_usr_ptr(jfw_window* wnd);

void jfw_window_set_usr_ptr(jfw_window* wnd, void* new_ptr);

#endif //JFW_WINDOW_H
