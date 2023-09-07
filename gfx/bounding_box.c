//
// Created by jan on 6.7.2023.
//
#include "bounding_box.h"
#include <jdm.h>

gfx_result gfx_find_bounding_sphere(const jta_point_list* points, vec4* origin, f32* radius)
{
    JDM_ENTER_FUNCTION;
    vec4 o;


    f32 min_bbx = +INFINITY, max_bbx = -INFINITY,
            min_bby = +INFINITY, max_bby = -INFINITY,
            min_bbz = +INFINITY, max_bbz = -INFINITY;
    for (u32 i = 0; i < points->count; ++i)
    {
        vec4 pos = VEC4(points->p_x[i], points->p_y[i], points->p_z[i]);
        if (pos.x < min_bbx)
        {
            min_bbx = pos.x;
        }
        if (pos.x > max_bbx)
        {
            max_bbx = pos.x;
        }
        if (pos.y < min_bby)
        {
            min_bby = pos.y;
        }
        if (pos.y > max_bby)
        {
            max_bby = pos.y;
        }
        if (pos.z < min_bbz)
        {
            min_bbz = pos.z;
        }
        if (pos.z > max_bbz)
        {
            max_bbz = pos.z;
        }
    }
    //  Make sure that in some (degenerate cases) there's at least some sensible bounding boxes
    if (min_bbx == max_bbx)
    {
        min_bbx -= 0.5f;
        max_bbx += 0.5f;
    }
    if (min_bby == max_bby)
    {
        min_bby -= 0.5f;
        max_bby += 0.5f;
    }
    if (min_bbz == max_bbz)
    {
        min_bbz -= 0.5f;
        max_bbz += 0.5f;
    }

    o = vec4_div_one(vec4_add(VEC4(max_bbx, max_bby, max_bbz), VEC4(min_bbx, min_bby, min_bbz)), 2.0f);

    *origin = o;
    *radius = vec4_magnitude(vec4_sub(VEC4(max_bbx, max_bby, max_bbz), VEC4(min_bbx, min_bby, min_bbz)));

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result gfx_find_bounding_planes(const jta_point_list* point_list, vec4 origin, vec4 unit_view_dir, f32* p_near, f32* p_far)
{
    JDM_ENTER_FUNCTION;

    f32 min = +INFINITY, max = 0.0f;
    u32 i;
    __m128 ox = _mm_set1_ps(origin.x);
    __m128 oy = _mm_set1_ps(origin.y);
    __m128 oz = _mm_set1_ps(origin.z);
    __m128 vx = _mm_set1_ps(unit_view_dir.x);
    __m128 vy = _mm_set1_ps(unit_view_dir.y);
    __m128 vz = _mm_set1_ps(unit_view_dir.z);
    for (i = 0; i < (point_list->count >> 2); ++i)
    {
        __m128 x = _mm_loadu_ps(point_list->p_x + (i << 2));
        __m128 y = _mm_loadu_ps(point_list->p_y + (i << 2));
        __m128 z = _mm_loadu_ps(point_list->p_z + (i << 2));

        __m128 dx = _mm_sub_ps(x, ox);
        __m128 dy = _mm_sub_ps(y, oy);
        __m128 dz = _mm_sub_ps(z, oz);

        __m128 d = _mm_mul_ps(dx, vx);
        d = _mm_add_ps(d, _mm_mul_ps(dy, vy));
        d = _mm_add_ps(d, _mm_mul_ps(dz, vz));

        _Alignas(16) f32 array[4];
        _mm_store_ps(array, d);
        for (u32 j = 0; j < 4; ++j)
        {
            f32 v_min = d[j] - point_list->max_radius[i];
            f32 v_max = d[j] + point_list->max_radius[i];
            if (v_min < min)
            {
                min = v_min;
            }
            if (v_max > max)
            {
                max = v_max;
            }
        }
    }
    for (i <<= 2; i < point_list->count; ++i)
    {
        f32 dx = point_list->p_x[i] - origin.x;
        f32 dy = point_list->p_y[i] - origin.y;
        f32 dz = point_list->p_z[i] - origin.z;
        f32 d = dx * unit_view_dir.x + dy * unit_view_dir.y + dz * unit_view_dir.z;

        f32 v_min = d - point_list->max_radius[i];
        f32 v_max = d + point_list->max_radius[i];
        if (v_min < min)
        {
            min = v_min;
        }
        if (v_max > max)
        {
            max = v_max;
        }
    }

    if (min == max)
    {
        max += 1.0f;
    }
    if (min < 1e-2f)
    {
        min = 1e-2f;
    }

    *p_far = max;
    *p_near = min;
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

void jta_bounding_box_add_point(jta_bounding_box* bbox, vec4 pos, float r)
{
    const float min_x = pos.x - r;
    const float min_y = pos.y - r;
    const float min_z = pos.z - r;
    const float max_x = pos.x + r;
    const float max_y = pos.y + r;
    const float max_z = pos.z + r;

    if (max_x > bbox->max_x)
    {
        bbox->max_x = max_x;
    }
    if (max_y > bbox->max_y)
    {
        bbox->max_y = max_y;
    }
    if (max_z > bbox->max_z)
    {
        bbox->max_z = max_z;
    }

    if (min_x < bbox->min_x)
    {
        bbox->min_x = min_x;
    }
    if (min_y < bbox->min_y)
    {
        bbox->min_y = min_y;
    }
    if (min_z < bbox->min_z)
    {
        bbox->min_z = min_z;
    }
}
