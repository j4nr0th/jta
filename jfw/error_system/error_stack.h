//
// Created by jan on 2.4.2023.
//

#ifndef JFW_ERROR_STACK_H
#define JFW_ERROR_STACK_H
#include "error_codes.h"

typedef enum jfw_error_level_enum jfw_error_level;
enum jfw_error_level_enum
{
    jfw_error_level_none = 0,
    jfw_error_level_info = 1,
    jfw_error_level_warn = 2,
    jfw_error_level_err = 3,
    jfw_error_level_crit = 4,
};

const char* jfw_error_level_str(jfw_error_level level);

void jfw_error_init_thread(char* thread_name, jfw_error_level level, u32 max_stack_trace, u32 max_errors);
void jfw_error_cleanup_thread(void);

u32 jfw_error_enter_function(const char* fn_name);
void jfw_error_leave_function(const char* fn_name, u32 level);

void jfw_error_push(jfw_error_level level, u32 line, const char* file, const char* function, const char* fmt, ...);
[[noreturn]] void jfw_error_report_critical(const char* fmt, ...);

typedef i32 (*jfw_error_report_fn)(u32 total_count, u32 index, jfw_error_level level, u32 line, const char* file, const char* function, const char* message, void* param);
typedef i32 (*jfw_error_hook_fn)(const char* thread_name, u32 stack_trace_count, const char*const* stack_trace, jfw_error_level level, u32 line, const char* file, const char* function, const char* message, void* param);

void jfw_error_process(jfw_error_report_fn function, void* param);
void jfw_error_peek(jfw_error_report_fn function, void* param);
void jfw_error_set_hook(jfw_error_hook_fn function, void* param);


#define JFW_ERROR(fmt, ...) jfw_error_push(jfw_error_level_err, __LINE__, __FILE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define JFW_WARN(fmt, ...) jfw_error_push(jfw_error_level_warn, __LINE__, __FILE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define JFW_INFO(fmt, ...) jfw_error_push(jfw_error_level_info, __LINE__, __FILE__, __func__, fmt __VA_OPT__(,) __VA_ARGS__)
#define JFW_ERROR_CRIT(fmt, ...) jfw_error_report_critical("JFW Error from \"%s\" at line %i: " fmt "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#ifndef NDEBUG
#define JFW_ENTER_FUNCTION const u32 _jfw_stacktrace_level = jfw_error_enter_function(__func__)
#define JFW_LEAVE_FUNCTION jfw_error_leave_function(__func__, _jfw_stacktrace_level)
#else
#define JFW_ENTER_FUNCTION (void)0
#define JFW_LEAVE_FUNCTION (void)0
#endif
#endif //JFW_ERROR_STACK_H
