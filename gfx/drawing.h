//
// Created by jan on 18.5.2023.
//

#ifndef JTA_DRAWING_H
#define JTA_DRAWING_H
#include <jmem/jmem.h>
#include <jrui.h>



//#include "vk_state.h"
#include "camera.h"
#include "gfxerr.h"
#include "mesh.h"
#include "vk_resources.h"
#include "../ui/jwin_handlers.h"
#include "../jta_state.h"
gfx_result
jta_draw_frame(
        jta_vulkan_window_context* wnd_ctx, jta_ui_state* ui_state, mtx4 view_matrix, jta_structure_meshes* deformed_meshes, jta_structure_meshes* undeformed_meshes,
        const jta_camera_3d* camera);
;

#endif //JTA_DRAWING_H
