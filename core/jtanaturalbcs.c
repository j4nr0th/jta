//
// Created by jan on 9.7.2023.
//

#include "jtanaturalbcs.h"


static const jio_string_segment NATURAL_BC_FILE_HEADERS[] =
        {
                {.begin = "point label", .len = sizeof("point label") - 1},
                {.begin = "Fx", .len = sizeof("Fx") - 1},
                {.begin = "Fy", .len = sizeof("Fy") - 1},
                {.begin = "Fz", .len = sizeof("Fz") - 1},
        };

static const size_t NATURAL_BC_FILE_HEADER_COUNT = sizeof(NATURAL_BC_FILE_HEADERS) / sizeof(*NATURAL_BC_FILE_HEADERS);

typedef struct natural_bc_parse_float_data_struct natural_bc_parse_float_data;
struct natural_bc_parse_float_data_struct
{
    uint32_t count;
    f32* values;
};

typedef struct natural_bc_parse_ss_data_struct natural_bc_parse_ss_data;
struct natural_bc_parse_ss_data_struct
{
    uint32_t count;
    const jta_point_list* point_list;
    uint32_t* values;
};

static bool converter_natural_bc_point_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    natural_bc_parse_ss_data* const data = (natural_bc_parse_ss_data*)param;
    uint32_t i;
    for (i = 0; i < data->point_list->count; ++i)
    {
        if (jio_string_segment_equal(data->point_list->label + i, v))
        {
            break;
        }
    }
    if (i == data->point_list->count)
    {
        JDM_ERROR("Natural boundary condition was to be applied at point with label \"%.*s\", but there is no point with such a label", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->values[data->count++] = i;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_natural_bc_float_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    natural_bc_parse_float_data* const data = (natural_bc_parse_float_data*)param;
    char* end;
    const f32 v_d = strtof(v->begin, &end);
    if (v->begin + v->len != end && v->len != 0)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->values[data->count++] = v_d;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_natural_bc_point_label_function,
                converter_natural_bc_float_function,
                converter_natural_bc_float_function,
                converter_natural_bc_float_function,
        };

jta_result jta_load_natural_boundary_conditions(
        const jio_memory_file* mem_file, const jta_point_list* point_list, jta_natural_boundary_condition_list* bcs)
{
    JDM_ENTER_FUNCTION;
    jta_result res;
    uint32_t line_count = 0;
    jio_result jio_res = jio_memory_file_count_lines(mem_file, &line_count);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not count lines in natural boundary condition input file, reason: %s", jio_result_to_str(jio_res));
        res = JTA_RESULT_BAD_IO;
        goto end;
    }

    f32* x = ill_jalloc(G_JALLOCATOR, sizeof(*x) * (line_count - 1));
    if (!x)
    {
        JDM_ERROR("Could not allocate memory for natural boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* y = ill_jalloc(G_JALLOCATOR, sizeof(*y) * (line_count - 1));
    if (!y)
    {
        ill_jfree(G_JALLOCATOR, x);
        JDM_ERROR("Could not allocate memory for natural boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* z = ill_jalloc(G_JALLOCATOR, sizeof(*z) * (line_count - 1));
    if (!z)
    {
        ill_jfree(G_JALLOCATOR, x);
        ill_jfree(G_JALLOCATOR, y);
        JDM_ERROR("Could not allocate memory for natural boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    uint32_t* i_pts = ill_jalloc(G_JALLOCATOR, sizeof(*i_pts) * (line_count - 1));
    if (!i_pts)
    {
        ill_jfree(G_JALLOCATOR, x);
        ill_jfree(G_JALLOCATOR, y);
        ill_jfree(G_JALLOCATOR, z);
        JDM_ERROR("Could not allocate memory for natural boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    natural_bc_parse_float_data float_data[] =
            {
                    {.values = x, .count = 0},
                    {.values = y, .count = 0},
                    {.values = z, .count = 0},
            };
    natural_bc_parse_ss_data ss_data = {.values = i_pts, .point_list = point_list, .count = 0};
    void* param_array[] = { &ss_data, float_data + 0, float_data + 1, float_data + 2, };
    jio_stack_allocator_callbacks jio_callbacks =
            {
                    .param = G_LIN_JALLOCATOR,
                    .alloc = (void* (*)(void*, uint64_t)) lin_jalloc,
                    .free = (void (*)(void*, void*)) lin_jfree,
                    .realloc = (void* (*)(void*, void*, uint64_t)) lin_jrealloc,
                    .restore = (void (*)(void*, void*)) lin_jallocator_restore_current,
                    .save = (void* (*)(void*)) lin_jallocator_save_state,
            };
    jio_res = jio_process_csv_exact(mem_file, ",", NATURAL_BC_FILE_HEADER_COUNT, NATURAL_BC_FILE_HEADERS, converter_functions, param_array, &jio_callbacks);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the natural boundary conditions input file failed, reason: %s", jio_result_to_str(jio_res));
        ill_jfree(G_JALLOCATOR, i_pts);
        ill_jfree(G_JALLOCATOR, x);
        ill_jfree(G_JALLOCATOR, y);
        ill_jfree(G_JALLOCATOR, z);
        res = JTA_RESULT_BAD_INPUT;
        goto end;
    }
    assert(float_data[0].count == float_data[1].count);
    assert(float_data[1].count == float_data[2].count);
    assert(float_data[1].count == ss_data.count);


    const uint32_t count = float_data[0].count;
    assert(count <= line_count - 1);

    if (count != line_count - 1)
    {
        f32* new_ptr = ill_jrealloc(G_JALLOCATOR, x, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the natural boundary condition array from %zu to %zu bytes", sizeof(*x) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            x = new_ptr;
        }
        new_ptr = ill_jrealloc(G_JALLOCATOR, y, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the natural boundary condition array from %zu to %zu bytes", sizeof(*y) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            y = new_ptr;
        }
        new_ptr = ill_jrealloc(G_JALLOCATOR, z, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the natural boundary condition array from %zu to %zu bytes", sizeof(*z) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            z = new_ptr;
        }
        uint32_t* const new_ptr1 = ill_jrealloc(G_JALLOCATOR, i_pts, sizeof(*new_ptr1) * count);
        if (!new_ptr1)
        {
            JDM_WARN("Failed shrinking the natural boundary condition array from %zu to %zu bytes", sizeof(*i_pts) * (line_count - 1), sizeof(*new_ptr1) * count);
        }
        else
        {
            i_pts = new_ptr1;
        }
    }

    f32 min_mag = +INFINITY, max_mag = -INFINITY;
    u32 i = 0;
    for (i = 0; i < (count >> 2); ++i)
    {
        __m128 vx = _mm_loadu_ps(x + (i << 2));
        __m128 vy = _mm_loadu_ps(y + (i << 2));
        __m128 vz = _mm_loadu_ps(z + (i << 2));
        __m128 hyp = _mm_mul_ps(vx, vx);
//        hyp = _mm_fmadd_ps(vy, vy, hyp);
//        hyp = _mm_fmadd_ps(vz, vz, hyp);
        hyp = _mm_add_ps(_mm_mul_ps(vy, vy), hyp);
        hyp = _mm_add_ps(_mm_mul_ps(vz, vz), hyp);
        _Alignas(16) f32 values[4];
        _mm_store_ps(values, hyp);
        for (u32 j = 0; j < 4; ++j)
        {
            const f32 v = values[i];
            if (v < min_mag)
            {
                min_mag = v;
            }
            if (v > max_mag)
            {
                max_mag = v;
            }
        }
    }

    for (i <<= 2; i < count; ++i)
    {
        f32 v = hypotf(x[i], hypotf(y[i], z[i]));
        v = (v * v);
        if (v < min_mag)
        {
            min_mag = v;
        }
        if (v > max_mag)
        {
            max_mag = v;
        }
    }
    min_mag = sqrtf(min_mag);
    max_mag = sqrtf(max_mag);

    *bcs = (jta_natural_boundary_condition_list){.count = count, .x = x, .y = y, .z = z, .i_point = i_pts, .min_mag = min_mag, .max_mag = max_mag};

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
    end:
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_free_natural_boundary_conditions(jta_natural_boundary_condition_list* bcs)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_JALLOCATOR, bcs->i_point);
    ill_jfree(G_JALLOCATOR, bcs->x);
    ill_jfree(G_JALLOCATOR, bcs->y);
    ill_jfree(G_JALLOCATOR, bcs->z);

    JDM_LEAVE_FUNCTION;
}
