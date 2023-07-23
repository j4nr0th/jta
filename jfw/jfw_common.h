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


typedef union render_color_struct jfw_color;
union render_color_struct
{
    uint32_t packed;
    uint8_t data[4];
    struct
    {
        uint8_t r, g, b, a;
    };
};


struct jfw_allocator_callbacks_struct
{
    void* (*alloc)(void* state, uint64_t size);
    void* (*realloc)(void* state, void* ptr, uint64_t new_size);
    void (*free)(void* state, void* ptr);
    void* state;
};

typedef struct jfw_allocator_callbacks_struct jfw_allocator_callbacks;


#endif //JFW_COMMON_H
