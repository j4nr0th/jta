//
// Created by jan on 6.7.2023.
//

#ifndef JTA_BOUNDING_BOX_H
#define JTA_BOUNDING_BOX_H
#include "gfxerr.h"
#include "../solver/jtapoints.h"

gfx_result gfx_find_bounding_sphere(const jta_point_list* points, vec4* origin, f32* radius);


gfx_result gfx_find_bounding_planes(const jta_point_list* point_list, vec4 origin, vec4 unit_view_dir, f32* p_near, f32* p_far);


#endif //JTA_BOUNDING_BOX_H
