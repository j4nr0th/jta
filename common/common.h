//
// Created by jan on 27.5.2023.
//

#ifndef JTA_COMMON_H
#define JTA_COMMON_H
//  Standard library includes
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <malloc.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <wchar.h>
#include <uchar.h>

#include <stdbool.h>

//  Typedefs used for the project
typedef uint_fast64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int_fast64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef double_t f64;
typedef float_t f32;

typedef void u0, i0;
#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
#endif
typedef unsigned char c8;
typedef uint_least16_t c16;
typedef uint_least32_t c32;

union jta_color_T
{
    struct
    {
        unsigned char r, g, b, a;
    };
    unsigned char data[4];
    uint32_t packed;
};
typedef union jta_color_T jta_color;

#include <jmem/jmem.h>


extern lin_allocator* G_LIN_ALLOCATOR;
extern ill_allocator* G_ALLOCATOR;

#include <assert.h>

#include "../gfx/gfx_math.h"

typedef struct ubo_3d_T ubo_3d;
struct ubo_3d_T
{
    mtx4 view;
    mtx4 proj;
    vec4 view_direction;
};
typedef struct ubo_ui_vtx_T ubo_ui_vtx;
struct ubo_ui_vtx_T
{
    float scale_x, scale_y;
    float offset_x, offset_y;
};
typedef struct ubo_ui_frg_T ubo_ui_frg;
struct ubo_ui_frg_T
{
    float font_w, font_h;
    float font_off_x, font_off_y;
};

typedef struct jta_timer_T jta_timer;
struct jta_timer_T
{
    struct timespec ts;
};

void jta_timer_set(jta_timer* timer);

f64 jta_timer_get(jta_timer* timer);


#endif //JTA_COMMON_H
