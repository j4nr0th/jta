//
// Created by jan on 16.1.2023.
//
#include <jdm.h>
#include "window.h"
#include "widget-base.h"
//#include "shaders/shader_src.h"

static jfw_res redraw_background(jfw_widget* this)
{
    JDM_ENTER_FUNCTION;
//    jfw_color color = {.packed = (u32)(u64)this->user_pointer};
//    const jfw_color color = this->window->color_scheme.color_BG;
//    glClearColor((f32)color.r / 255.0f, (f32)color.g / 255.0f, (f32)color.b / 255.0f, (f32)color.a / 255.0f);
//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_create(jfw_ctx* ctx, u32 w, u32 h, char* title, jfw_color color, jfw_window** p_wnd, i32 fixed)
{
    JDM_ENTER_FUNCTION;
    jfw_res result;
    jfw_window* this;
    if (!jfw_success(result = jfw_malloc(sizeof(*this), &this)))
    {
        JDM_ERROR("Memory allocation for window failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    memset(this, 0, sizeof(*this));
//    this->color_scheme = JDM_DEFAULT_COLOR_SCHEME;

    if (!title)
    {
        title = "jfw_window";
    }
    this->title = NULL;
    size_t title_len = strlen(title);
    if (!jfw_success(result = jfw_malloc(title_len + 1, &this->title)))
    {
        jfw_free(&this);
        JDM_ERROR("Memory allocation for window title failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    memcpy(this->title, title, title_len + 1);
    if (!jfw_success(result = jfw_platform_create(ctx, &this->platform, w, h, title_len, title, 2, fixed, color)))
    {
        jfw_free(&this->title);
        jfw_free(&this);
        JDM_ERROR("Window platform interface creation failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    this->ctx = ctx;
    this->w = w;
    this->h = h;
    //  Create window's renderer and background
    //      renderer
    jfw_platform_draw_bind(ctx, &this->platform);

    //  Register window to the context's list of all windows
    if (!jfw_success(result = jfw_context_add_window(ctx, this)))
    {
        jfw_free(&this->base);
        jfw_free(&this->title);
        jfw_platform_destroy(&this->platform);
        jfw_free(&this);
        JDM_ERROR("Registering window to the library context failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    jfw_window_redraw(ctx, this);

    *p_wnd = this;
    JDM_LEAVE_FUNCTION;
    return result;
}

jfw_res jfw_window_destroy(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_res result = jfw_res_success;
    if (!jfw_success(result = jfw_context_remove_window(ctx, wnd)))
    {
        JDM_ERROR("Removing window registration from the library context failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    assert(ctx == wnd->ctx);
    //  Destroy window's widgets
    if (wnd->base)
    {
        jfw_widget_destroy(wnd->base);
    }
//    while (wnd->base->children_count)
//    {
//        jfw_widget_destroy(wnd->base->children_array[wnd->base->children_count - 1]);
//    }
//    if (wnd->hooks.on_close)
//    {
//        wnd->hooks.on_close(wnd);
//    }
    jfw_free(&wnd->title);
    jfw_platform_destroy(&wnd->platform);
    memset(wnd, 0, sizeof(*wnd));
    jfw_free(&wnd);

    JDM_LEAVE_FUNCTION;
    return jfw_res_window_close;
}

jfw_res jfw_window_show(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    const jfw_res res = jfw_platform_show(ctx, &wnd->platform);
    if (wnd->redraw)
    {
        jfw_window_redraw(ctx, wnd);
    }
    JDM_LEAVE_FUNCTION;
    return res;
}

jfw_res jfw_window_hide(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    const jfw_res res = jfw_platform_hide(ctx, &wnd->platform);
    JDM_LEAVE_FUNCTION;
    return res;
}

static void redraw_widget(jfw_widget* widget)
{
    JDM_ENTER_FUNCTION;
    if (widget->redraw)
    {
        if (widget->functions.drawing_predraw)
        {
            widget->functions.drawing_predraw(widget);
        }
        if (widget->draw_fn)
        {
            widget->draw_fn(widget);
        }
        if (widget->functions.drawing_postdraw)
        {
            widget->functions.drawing_postdraw(widget);
        }
        for (u32 i = 0; i < widget->children_count; ++i)
        {
            jfw_widget* child = widget->children_array[i];
            child->redraw += 1;
            if (child->functions.parent_redrawn)
            {
                child->functions.parent_redrawn(child, widget);
            }
            redraw_widget(child);
        }
        widget->redraw = 0;
    }
    JDM_LEAVE_FUNCTION;
}

jfw_res jfw_window_redraw(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_res res = jfw_res_success;

    if (wnd->redraw && wnd->base)
    {
        if (!jfw_success(res = jfw_platform_draw_bind(ctx, &wnd->platform)))
        {
            JDM_ERROR("Binding window's drawing platform interface failed");
            JDM_LEAVE_FUNCTION;
            return res;
        }
        wnd->base->redraw += (wnd->base->redraw == 0);
        redraw_widget(wnd->base);
        jfw_platform_swap(ctx, &wnd->platform);
        wnd->redraw = 0;
    }
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_force_redraw(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    wnd->redraw += 1;
    const jfw_res res = jfw_window_redraw(ctx, wnd);
    JDM_LEAVE_FUNCTION;
    return res;
}

jfw_res jfw_window_set_mouse_focus(jfw_ctx* ctx, jfw_window* wnd, jfw_widget* new_focus)
{
    JDM_ENTER_FUNCTION;
    jfw_widget* const old_focus = wnd->mouse_focus;
    jfw_res res = jfw_res_success;
    if (old_focus && old_focus->functions.mouse_focus_lose && !jfw_success(res = old_focus->functions.mouse_focus_lose(old_focus, new_focus)))
    {
        char w_buffer_1[64];
        char w_buffer_2[64];
        jfw_widget_to_string(old_focus, 64, w_buffer_1);
        jfw_widget_to_string(new_focus, 64, w_buffer_2);
        JDM_ERROR("Reporting loss of mouse focus from widget (%s) to widget (%s) failed", w_buffer_1, w_buffer_2);
        JDM_LEAVE_FUNCTION;
        return res;
    }
    wnd->mouse_focus = NULL;
    if (new_focus && new_focus->functions.mouse_focus_get && !jfw_success(res = new_focus->functions.mouse_focus_get(new_focus, old_focus)))
    {
        char w_buffer_1[64];
        char w_buffer_2[64];
        jfw_widget_to_string(old_focus, 64, w_buffer_1);
        jfw_widget_to_string(new_focus, 64, w_buffer_2);
        JDM_ERROR("Reporting gain of mouse focus from widget (%s) to widget (%s) failed", w_buffer_1, w_buffer_2);
        JDM_LEAVE_FUNCTION;
        return res;
    }
    wnd->mouse_focus = new_focus;

    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_set_keyboard_focus(jfw_ctx* ctx, jfw_window* wnd, jfw_widget* new_focus)
{
    JDM_ENTER_FUNCTION;
    jfw_widget* const old_focus = wnd->keyboard_focus;
    jfw_res res = jfw_res_success;
    if (old_focus && old_focus->functions.keyboard_focus_lose && !jfw_success(res = old_focus->functions.keyboard_focus_lose(old_focus, new_focus)))
    {
        char w_buffer_1[64];
        char w_buffer_2[64];
        jfw_widget_to_string(old_focus, 64, w_buffer_1);
        jfw_widget_to_string(new_focus, 64, w_buffer_2);
        JDM_ERROR("Reporting loss of keyboard focus from widget (%s) to widget (%s) failed", w_buffer_1, w_buffer_2);
        JDM_LEAVE_FUNCTION;
        return res;
    }
    wnd->keyboard_focus = NULL;
    if (new_focus && new_focus->functions.keyboard_focus_get && !jfw_success(res = new_focus->functions.keyboard_focus_get(new_focus, old_focus)))
    {
        char w_buffer_1[64];
        char w_buffer_2[64];
        jfw_widget_to_string(old_focus, 64, w_buffer_1);
        jfw_widget_to_string(new_focus, 64, w_buffer_2);
        JDM_ERROR("Reporting loss of mouse focus from widget (%s) to widget (%s) failed", w_buffer_1, w_buffer_2);
        JDM_LEAVE_FUNCTION;
        return res;
    }
    wnd->keyboard_focus = new_focus;

    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_show(jfw_window* wnd, void (* on_show)(jfw_window*))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_show = on_show;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_hide(jfw_window* wnd, void (* on_hide)(jfw_window*))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_hide = on_hide;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_close(jfw_window* wnd, void (* on_close)(jfw_window*))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_close = on_close;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_resize(jfw_window* wnd, void (* on_resize)(jfw_window*, u32, u32, u32, u32))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_resize = on_resize;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_move(jfw_window* wnd, void (* on_move)(jfw_window*, i32, i32, u32, u32))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_move = on_move;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_focus_get(jfw_window* wnd, void (* on_focus_get)(jfw_window*))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_focus_get = on_focus_get;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_window_hook_on_focus_lose(jfw_window* wnd, void (* on_focus_lose)(jfw_window*))
{
    JDM_ENTER_FUNCTION;
    wnd->hooks.on_focus_lose = on_focus_lose;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_widget* jfw_window_get_base(jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_widget* base = wnd->base;
    JDM_LEAVE_FUNCTION;
    return base;
}

jfw_res jfw_window_set_base(jfw_window* wnd, jfw_widget* base)
{
    JDM_ENTER_FUNCTION;
    wnd->base = base;
    JDM_LEAVE_FUNCTION;
    return jfw_res_success;
}

//jfw_res jfw_window_set_color_scheme(jfw_window* wnd, jfw_color_scheme color_scheme)
//{
//    JDM_ENTER_FUNCTION;
//    wnd->color_scheme = color_scheme;
//    jfw_widget_ask_for_redraw(wnd->base);
//    JDM_LEAVE_FUNCTION;
//    return jfw_res_success;
//}

jfw_window_hooks* jfw_window_hook_ptr(jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_window_hooks* hooks = &wnd->hooks;
    JDM_LEAVE_FUNCTION;
    return &wnd->hooks;
}

void* jfw_window_get_usr_ptr(jfw_window* wnd)
{
    return wnd->user_ptr;
}

void jfw_window_set_usr_ptr(jfw_window* wnd, void* new_ptr)
{
    wnd->user_ptr = new_ptr;
}
