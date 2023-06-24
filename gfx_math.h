//
// Created by jan on 18.4.2023.
//

#ifndef JTB_GFX_MATH_H
#define JTB_GFX_MATH_H
#include "jfw/jfw_common.h"
#include <immintrin.h>
#include <ammintrin.h>
#include <math.h>
#include <float.h>

typedef union
{
    _Alignas(16) f32 data[4];
    struct {f32 x, y, z, w;};
    struct {f32 s0, s1, s2, s3;};
} vec4;

typedef union
{
    _Alignas(16) f32 data[16];
    struct
    {
        vec4 col0;
        vec4 col1;
        vec4 col2;
        vec4 col3;
    };
    struct
    {
        f32 xx, xy, xz, xw;
        f32 yx, yy, yz, yw;
        f32 zx, zy, zz, zw;
        f32 wx, wy, wz, ww;
    };
    struct
    {
        f32 s00, s01, s02, s03;
        f32 s10, s11, s12, s13;
        f32 s20, s21, s22, s23;
        f32 s30, s31, s32, s33;
    };
    f32 m[4][4];
} mtx4;



static inline vec4 vec4_add(vec4 a, vec4 b)
{
    assert(a.w == 1.0f);
    assert(b.w == 1.0f);
    b.w = 0;
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_load_ps(b.data);

    vc = _mm_add_ps(va, vb);
    _mm_store_ps(c.data, vc);
    return c;
}

static inline vec4 vec4_sub(vec4 a, vec4 b)
{
    assert(a.w == 1.0f);
    assert(b.w == 1.0f);
    b.w = 0;
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_load_ps(b.data);

    vc = _mm_sub_ps(va, vb);
    _mm_store_ps(c.data, vc);
    return c;
}

static inline vec4 vec4_mul(vec4 a, vec4 b)
{
    assert(a.w == 1.0f);
    assert(b.w == 1.0f);
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_load_ps(b.data);

    vc = _mm_mul_ps(va, vb);
    _mm_store_ps(c.data, vc);
    return c;
}

static inline vec4 vec4_div(vec4 a, vec4 b)
{
    assert(a.w == 1.0f);
    assert(b.w == 1.0f);
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_load_ps(b.data);

    vc = _mm_mul_ps(va, vb);
    _mm_store_ps(c.data, vc);
    return c;
}

static inline vec4 vec4_add_one(vec4 a, f32 k)
{
    assert(a.w == 1.0f);
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_set1_ps(k);

    vc = _mm_add_ps(va, vb);
    _mm_store_ps(c.data, vc);
    c.w = 1;
    return c;
}

static inline vec4 vec4_sub_one(vec4 a, f32 k)
{
    assert(a.w == 1.0f);
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_set_ps1(k);

    vc = _mm_sub_ps(va, vb);
    _mm_store_ps(c.data, vc);
    c.w = 1;
    return c;
}

static inline vec4 vec4_mul_one(vec4 a, f32 k)
{
    assert(a.w == 1.0f);
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_set_ps1(k);

    vc = _mm_mul_ps(va, vb);
    _mm_store_ps(c.data, vc);
    c.w = 1;
    return c;
}

static inline vec4 vec4_div_one(vec4 a, f32 k)
{
    assert(a.w == 1.0f);
    vec4 c;
    __m128 va, vb, vc;
    va = _mm_load_ps(a.data);
    vb = _mm_set_ps1(k);

    vc = _mm_mul_ps(va, vb);
    _mm_store_ps(c.data, vc);
    c.w = 1;
    return c;
}

static inline f32 vec4_dot(vec4 a, vec4 b)
{
    assert(a.w == 1.0f);
    assert(b.w == 1.0f);
    f32 dot;

    dot = a.x * b.x + a.y * b.y + a.z * b.z;

    return dot;
}

static inline vec4 vec4_cross(vec4 a, vec4 b)
{
    assert(a.w == 1.0f);
    assert(b.w == 1.0f);
    vec4 crs;

    crs.x = a.y * b.z - a.z * b.y;
    crs.y = a.z * b.x - a.x * b.z;
    crs.z = a.x * b.y - a.y * b.x;
    crs.w = 1.0f;

    return crs;
}

static inline vec4 vec4_unit(vec4 v)
{
    assert(v.w == 1.0f);
    f32 mag;

    mag = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    return (vec4){.x = v.x / mag, .y = v.y / mag, .z = v.z / mag, .w = 1.0f};
}

