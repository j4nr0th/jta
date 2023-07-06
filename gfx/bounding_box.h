//
// Created by jan on 6.7.2023.
//

#ifndef JTB_BOUNDING_BOX_H
#define JTB_BOUNDING_BOX_H
#include "gfxerr.h"
#include "../solver/jtbpoints.h"

gfx_result gfx_find_bounding_box(u32 n_points, const jtb_point* points);

gfx_result gfx_find_bounding_sphere(u32 n_points, const jtb_point* points, vec4* origin, f32* radius);

typedef struct geometry_contents_struct geometry_contents;
struct geometry_contents_struct
{
    u32 n_points;
    f32 *x;
    f32 *y;
    f32 *z;
};

gfx_result gfx_convert_points_to_geometry_contents(u32 n_points, const jtb_point* points, geometry_contents* p_out);

gfx_result gfx_find_bounding_planes(const geometry_contents* geometry, vec4 origin, vec4 unit_view_dir, f32* p_near, f32* p_far);


#endif //JTB_BOUNDING_BOX_H
