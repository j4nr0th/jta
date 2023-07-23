//
// Created by jan on 2.4.2023.
//

#ifndef JFW_ERROR_CODES_H
#define JFW_ERROR_CODES_H

typedef enum jfw_result_enum jfw_result;
#include <stdlib.h>
enum jfw_result_enum
{
    JFW_RESULT_SUCCESS = 0,
    JFW_RESULT_ERROR,
    JFW_RESULT_BAD_ERROR,

    JFW_RESULT_BAD_ALLOC,
    JFW_RESULT_BAD_REALLOC,

    JFW_RESULT_PLATFORM_NO_WND,
    JFW_RESULT_INVALID_WINDOW,
    JFW_RESULT_WINDOW_CLOSE,

    JFW_RESULT_CTX_NO_DPY,
    JFW_RESULT_CTX_NO_IM,
    JFW_RESULT_CTX_NO_IC,

    JFW_RESULT_PLATFORM_NO_BIND,
    JFW_RESULT_CTX_NO_WINDOWS,

    JFW_RESULT_VK_FAIL,

    JFW_RESULT_COUNT,  //  Has to be the last in the enum
};

const char* jfw_error_message(jfw_result error_code);

jfw_result jfw_error_message_r(jfw_result error_code, size_t buffer_size, char* restrict buffer);

const char* jfw_vk_error_msg(size_t vk_code);

#endif //JFW_ERROR_CODES_H
