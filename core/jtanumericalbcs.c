//
// Created by jan on 10.7.2023.
//

#include "jtanumericalbcs.h"
#include <jdm.h>

static const jio_string_segment NUMERICAL_BC_FILE_HEADERS[] =
        {
                {.begin = "point label", .len = sizeof("point label") - 1},
                {.begin = "x", .len = sizeof("x") - 1},
                {.begin = "y", .len = sizeof("y") - 1},
                {.begin = "z", .len = sizeof("z") - 1},
        };

static const size_t NUMERICAL_BC_FILE_HEADER_COUNT = sizeof(NUMERICAL_BC_FILE_HEADERS) / sizeof(*NUMERICAL_BC_FILE_HEADERS);

typedef struct numerical_bc_parse_float_data_T numerical_bc_parse_float_data;
struct numerical_bc_parse_float_data_T
{
    uint32_t count;
    jta_numerical_boundary_condition_type* type;
    f32* values;
};

typedef struct numerical_bc_parse_ss_data_T numerical_bc_parse_ss_data;
struct numerical_bc_parse_ss_data_T
{
    uint32_t count;
    const jta_point_list* point_list;
    uint32_t* values;
};

static bool converter_numerical_bc_point_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    numerical_bc_parse_ss_data* const data = (numerical_bc_parse_ss_data*)param;
    uint32_t i;
    for (i = 0; i < data->point_list->count; ++i)
    {
        if (strncmp(data->point_list->label[i].begin, v->begin, v->len) == 0)
        {
            break;
        }
    }
    if (i == data->point_list->count)
    {
        JDM_ERROR("Numerical boundary condition was to be applied at point with label \"%.*s\", but there is no point with such a label", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->values[data->count++] = i;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_numerical_bc_floatx_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    numerical_bc_parse_float_data* const data = (numerical_bc_parse_float_data*)param;
    char* end;
    const f32 v_d = strtof(v->begin, &end);
    if (v->len)
    {
        if (v->begin + v->len != end)
        {
            JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
            JDM_LEAVE_FUNCTION;
            return false;
        }
        data->type[data->count] = JTA_NUMERICAL_BC_TYPE_X;
        data->values[data->count] = v_d;
    }
    else
    {
        data->type[data->count] = 0;
        data->values[data->count] = 0;
    }
    data->count += 1;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_numerical_bc_floaty_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    numerical_bc_parse_float_data* const data = (numerical_bc_parse_float_data*)param;
    char* end;
    const f32 v_d = strtof(v->begin, &end);
    if (v->len)
    {
        if (v->begin + v->len != end)
        {
            JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
            JDM_LEAVE_FUNCTION;
            return false;
        }
        data->type[data->count] |= JTA_NUMERICAL_BC_TYPE_Y;
        data->values[data->count] = v_d;
    }
    else
    {
        data->values[data->count] = 0;
    }
    data->count += 1;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_numerical_bc_floatz_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    numerical_bc_parse_float_data* const data = (numerical_bc_parse_float_data*)param;
    char* end;
    const f32 v_d = strtof(v->begin, &end);
    if (v->len)
    {
        if (v->begin + v->len != end)
        {
            JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
            JDM_LEAVE_FUNCTION;
            return false;
        }
        data->type[data->count] |= JTA_NUMERICAL_BC_TYPE_Z;
        data->values[data->count] = v_d;
    }
    else
    {
        data->values[data->count] = 0;
    }
    assert((data->type[data->count] & JTA_NUMERICAL_BC_TYPE_X) || (data->type[data->count] & JTA_NUMERICAL_BC_TYPE_Y) || (data->type[data->count] & JTA_NUMERICAL_BC_TYPE_Z));
    data->count += 1;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_numerical_bc_point_label_function,
                converter_numerical_bc_floatx_function,
                converter_numerical_bc_floaty_function,
                converter_numerical_bc_floatz_function,
        };
jta_result jta_load_numerical_boundary_conditions(
        const jio_context* io_ctx, const jio_memory_file* mem_file, const jta_point_list* point_list,
        jta_numerical_boundary_condition_list* bcs)
{
    JDM_ENTER_FUNCTION;
    jta_result res;
    uint32_t line_count = jio_memory_file_count_lines(mem_file);

    f32* x = ill_alloc(G_ALLOCATOR, sizeof(*x) * (line_count - 1));
    if (!x)
    {
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* y = ill_alloc(G_ALLOCATOR, sizeof(*y) * (line_count - 1));
    if (!y)
    {
        ill_jfree(G_ALLOCATOR, x);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* z = ill_alloc(G_ALLOCATOR, sizeof(*z) * (line_count - 1));
    if (!z)
    {
        ill_jfree(G_ALLOCATOR, x);
        ill_jfree(G_ALLOCATOR, y);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    uint32_t* i_pts = ill_alloc(G_ALLOCATOR, sizeof(*i_pts) * (line_count - 1));
    if (!i_pts)
    {
        ill_jfree(G_ALLOCATOR, x);
        ill_jfree(G_ALLOCATOR, y);
        ill_jfree(G_ALLOCATOR, z);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    jta_numerical_boundary_condition_type* type = ill_alloc(G_ALLOCATOR, sizeof(*type) * (line_count - 1));
    if (!type)
    {
        ill_jfree(G_ALLOCATOR, x);
        ill_jfree(G_ALLOCATOR, y);
        ill_jfree(G_ALLOCATOR, z);
        ill_jfree(G_ALLOCATOR, i_pts);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    numerical_bc_parse_float_data float_data[] =
            {
                    {.values = x, .type = type, .count = 0},
                    {.values = y, .type = type, .count = 0},
                    {.values = z, .type = type, .count = 0},
            };
    numerical_bc_parse_ss_data ss_data = {.values = i_pts, .point_list = point_list, .count = 0};
    void* param_array[] = { &ss_data, float_data + 0, float_data + 1, float_data + 2, };
    jio_result jio_res = jio_process_csv_exact(io_ctx, mem_file, ",", NUMERICAL_BC_FILE_HEADER_COUNT, NUMERICAL_BC_FILE_HEADERS, converter_functions, param_array);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the numerical boundary conditions input file failed, reason: %s", jio_result_to_str(jio_res));
        ill_jfree(G_ALLOCATOR, type);
        ill_jfree(G_ALLOCATOR, i_pts);
        ill_jfree(G_ALLOCATOR, x);
        ill_jfree(G_ALLOCATOR, y);
        ill_jfree(G_ALLOCATOR, z);
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
        f32* new_ptr = ill_jrealloc(G_ALLOCATOR, x, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*x) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            x = new_ptr;
        }
        new_ptr = ill_jrealloc(G_ALLOCATOR, y, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*y) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            y = new_ptr;
        }
        new_ptr = ill_jrealloc(G_ALLOCATOR, z, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*z) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            z = new_ptr;
        }
        uint32_t* const new_ptr1 = ill_jrealloc(G_ALLOCATOR, i_pts, sizeof(*new_ptr1) * count);
        if (!new_ptr1)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*i_pts) * (line_count - 1), sizeof(*new_ptr1) * count);
        }
        else
        {
            i_pts = new_ptr1;
        }
        jta_numerical_boundary_condition_type* const new_ptr2 = ill_jrealloc(G_ALLOCATOR, type, sizeof(*new_ptr2) * count);
        if (!new_ptr2)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*type) * (line_count - 1), sizeof(*new_ptr2) * count);
        }
        else
        {
            type = new_ptr2;
        }
    }
    //  Sort the numerical BCs and discover duplicates
    uint32_t* const order = lin_alloc(G_LIN_ALLOCATOR, sizeof(*order) * count);
    if (!order)
    {
        JDM_ERROR("Could not allocate buffer needed to sort numerical boundary conditions");
        ill_jfree(G_ALLOCATOR, type);
        ill_jfree(G_ALLOCATOR, i_pts);
        ill_jfree(G_ALLOCATOR, x);
        ill_jfree(G_ALLOCATOR, y);
        ill_jfree(G_ALLOCATOR, z);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_ALLOC;
    }
    void* const sup_buffer = lin_alloc(G_LIN_ALLOCATOR, sizeof(void*) * count);
    if (!sup_buffer)
    {
        JDM_ERROR("Could not allocate buffer needed to sort numerical boundary conditions");
        lin_jfree(G_LIN_ALLOCATOR, order);
        ill_jfree(G_ALLOCATOR, type);
        ill_jfree(G_ALLOCATOR, i_pts);
        ill_jfree(G_ALLOCATOR, x);
        ill_jfree(G_ALLOCATOR, y);
        ill_jfree(G_ALLOCATOR, z);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_ALLOC;
    }

    uint32_t i_bc = 0;
    for (uint32_t i_pt = 0; i_pt < point_list->count && i_bc < count; ++i_pt)
    {
        uint32_t pos = UINT32_MAX;
        for (uint32_t j = 0; j < count; ++j)
        {
            if (i_pt == i_pts[j])
            {
                if (pos != UINT32_MAX)
                {
                    JDM_ERROR("Multiple numerical boundary conditions are applied on point \"%.*s\"", (int)point_list->label[i_pt].len, point_list->label[i_pt].begin);
                    ill_jfree(G_ALLOCATOR, type);
                    ill_jfree(G_ALLOCATOR, i_pts);
                    ill_jfree(G_ALLOCATOR, x);
                    ill_jfree(G_ALLOCATOR, y);
                    ill_jfree(G_ALLOCATOR, z);
                    JDM_LEAVE_FUNCTION;
                    return JTA_RESULT_BAD_NUM_BC;
                }
                pos = j;
            }
        }
        if (pos == UINT32_MAX)
        {
            continue;
        }
        order[i_bc] = pos;
        i_bc += 1;
    }
    assert(i_bc == count);

    f32* const sup_x = sup_buffer;
    for (uint32_t i = 0; i < count; ++i)
    {
        sup_x[i] = x[order[i]];
    }
    memcpy(x, sup_x, sizeof(*x) * count);

    f32* const sup_y = sup_buffer;
    for (uint32_t i = 0; i < count; ++i)
    {
        sup_y[i] = y[order[i]];
    }
    memcpy(y, sup_y, sizeof(*y) * count);

    f32* const sup_z = sup_buffer;
    for (uint32_t i = 0; i < count; ++i)
    {
        sup_z[i] = z[order[i]];
    }
    memcpy(z, sup_z, sizeof(*z) * count);

    uint32_t* const sup_i = sup_buffer;
    for (uint32_t i = 0; i < count; ++i)
    {
        sup_i[i] = i_pts[order[i]];
    }
    memcpy(i_pts, sup_i, sizeof(*i_pts) * count);

    jta_numerical_boundary_condition_type* const sup_t = sup_buffer;
    for (uint32_t i = 0; i < count; ++i)
    {
        sup_t[i] = type[order[i]];
    }
    memcpy(type, sup_t, sizeof(*type) * count);

    lin_jfree(G_LIN_ALLOCATOR, sup_buffer);
    lin_jfree(G_LIN_ALLOCATOR, order);

    *bcs = (jta_numerical_boundary_condition_list){.count = count, .x = x, .y = y, .z = z, .i_point = i_pts, .type = type};

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_free_numerical_boundary_conditions(jta_numerical_boundary_condition_list* bcs)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_ALLOCATOR, bcs->i_point);
    ill_jfree(G_ALLOCATOR, bcs->type);
    ill_jfree(G_ALLOCATOR, bcs->x);
    ill_jfree(G_ALLOCATOR, bcs->y);
    ill_jfree(G_ALLOCATOR, bcs->z);

    JDM_LEAVE_FUNCTION;
}
