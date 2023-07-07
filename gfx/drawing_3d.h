//
// Created by jan on 18.5.2023.
//

#ifndef JTB_DRAWING_3D_H
#define JTB_DRAWING_3D_H
#include "../mem/aligned_jalloc.h"
#include <jmem/jmem.h>

#include "../jfw/window.h"
//#include "../jfw/error_system/error_codes.h"


#include "vk_state.h"
#include "camera.h"
#include "gfxerr.h"
#include "mesh.h"



gfx_result draw_3d_scene(
        jfw_window* wnd, vk_state* state, jfw_window_vk_resources* vk_resources, vk_buffer_allocation* p_buffer_geo,
        vk_buffer_allocation* p_buffer_mod, const jtb_mesh* mesh,
        const jtb_camera_3d* camera);


#endif //JTB_DRAWING_3D_H
