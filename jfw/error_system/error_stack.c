//
// Created by jan on 2.4.2023.
//

#include "error_stack.h"
#include <stdarg.h>

struct jfw_error_message
{
    jfw_error_level level;
    const char* file;
    const char* function;
    u32 line;
    char message[];
};

typedef struct jfw_error_state_struct jfw_error_state;
struct jfw_error_state_struct
{
    i32 init;

    char* thrd_name;
    size_t len_name;

    u32 error_count;
    u32 error_capacity;
    struct jfw_error_message** errors;

    u32 stacktrace_count;
    u32 stacktrace_capacity;
    const char** stack_traces;

    jfw_error_level level;

    jfw_error_hook_fn hook;
    void* hook_param;
};

_Thread_local jfw_error_state JFW_THREAD_ERROR_STATE;

void jfw_error_init_thread(char* thread_name, jfw_error_level level, u32 max_stack_trace, u32 max_errors)
{
    assert(JFW_THREAD_ERROR_STATE.init == 0);
    JFW_THREAD_ERROR_STATE.thrd_name = thread_name ? strdup(thread_name) : NULL;
    assert(JFW_THREAD_ERROR_STATE.thrd_name != NULL);
    JFW_THREAD_ERROR_STATE.len_name = thread_name ? strlen(thread_name) : 0;
    JFW_THREAD_ERROR_STATE.level = level;
    JFW_THREAD_ERROR_STATE.errors = calloc(max_errors, sizeof(*JFW_THREAD_ERROR_STATE.errors));
    assert(JFW_THREAD_ERROR_STATE.errors != NULL);
    JFW_THREAD_ERROR_STATE.error_capacity = max_errors;

    JFW_THREAD_ERROR_STATE.stack_traces = calloc(max_stack_trace, sizeof(char*));
    assert(JFW_THREAD_ERROR_STATE.stack_traces != NULL);
    JFW_THREAD_ERROR_STATE.stacktrace_capacity = max_stack_trace;
}

void jfw_error_cleanup_thread(void)
{
    assert(JFW_THREAD_ERROR_STATE.stacktrace_count == 0);
    free(JFW_THREAD_ERROR_STATE.thrd_name);
    for (u32 i = 0; i < JFW_THREAD_ERROR_STATE.error_count; ++i)
    {
        free(JFW_THREAD_ERROR_STATE.errors[i]);
    }
    free(JFW_THREAD_ERROR_STATE.errors);
    free(JFW_THREAD_ERROR_STATE.stack_traces);
    memset(&JFW_THREAD_ERROR_STATE, 0, sizeof(JFW_THREAD_ERROR_STATE));
}

u32 jfw_error_enter_function(const char* fn_name)
{
    u32 id = JFW_THREAD_ERROR_STATE.stacktrace_count;
    if (JFW_THREAD_ERROR_STATE.stacktrace_count < JFW_THREAD_ERROR_STATE.stacktrace_capacity)
    {
        JFW_THREAD_ERROR_STATE.stack_traces[JFW_THREAD_ERROR_STATE.stacktrace_count++] = fn_name;
    }
    return id;
}

void jfw_error_leave_function(const char* fn_name, u32 level)
{
    if (level < JFW_THREAD_ERROR_STATE.stacktrace_capacity)
    {
        assert(JFW_THREAD_ERROR_STATE.stacktrace_count == level + 1);
        assert(JFW_THREAD_ERROR_STATE.stack_traces[level] == fn_name);
        JFW_THREAD_ERROR_STATE.stacktrace_count = level;
    }
    else
    {
        assert(level == JFW_THREAD_ERROR_STATE.stacktrace_capacity);
        assert(JFW_THREAD_ERROR_STATE.stacktrace_count == JFW_THREAD_ERROR_STATE.stacktrace_capacity);
    }
}

