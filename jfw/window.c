//
// Created by jan on 16.1.2023.
//
#include <jdm.h>
#include "window.h"
//#include "shaders/shader_src.h"

jfw_result jfw_window_create(jfw_ctx* ctx, u32 w, u32 h, char* title, jfw_color color, jfw_window** p_wnd, i32 fixed)
{
    JDM_ENTER_FUNCTION;
    jfw_result result;
    jfw_window* this;
    if (JFW_RESULT_SUCCESS !=(result = jfw_malloc(sizeof(*this), &this)))
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
    if (JFW_RESULT_SUCCESS !=(result = jfw_malloc(title_len + 1, &this->title)))
    {
        jfw_free(&this);
        JDM_ERROR("Memory allocation for window title failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    memcpy(this->title, title, title_len + 1);
    if (JFW_RESULT_SUCCESS !=(result = jfw_platform_create(ctx, &this->platform, w, h, title_len, title, 2, fixed, color)))
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

    //  Register window to the context's list of all windows
    if (JFW_RESULT_SUCCESS !=(result = jfw_context_add_window(ctx, this)))
    {
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

jfw_result jfw_window_destroy(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_result result;
    if (wnd->functions.dtor_fn)
    {
        wnd->functions.dtor_fn(wnd);
    }
    if (JFW_RESULT_SUCCESS !=(result = jfw_context_remove_window(ctx, wnd)))
    {
        JDM_ERROR("Removing window registration from the library context failed");
        JDM_LEAVE_FUNCTION;
        return result;
    }
    assert(ctx == wnd->ctx);
    //  Destroy window's widgets
    jfw_free(&wnd->title);
    jfw_platform_destroy(&wnd->platform);
    memset(wnd, 0, sizeof(*wnd));
    jfw_free(&wnd);

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_WINDOW_CLOSE;
}

jfw_result jfw_window_show(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    const jfw_result res = jfw_platform_show(ctx, &wnd->platform);
    if (wnd->redraw)
    {
        jfw_window_redraw(ctx, wnd);
    }
    JDM_LEAVE_FUNCTION;
    return res;
}

jfw_result jfw_window_hide(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    const jfw_result res = jfw_platform_hide(ctx, &wnd->platform);
    JDM_LEAVE_FUNCTION;
    return res;
}

jfw_result jfw_window_redraw(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_result res = JFW_RESULT_SUCCESS;

    if (wnd->redraw && wnd->functions.draw)
    {
        res = wnd->functions.draw(wnd);
    }
    JDM_LEAVE_FUNCTION;
    return res;
}

jfw_result jfw_window_force_redraw(jfw_ctx* ctx, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    wnd->redraw += 1;
    const jfw_result res = jfw_window_redraw(ctx, wnd);
    JDM_LEAVE_FUNCTION;
    return res;
}

jfw_window_functions* jfw_window_function_table_ptr(jfw_window* wnd)
{
    return &wnd->functions;
}

void* jfw_window_get_usr_ptr(jfw_window* wnd)
{
    return wnd->user_ptr;
}

void jfw_window_set_usr_ptr(jfw_window* wnd, void* new_ptr)
{
    wnd->user_ptr = new_ptr;
}

jfw_result jfw_window_ask_for_redraw(jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;

    wnd->redraw += 1;

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}
