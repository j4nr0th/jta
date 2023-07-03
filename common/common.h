//
// Created by jan on 27.5.2023.
//

#ifndef JTB_COMMON_H
#define JTB_COMMON_H
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


#include "../jfw/jfw_common.h"
#include "../jfw/window.h"
#include "../jfw/widget-base.h"
#include <jmem/jmem.h>
#include "../mem/aligned_jalloc.h"


extern linear_jallocator* G_LIN_JALLOCATOR;
extern aligned_jallocator* G_ALIGN_JALLOCATOR;
extern jallocator* G_JALLOCATOR;

#include <vulkan/vulkan.h>
#include <assert.h>

#include "../gfx/gfx_math.h"

typedef struct ubo_3d_struct ubo_3d;
struct ubo_3d_struct
{
    mtx4 view;
    mtx4 proj;
    vec4 view_direction;
};


//  My custom memory allocation functions
#include <jmem/jmem.h>


//#ifndef NDEBUG
////  When in debug mode, jallocator is not used, so that malloc can be used for debugging instead
//#define jalloc(a, size) malloc(size)
//#define jfree(a, ptr) free(ptr)
//#define jrealloc(a, ptr, new_size) realloc(ptr, new_size)
////#define lin_jalloc(a, p, sz) malloc(sz)
////#define lin_jfree(a, p, p1) free(p1)
//#else
//#define jalloc(a, size) jalloc((a), (size))
//#define jfree(a, ptr) jfree((a), (ptr))
//#define jrealloc(a, ptr, new_size) jrealloc((a), (ptr), (new_size))
//#endif

#endif //JTB_COMMON_H
