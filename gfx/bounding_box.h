//
// Created by jan on 6.7.2023.
//

#ifndef JTA_BOUNDING_BOX_H
#define JTA_BOUNDING_BOX_H
#include "gfxerr.h"
#include "../core/jtapoints.h"

//  Very simple Cartesian bounding box struct. Purpose is to quickly find near-far planes which work well.
struct jta_bounding_box_T
{
    float min_x, max_x;
    float min_y, max_y;
    float min_z, max_z;
};

typedef struct jta_bounding_box_T jta_bounding_box;


void jta_bounding_box_add_point(jta_bounding_box* bbox, vec4 pos, float r);


gfx_result gfx_find_bounding_sphere(const jta_point_list* points, vec4* origin, f32* radius);


gfx_result gfx_find_bounding_planes(const jta_point_list* point_list, vec4 origin, vec4 unit_view_dir, f32* p_near, f32* p_far);


#endif //JTA_BOUNDING_BOX_H
