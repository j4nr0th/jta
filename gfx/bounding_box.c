//
// Created by jan on 6.7.2023.
//
#include "bounding_box.h"

gfx_result gfx_find_bounding_box(u32 n_points, const jtb_point* points)
{
    JDM_ENTER_FUNCTION;
    vec4 bb_e1, bb_e2, bb_e3, origin;
    f32 d1 = 0.0f, d2 = 0.0f, d3 = 0.0f;
    u32 p1 = 0, p2 = 0, p3 = 0, p4 = 0;

    //  Find the two points which are the furthest apart
    for (u32 i = 0; i < n_points; ++i)
    {
        vec4 r1 = VEC4(points[i].x, points[i].y, points[i].z);
        for (u32 j = i + 1; j < n_points; ++j)
        {
            vec4 r2 = VEC4(points[j].x, points[j].y, points[j].z);
            f32 d = vec4_magnitude(vec4_sub(r1, r2));
            if (d < d1)
            {
                d1 = d;
                p1 = i;
                p2 = j;
            }
        }
    }
    //  Principal vector 1
    bb_e1 = vec4_unit(vec4_sub(
            VEC4(points[p1].x, points[p1].y, points[p1].z),
            VEC4(points[p2].x, points[p2].y, points[p2].z)
                              ));

    //  Find the two which are the furthest apart in the plane perpendicular to bb_e1
    for (u32 i = 0; i < n_points; ++i)
    {
        vec4 r3 = VEC4(points[i].x, points[i].y, points[i].z);
        for (u32 j = i + 1; j < n_points; ++j)
        {
            vec4 r4 = VEC4(points[j].x, points[j].y, points[j].z);
            vec4 dif = vec4_sub(r3, r4);                //  Displacement between the two points
            f32 dis2 = vec4_dot(dif, dif);                //  Length of displacement squared
            f32 from_plane = vec4_dot(dif, bb_e1);      //  Distance from the plane
            f32 d = sqrtf(dis2 - from_plane * from_plane);//  Distance in the plane
            if (d < d2)
            {
                d2 = d;
                p3 = i;
                p4 = j;
            }
        }
    }
    vec4 d_34 = vec4_sub(VEC4(points[p3].x, points[p3].y, points[p3].z), VEC4(points[p4].x, points[p4].y, points[p4].z));
    vec4 v_e3 = vec4_cross(d_34, bb_e1);
    if (vec4_magnitude(v_e3) == 0.0f)
    {
        d2 = d1;
        d3 = d1;
        if (bb_e1.y == 0.0f)
        {
            bb_e2 = vec4_unit(vec4_cross(bb_e1, VEC4(0, 1, 0)));
        }
        else if (bb_e1.z == 0.0f)
        {
            bb_e2 = vec4_unit(vec4_cross(bb_e1, VEC4(0, 0, 1)));
        }
        else
        {
            bb_e2 = vec4_unit(vec4_cross(bb_e1, VEC4(1, 0, 0)));
        }
        bb_e3 = vec4_unit(vec4_cross(bb_e1, bb_e2));
    }
    else
    {
        bb_e3 = vec4_unit(v_e3);
        bb_e2 = vec4_unit(vec4_cross(bb_e3, bb_e1));
    }

    //  Transformation from the global coordinate system to bounding box system
    const mtx4 t_mat =
            {
                    .col0 = { .x = bb_e1.x, .y = bb_e1.y, .z = bb_e1.z, .w = 0},
                    .col1 = { .x = bb_e2.x, .y = bb_e2.y, .z = bb_e2.z, .w = 0},
                    .col2 = { .x = bb_e3.x, .y = bb_e3.y, .z = bb_e3.z, .w = 0},
                    .col3 = { .x = 0, .y = 0, .z = 0, .w = 1},
            };

    //  Transform points into the correct coordinate frame
    f32 min_bbx = +INFINITY, max_bbx = -INFINITY,
            min_bby = +INFINITY, max_bby = -INFINITY,
            min_bbz = +INFINITY, max_bbz = -INFINITY;
    for (u32 i = 0; i < n_points; ++i)
    {
        vec4 pos = mtx4_vector_mul(t_mat, VEC4(points[i].x, points[i].y, points[i].z));
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
        min_bby -= (max_bbx - min_bbx);
        max_bby += (max_bbx - min_bbx);
    }
    if (min_bbz == max_bbz)
    {
        min_bbz -= (max_bbx - min_bbx);
        max_bbz += (max_bbx - min_bbx);
    }


    origin = vec4_div_one(vec4_add(VEC4(max_bbx, max_bby, max_bbz), VEC4(min_bbx, min_bby, min_bbz)), 2.0f);


    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result gfx_find_bounding_sphere(u32 n_points, const jtb_point* points, vec4* origin, f32* radius)
{
    JDM_ENTER_FUNCTION;
    vec4 o;
    f32 r = 0.0f;


    f32 min_bbx = +INFINITY, max_bbx = -INFINITY,
            min_bby = +INFINITY, max_bby = -INFINITY,
            min_bbz = +INFINITY, max_bbz = -INFINITY;
    for (u32 i = 0; i < n_points; ++i)
    {
        vec4 pos = VEC4(points[i].x, points[i].y, points[i].z);
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

gfx_result gfx_convert_points_to_geometry_contents(u32 n_points, const jtb_point* points, geometry_contents* p_out)
{
    JDM_ENTER_FUNCTION;
    f32* const x = jalloc(G_JALLOCATOR, sizeof(*x) * n_points);
    if (!x)
    {
        JDM_ERROR("Could not allocate memory for point's x coordinates");
        goto end;
    }
    f32* const y = jalloc(G_JALLOCATOR, sizeof(*y) * n_points);
    if (!y)
    {
        jfree(G_JALLOCATOR, x);
        JDM_ERROR("Could not allocate memory for point's y coordinates");
        goto end;
    }
    f32* z = jalloc(G_JALLOCATOR, sizeof(*z) * n_points);
    if (!z)
    {
        jfree(G_JALLOCATOR, x);
        jfree(G_JALLOCATOR, y);
        JDM_ERROR("Could not allocate memory for point's z coordinates");
    }

    for (u32 i = 0; i < n_points; ++i)
    {
        x[i] = (f32)points[i].x;
        y[i] = (f32)points[i].y;
        z[i] = (f32)points[i].z;
    }

    *p_out = (geometry_contents){.n_points = n_points, .x = x, .y = y, .z = z};


    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_BAD_ALLOC;
}

gfx_result gfx_find_bounding_planes(const geometry_contents* geometry, vec4 origin, vec4 unit_view_dir, f32* p_near, f32* p_far)
{
    JDM_ENTER_FUNCTION;

    f32 min = +INFINITY, max = -INFINITY;
    u32 i;
    __m128 ox = _mm_set1_ps(origin.x);
    __m128 oy = _mm_set1_ps(origin.y);
    __m128 oz = _mm_set1_ps(origin.z);
    __m128 vx = _mm_set1_ps(unit_view_dir.x);
    __m128 vy = _mm_set1_ps(unit_view_dir.y);
    __m128 vz = _mm_set1_ps(unit_view_dir.z);
    for (i = 0; i < (geometry->n_points >> 2); ++i)
    {
        __m128 x = _mm_loadu_ps(geometry->x + (i << 2));
        __m128 y = _mm_loadu_ps(geometry->y + (i << 2));
        __m128 z = _mm_loadu_ps(geometry->z + (i << 2));

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
            const f32 v = d[j] > 0.0f ? d[j] : 0.0f;
            if (v < min)
            {
                min = v;
            }
            if (v > max)
            {
                max = v;
            }
        }
    }
    for (i <<= 2; i < geometry->n_points; ++i)
    {
        f32 dx = geometry->x[i] - origin.x;
        f32 dy = geometry->y[i] - origin.y;
        f32 dz = geometry->z[i] - origin.z;
        f32 d = dx * unit_view_dir.x + dy * unit_view_dir.y + dz * unit_view_dir.z;
        if (d < 0.0f)
        {
            d = 0.0f;
        }
        if (d < min)
        {
            min = d;
        }
        if (d > max)
        {
            max = d;
        }
    }

    if (min == max)
    {
        max += 1.0f;
    }
    if (min == 0.0f)
    {
        min += 1e-5f;
    }

    *p_far = max;
    *p_near = min;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}
