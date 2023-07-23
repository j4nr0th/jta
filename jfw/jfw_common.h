//
// Created by jan on 16.1.2023.
//

#ifndef JFW_COMMON_H
#define JFW_COMMON_H
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "jfw_error.h"

typedef struct jfw_context_struct jfw_ctx;
typedef struct jfw_window_struct jfw_window;
typedef struct jfw_window_platform_struct jfw_platform;


typedef int64_t i64;
typedef uint64_t u64;
typedef int32_t i32;
typedef uint32_t u32;
typedef int16_t i16;
typedef uint16_t u16;
typedef int8_t i8;
typedef uint8_t u8;
typedef void u0;

typedef float f32;
typedef double f64;

//typedef enum jfw_result_enum jfw_result;

typedef struct rect32u_struct
{
    u32 x0, x1, y0, y1;
} rect32u;

typedef struct rect32i_struct
{
    i32 x0, x1, y0, y1;
} rect32i;

typedef struct rect32f_struct
{
    f32 x0, x1, y0, y1;
} rect32f;

typedef union render_color_struct jfw_color;
union render_color_struct
{
    u32 packed;
    u8 data[4];
    struct
    {
        u8 r, g, b, a;
    };
};


static inline int rect32u_contains_point(const rect32u rect, const u32 x, const u32 y)
{
    return rect.x0 <= x && rect.x1 > x && rect.y0 <= y && rect.y1 > y;
}

static inline int rect32i_contains_point(const rect32i rect, const i32 x, const i32 y)
{
    return rect.x0 <= x && rect.x1 > x && rect.y0 <= y && rect.y1 > y;
}

static inline int rect32f_contains_point(const rect32f rect, const f32 x, const f32 y)
{
    return rect.x0 <= x && rect.x1 > x && rect.y0 <= y && rect.y1 > y;
}

static inline i32 clamp_i32(i32 x, i32 min, i32 max)
{
    return x < min ? min : x > max ? max : x;
}

jfw_result jfw_malloc(size_t size, void** pptr);
#define jfw_malloc(size, pptr) jfw_malloc(size, (void**)(pptr))

jfw_result jfw_calloc(size_t nmemb, size_t size, void** (pptr));
#define jfw_calloc(nmemb, size, pptr) jfw_calloc(nmemb, size, (void**)(pptr))

jfw_result jfw_realloc(size_t new_size, void** ptr);
#define jfw_realloc(new_size, ptr) jfw_realloc(new_size, (void**)(ptr))

jfw_result jfw_free(void** ptr);
#define jfw_free(ptr) jfw_free((void**)(ptr))


#endif //JFW_COMMON_H
