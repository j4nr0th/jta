//
// Created by jan on 16.1.2023.
//

#ifndef JFW_WINDOW_H
#define JFW_WINDOW_H

#include "jfw_common.h"
//  Include one platform header
#include "vk/platform.h"

//#include "color-schemes.h"

jfw_res jfw_window_hook_on_show(jfw_window* wnd, void (*on_show)(jfw_window* window));

jfw_res jfw_window_hook_on_hide(jfw_window* wnd, void (*on_hide)(jfw_window* window));

jfw_res jfw_window_hook_on_close(jfw_window* wnd, void (*on_close)(jfw_window* window));

jfw_res jfw_window_hook_on_resize(jfw_window* wnd, void (*on_resize)(jfw_window* window, u32 old_w, u32 old_h, u32 new_w, u32 new_h));

jfw_res jfw_window_hook_on_move(jfw_window* wnd, void (*on_move)(jfw_window* window, i32 old_x, i32 old_y, u32 new_x, u32 new_y));

jfw_res jfw_window_hook_on_focus_get(jfw_window* wnd, void (*on_focus_get)(jfw_window* window));

jfw_res jfw_window_hook_on_focus_lose(jfw_window* wnd, void (*on_focus_lose)(jfw_window* window));


typedef struct jfw_window_hooks_struct
{
    void (*on_show)(jfw_window* window);                                                //  Called when the window is shown
    void (*on_hide)(jfw_window* window);                                                //  Called when the window is hidden
    void (*on_close)(jfw_window* window);                                               //  Called when the window is requested to close
    void (*on_resize)(jfw_window* window, u32 old_w, u32 old_h, u32 new_w, u32 new_h);  //  Called when the window is resized
    void (*on_move)(jfw_window* window, i32 old_x, i32 old_y, u32 new_x, u32 new_y);    //  Called when the window is moved
    void (*on_focus_get)(jfw_window* window);                                           //  Called when the window gets focus back
    void (*on_focus_lose)(jfw_window* window);                                          //  Called when the window loses focus
} jfw_window_hooks;

struct jfw_window_struct
{
    jfw_ctx* ctx;                   //  context for interacting with the whole WM state and such
    u32 w, h;                       //  window dimensions
    char* title;                    //  window title
    jfw_widget* base;         //  background widget, which is the only widget the window works with directly
    jfw_platform platform;          //  platform-specific window state and handles
    int redraw;                     //  set to non-zero value if it should be redrawn next time it is possible
    jfw_widget* mouse_focus;        //  receives mouse events over others
    jfw_widget* keyboard_focus;     //  receives keyboard events over others
    jfw_window_hooks hooks;         //  functions called when something happens to the window
//    jfw_color_scheme color_scheme;  //  color scheme used for rendering colors
};

jfw_res jfw_window_create(jfw_ctx* ctx, u32 w, u32 h, char* title, jfw_color color, jfw_window** p_wnd, i32 fixed);

jfw_res jfw_window_show(jfw_ctx* ctx, jfw_window* wnd);

jfw_res jfw_window_hide(jfw_ctx* ctx, jfw_window* wnd);

jfw_res jfw_window_destroy(jfw_ctx* ctx, jfw_window* wnd);

jfw_res jfw_window_redraw(jfw_ctx* ctx, jfw_window* wnd);

jfw_res jfw_window_force_redraw(jfw_ctx* ctx, jfw_window* wnd);

jfw_res jfw_window_set_mouse_focus(jfw_ctx* ctx, jfw_window* wnd, jfw_widget* new_focus);

jfw_res jfw_window_set_keyboard_focus(jfw_ctx* ctx, jfw_window* wnd, jfw_widget* new_focus);

jfw_widget* jfw_window_get_base(jfw_window* wnd);

jfw_res jfw_window_set_base(jfw_window* wnd, jfw_widget* base);

//jfw_res jfw_window_set_color_scheme(jfw_window* wnd, jfw_color_scheme color_scheme);

jfw_window_hooks* jfw_window_hook_ptr(jfw_window* wnd);

#endif //JFW_WINDOW_H
