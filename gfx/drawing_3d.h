//
// Created by jan on 18.5.2023.
//

#ifndef JTA_DRAWING_3D_H
#define JTA_DRAWING_3D_H
#include "../mem/aligned_jalloc.h"
#include <jmem/jmem.h>



//#include "vk_state.h"
#include "camera.h"
#include "gfxerr.h"
#include "mesh.h"
#include "vk_resources.h"

//
//gfx_result
//draw_frame(
//        vk_state* state, jfw_window_vk_resources* vk_resources, jta_structure_meshes* meshes,
//        const jta_camera_3d* camera);

gfx_result
jta_draw_frame(
        jta_vulkan_window_context* wnd_ctx, mtx4 view_matrix, jta_structure_meshes* meshes,
        const jta_camera_3d* camera);

#endif //JTA_DRAWING_3D_H
