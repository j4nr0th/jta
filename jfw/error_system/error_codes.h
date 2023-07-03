//
// Created by jan on 2.4.2023.
//

#ifndef JFW_ERROR_CODES_H
#define JFW_ERROR_CODES_H

typedef enum jfw_res_enum jfw_res;
#include <stdlib.h>
enum jfw_res_enum
{
    jfw_res_success = 0,
    jfw_res_error,
    jfw_res_bad_error,

    jfw_res_bad_malloc,
    jfw_res_bad_realloc,

    jfw_res_platform_no_wnd,
    jfw_res_invalid_window,
    jfw_res_window_close,

    jfw_res_ctx_no_dpy,
    jfw_res_ctx_no_im,
    jfw_res_ctx_no_ic,

    jfw_res_platform_no_bind,
    jfw_res_ctx_no_windows,
    jfw_res_wnd_widget_root,

    jfw_res_platform_no_FB_config,
    jfw_res_platform_no_visual,
    jfw_res_platform_no_cmap,
    jfw_res_platform_no_glx,
    jfw_res_no_renderer,

    jfw_res_vk_fail,

    jfw_count,  //  Has to be the last in the enum
};

const char* jfw_error_message(jfw_res error_code);

jfw_res jfw_error_message_r(jfw_res error_code, size_t buffer_size, char* restrict buffer);

const char* jfw_vk_error_msg(size_t vk_code);

#endif //JFW_ERROR_CODES_H