static inline f32 vec4_magnitude(vec4 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline mtx4 mtx4_rotation_z(f32 alpha)
{
    mtx4 m;
    const f32 c = cosf(alpha);
    const f32 s = sinf(alpha);

    m = (mtx4){
        .data =
        {
                   c,   -s, 0.0f, 0.0f,
                  +s,    c, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f,
        }
    };

    return m;
}

static inline mtx4 mtx4_rotation_y(f32 beta)
{
    mtx4 m;
    const f32 c = cosf(beta);
    const f32 s = sinf(beta);

    m = (mtx4){
            .data =
                    {
                           c, 0.0f,   +s, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                          -s, 0.0f,    c, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f,
                    }
    };

    return m;
}

static inline mtx4 mtx4_rotation_x(f32 gamma)
{
    mtx4 m;
    const f32 c = cosf(gamma);
    const f32 s = sinf(gamma);

    m = (mtx4){
            .data =
                    {
                            1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f,    c,   -s, 0.0f,
                            0.0f,   +s,    c, 0.0f,
                            0.0f, 0.0f, 0.0f, 1.0f,
                    }
    };
    return m;
}

static inline mtx4 mtx4_multiply_manual(mtx4 a, mtx4 b)
{
    mtx4 res;
    for (u32 i = 0; i < 4; ++i)
    {
        for (u32 j = 0; j < 4; ++j)
        {
            f32 v = 0;
            for (u32 k = 0; k < 4; ++k)
            {
                v += a.data[4 * k + i] * b.data[4 * j + k];
            }
            res.data[4 * j + i] = v;
        }
    }
    return res;
}

static inline vec4 mtx4_vector_mul(mtx4 m, vec4 x)
{
//    assert(x.w == 1.0f);
    vec4 y;
    __m128 x_element = _mm_set1_ps(x.s0);
    __m128 col = _mm_load_ps(m.col0.data);
    __m128 res = _mm_mul_ps(col, x_element);

    col = _mm_load_ps(m.col1.data);
    x_element = _mm_set1_ps(x.s1);
    res = _mm_add_ps(res, _mm_mul_ps(col, x_element));

    col = _mm_load_ps(m.col2.data);
    x_element = _mm_set1_ps(x.s2);
    res = _mm_add_ps(res, _mm_mul_ps(col, x_element));

    col = _mm_load_ps(m.col3.data);
    x_element = _mm_set1_ps(x.s3);
    res = _mm_add_ps(res, _mm_mul_ps(col, x_element));
    _mm_store_ps(y.data, res);
    return y;
}

static inline mtx4 mtx4_multiply(mtx4 a, mtx4 b)
{
    mtx4 c;
    c.col0 = mtx4_vector_mul(a, b.col0);
    c.col1 = mtx4_vector_mul(a, b.col1);
    c.col2 = mtx4_vector_mul(a, b.col2);
    c.col3 = mtx4_vector_mul(a, b.col3);
    return c;
}

static const mtx4 mtx4_identity =
{
        .data =
           {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
           }
};

static inline mtx4 mtx4_translate(mtx4 m, vec4 offset)
{
    mtx4 r = m;
    r.col3.x -= offset.x;
    r.col3.y -= offset.y;
    r.col3.z -= offset.z;

    return r;
}

static inline mtx4 mtx4_negative(mtx4 m)
{
    return (mtx4)
            {
        .data[ 0] = -m.data[ 0],
        .data[ 1] = -m.data[ 1],
        .data[ 2] = -m.data[ 2],
        .data[ 3] = -m.data[ 3],
        .data[ 4] = -m.data[ 4],
        .data[ 5] = -m.data[ 5],
        .data[ 6] = -m.data[ 6],
        .data[ 7] = -m.data[ 7],
        .data[ 8] = -m.data[ 8],
        .data[ 9] = -m.data[ 9],
        .data[10] = -m.data[10],
        .data[11] = -m.data[11],
        .data[12] = -m.data[12],
        .data[13] = -m.data[13],
        .data[14] = -m.data[14],
        .data[15] = -m.data[15],
            };
}

static vec4 vec4_negative(vec4 v)
{
    return (vec4){.x = -v.x, .y = -v.y, .z = -v.z, .w = 1.0f};
}

static inline mtx4 mtx4_view_matrix(vec4 offset, vec4 view_direction, f32 roll)
{
    mtx4 m = mtx4_translate(mtx4_identity, offset);

    f32 theta, phi;
    theta = atan2f(view_direction.y, view_direction.x);
    phi = acosf(view_direction.z / vec4_magnitude(view_direction));
//    phi = acosf(view_direction.z / sqrtf(view_direction.x * view_direction.x + view_direction.y * view_direction.y));
    m = mtx4_multiply(mtx4_rotation_z(-theta), m);
    m = mtx4_multiply(mtx4_rotation_y(-phi), m);
    m = mtx4_multiply(mtx4_rotation_z(roll), m);

    return m;
}

static inline mtx4 mtx4_view_look_at(vec4 pos, vec4 target, vec4 down)
{
    vec4 d = vec4_sub(target, pos);
    assert(vec4_magnitude(d) > 0.0f);
    assert(vec4_magnitude(down) > 0.0f);
    vec4 right_vector = vec4_cross((down), d);
    assert(vec4_magnitude(right_vector) > 0.0f);
    vec4 unit_y = vec4_cross(d, right_vector);
    //  normalize
    vec4 unit_x = vec4_unit(right_vector);
    unit_y = vec4_unit(unit_y);
    vec4 unit_z = vec4_unit(d);
    return (mtx4)
            {
        .col0 = {unit_x.x, unit_y.x, unit_z.x, 0},
        .col1 = {unit_x.y, unit_y.y, unit_z.y, 0},
        .col2 = {unit_x.z, unit_y.z, unit_z.z, 0},
        .col3 = {-pos.x, -pos.y, -pos.z, 1},
            };
}

static inline mtx4 mtx4_view_matrix_inverse(vec4 d, f32 roll)
{
    f32 theta, phi;
    theta = atan2f(d.y, d.x);
    phi = acosf(d.z / vec4_magnitude(d));
    mtx4 m = mtx4_identity;
    m = mtx4_multiply(mtx4_rotation_z(-roll), m);
    m = mtx4_multiply(mtx4_rotation_y(-phi), m);
    m = mtx4_multiply(mtx4_rotation_z(-theta), m);

    return m;
}

static inline mtx4 mtx4_projection(f32 fov, f32 ar, f32 zoom, f32 near, f32 far)
{
    const f32 k = 2 * zoom / tanf(fov / 2);
    assert(far > near);
    const f32 z_dif = far - near;
    return (mtx4)
    {
        .data = {
                k, 0, 0, 0,
                0, k*ar, 0, 0,
                0, 0, far / z_dif, 1,
                0, 0, -far * near / z_dif, 0,
                }
    };
}

static inline mtx4 mtx4_enlarge(f32 x_factor, f32 y_factor, f32 z_factor)
{
    return (mtx4)
            {
        .data =
                {
                x_factor, 0, 0, 0,
                0, y_factor, 0, 0,
                0, 0, z_factor, 0,
                0, 0, 0,        1,
                }
            };
}

static mtx4 mtx4_transpose(mtx4 m)
{
    for (u32 i = 0; i < 4; ++i)
    {
        for (u32 j = 0; j < i; ++j)
        {
            const f32 tmp = m.m[i][j];
            m.m[i][j] = m.m[j][i];
            m.m[j][i] = tmp;
        }
    }

    return m;
}

static inline mtx4 mtx4_rotate_around_axis(vec4 axis_of_rotation, f32 rotation_angle)
{
    //  Align the rotation vector with the z axis
    f32 theta, phi;
    theta = atan2f(axis_of_rotation.y, axis_of_rotation.x);
    phi = acosf(axis_of_rotation.z / vec4_magnitude(axis_of_rotation));
    mtx4 mf = mtx4_multiply(mtx4_rotation_y(phi), mtx4_rotation_z(theta));
    mtx4 m = mtx4_multiply(mtx4_rotation_z(-rotation_angle), mf);
    m = mtx4_multiply(mtx4_transpose(mf), m);
    return m;
}

static int vec4_equal(vec4 v1, vec4 v2)
{
    assert(v1.w == 1.0f);
    assert(v2.w == 1.0f);

    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

static int vec4_close(vec4 v1, vec4 v2)
{
    assert(v1.w == 1.0f);
    assert(v2.w == 1.0f);
    if (vec4_equal(v1, v2))
    {
        return 1;
    }

    f32 dx = fabsf((v1.x - v2.x));
    f32 dy = fabsf((v1.y - v2.y));
    f32 dz = fabsf((v1.z - v2.z));

    return dx < 1024 * FLT_EPSILON && dy < 1024 * FLT_EPSILON && dz < 1024 * FLT_EPSILON;
}

#define VEC4(vx, vy, vz) (vec4){.x = (vx), .y = (vy), .z = (vz), .w = 1.0f}
#endif //JTB_GFX_MATH_H