void jfw_error_push(jfw_error_level level, u32 line, const char* file, const char* function, const char* fmt, ...)
{
    if (level < JFW_THREAD_ERROR_STATE.level || JFW_THREAD_ERROR_STATE.error_count == JFW_THREAD_ERROR_STATE.error_capacity) return;
    va_list args1, args2;
    va_start(args1, fmt);
    va_copy(args2, args1);
    const size_t error_length = JFW_THREAD_ERROR_STATE.len_name + vsnprintf(NULL, 0, fmt, args1) + 4;
    va_end(args1);
    struct jfw_error_message* message = malloc(sizeof(*message) + error_length);
    assert(message);
    size_t used = vsnprintf(message->message, error_length, fmt, args2);
    va_end(args2);
    snprintf(message->message + used, error_length - used, " (%s)", JFW_THREAD_ERROR_STATE.thrd_name);
    i32 put_in_stack = 1;
    if (JFW_THREAD_ERROR_STATE.hook)
    {
        put_in_stack = JFW_THREAD_ERROR_STATE.hook(JFW_THREAD_ERROR_STATE.thrd_name, JFW_THREAD_ERROR_STATE.stacktrace_count, JFW_THREAD_ERROR_STATE.stack_traces, level, line, file, function, message->message, JFW_THREAD_ERROR_STATE.hook_param);
    }

    if (put_in_stack)
    {
        message->file = file;
        message->level = level;
        message->function = function;
        message->line = line;
        JFW_THREAD_ERROR_STATE.errors[JFW_THREAD_ERROR_STATE.error_count++] = message;
    }
    else
    {
        free(message);
    }
}

[[noreturn]] void jfw_error_report_critical(const char* fmt, ...)
{
    fprintf(stderr, "Critical error:\n");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\t(%s : %s", JFW_THREAD_ERROR_STATE.thrd_name, JFW_THREAD_ERROR_STATE.stack_traces[0]);
    for (u32 i = 1; i < JFW_THREAD_ERROR_STATE.stacktrace_count; ++i)
    {
        fprintf(stderr, " -> %s", JFW_THREAD_ERROR_STATE.stack_traces[i]);
    }
    fprintf(stderr, ")\nTerminating program\n");
    exit(EXIT_FAILURE);
}

void jfw_error_process(jfw_error_report_fn function, void* param)
{
    assert(function);
    u32 i;
    for (i = 0; i < JFW_THREAD_ERROR_STATE.error_count; ++i)
    {
        struct jfw_error_message* msg = JFW_THREAD_ERROR_STATE.errors[JFW_THREAD_ERROR_STATE.error_count - 1 - i];
        const i32 ret = function(JFW_THREAD_ERROR_STATE.error_count, i, msg->level, msg->line, msg->file, msg->function, msg->message, param);
        if (ret < 0) break;
    }
    for (u32 j = 0; j < i; ++j)
    {
        struct jfw_error_message* msg = JFW_THREAD_ERROR_STATE.errors[JFW_THREAD_ERROR_STATE.error_count - 1 - j];
        JFW_THREAD_ERROR_STATE.errors[JFW_THREAD_ERROR_STATE.error_count - 1 - j] = NULL;
        free(msg);
    }
    JFW_THREAD_ERROR_STATE.error_count = 0;
}

void jfw_error_peek(jfw_error_report_fn function, void* param)
{
    assert(function);
    u32 i;
    for (i = 0; i < JFW_THREAD_ERROR_STATE.error_count; ++i)
    {
        struct jfw_error_message* msg = JFW_THREAD_ERROR_STATE.errors[JFW_THREAD_ERROR_STATE.error_count - 1 - i];
        const i32 ret = function(JFW_THREAD_ERROR_STATE.error_count, JFW_THREAD_ERROR_STATE.error_count - 1 - i, msg->level, msg->line, msg->file, msg->function, msg->message, param);
        if (ret < 0) break;
    }
}

void jfw_error_set_hook(jfw_error_hook_fn function, void* param)
{
    JFW_THREAD_ERROR_STATE.hook = function;
    JFW_THREAD_ERROR_STATE.hook_param = param;
}

const char* jfw_error_level_str(jfw_error_level level)
{
    static const char* const NAMES[] =
            {
                    [jfw_error_level_none] = "jfw_error_level_none" ,
                    [jfw_error_level_info] = "jfw_error_level_info" ,
                    [jfw_error_level_warn] = "jfw_error_level_warn" ,
                    [jfw_error_level_err] = "jfw_error_level_err"  ,
                    [jfw_error_level_crit] = "jfw_error_level_crit" ,
            };
    if (level >= jfw_error_level_none && level <= jfw_error_level_crit)
    {
        return NAMES[level];
    }
    return NULL;
}
