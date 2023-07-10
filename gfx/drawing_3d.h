//
// Created by jan on 18.5.2023.
//

#ifndef JTA_DRAWING_3D_H
#define JTA_DRAWING_3D_H
#include "../mem/aligned_jalloc.h"
#include <jmem/jmem.h>

#include "../jfw/window.h"
//#include "../jfw/error_system/error_codes.h"


#include "vk_state.h"
#include "camera.h"
#include "gfxerr.h"
#include "mesh.h"



gfx_result
draw_frame(
        vk_state* state, jfw_window_vk_resources* vk_resources, u32 n_meshes, jta_mesh** meshes,
        const jta_camera_3d* camera);


#endif //JTA_DRAWING_3D_H
