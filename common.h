//
// Created by jan on 14.5.2023.
//

#ifndef JTB_COMMON_H
#define JTB_COMMON_H

#include "jfw/jfw_common.h"
#include "jfw/window.h"
#include "jfw/widget-base.h"
#include "mem/lin_jalloc.h"
#include "mem/aligned_jalloc.h"


extern linear_jallocator* G_LIN_JALLOCATOR;
extern aligned_jallocator* G_ALIGN_JALLOCATOR;

#include <vulkan/vulkan.h>
#include <assert.h>

#include "jfw/gfx_math.h"

typedef struct ubo_3d_struct ubo_3d;
struct ubo_3d_struct
{
    mtx4 view;
    mtx4 proj;
};

#endif //JTB_COMMON_H
