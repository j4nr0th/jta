//
// Created by jan on 10.7.2023.
//

#include "jtanumericalbcs.h"

static const jio_string_segment NUMERICAL_BC_FILE_HEADERS[] =
        {
                {.begin = "point label", .len = sizeof("point label") - 1},
                {.begin = "x", .len = sizeof("x") - 1},
                {.begin = "y", .len = sizeof("y") - 1},
                {.begin = "z", .len = sizeof("z") - 1},
        };

static const size_t NUMERICAL_BC_FILE_HEADER_COUNT = sizeof(NUMERICAL_BC_FILE_HEADERS) / sizeof(*NUMERICAL_BC_FILE_HEADERS);

typedef struct numerical_bc_parse_float_data_struct numerical_bc_parse_float_data;
struct numerical_bc_parse_float_data_struct
{
    uint32_t count;
    jta_numerical_boundary_condition_type* type;
    f32* values;
};

typedef struct numerical_bc_parse_ss_data_struct numerical_bc_parse_ss_data;
struct numerical_bc_parse_ss_data_struct
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
        if (string_segment_equal(data->point_list->label + i, v))
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
        data->type[data->count] |= JTA_NUMERICAL_BC_TYPE_X;
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
        const jio_memory_file* mem_file, const jta_point_list* point_list, jta_numerical_boundary_condition_list* bcs)
{

    JDM_ENTER_FUNCTION;
    jta_result res;
    uint32_t line_count = 0;
    jio_result jio_res = jio_memory_file_count_lines(mem_file, &line_count);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not count lines in numerical boundary condition input file, reason: %s", jio_result_to_str(jio_res));
        res = JTA_RESULT_BAD_IO;
        goto end;
    }

    f32* x = ill_jalloc(G_JALLOCATOR, sizeof(*x) * (line_count - 1));
    if (!x)
    {
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* y = ill_jalloc(G_JALLOCATOR, sizeof(*y) * (line_count - 1));
    if (!y)
    {
        ill_jfree(G_JALLOCATOR, x);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* z = ill_jalloc(G_JALLOCATOR, sizeof(*z) * (line_count - 1));
    if (!z)
    {
        ill_jfree(G_JALLOCATOR, x);
        ill_jfree(G_JALLOCATOR, y);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    uint32_t* i_pts = ill_jalloc(G_JALLOCATOR, sizeof(*i_pts) * (line_count - 1));
    if (!i_pts)
    {
        ill_jfree(G_JALLOCATOR, x);
        ill_jfree(G_JALLOCATOR, y);
        ill_jfree(G_JALLOCATOR, z);
        JDM_ERROR("Could not allocate memory for numerical boundary condition array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    jta_numerical_boundary_condition_type* type = ill_jalloc(G_JALLOCATOR, sizeof(*type) * (line_count - 1));
    if (!type)
    {
        ill_jfree(G_JALLOCATOR, x);
        ill_jfree(G_JALLOCATOR, y);
        ill_jfree(G_JALLOCATOR, z);
        ill_jfree(G_JALLOCATOR, i_pts);
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
    jio_stack_allocator_callbacks jio_callbacks =
            {
                    .param = G_LIN_JALLOCATOR,
                    .alloc = (void* (*)(void*, uint64_t)) lin_jalloc,
                    .free = (void (*)(void*, void*)) lin_jfree,
                    .realloc = (void* (*)(void*, void*, uint64_t)) lin_jrealloc,
                    .restore = (void (*)(void*, void*)) lin_jallocator_restore_current,
                    .save = (void* (*)(void*)) lin_jallocator_save_state,
            };
    jio_res = jio_process_csv_exact(mem_file, ",", NUMERICAL_BC_FILE_HEADER_COUNT, NUMERICAL_BC_FILE_HEADERS, converter_functions, param_array, &jio_callbacks);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the numerical boundary conditions input file failed, reason: %s", jio_result_to_str(jio_res));
        ill_jfree(G_JALLOCATOR, type);
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
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*x) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            x = new_ptr;
        }
        new_ptr = ill_jrealloc(G_JALLOCATOR, y, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*y) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            y = new_ptr;
        }
        new_ptr = ill_jrealloc(G_JALLOCATOR, z, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*z) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            z = new_ptr;
        }
        uint32_t* const new_ptr1 = ill_jrealloc(G_JALLOCATOR, i_pts, sizeof(*new_ptr1) * count);
        if (!new_ptr1)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*i_pts) * (line_count - 1), sizeof(*new_ptr1) * count);
        }
        else
        {
            i_pts = new_ptr1;
        }
        jta_numerical_boundary_condition_type* const new_ptr2 = ill_jrealloc(G_JALLOCATOR, type, sizeof(*new_ptr2) * count);
        if (!new_ptr2)
        {
            JDM_WARN("Failed shrinking the numerical boundary condition array from %zu to %zu bytes", sizeof(*type) * (line_count - 1), sizeof(*new_ptr2) * count);
        }
        else
        {
            type = new_ptr2;
        }
    }
    *bcs = (jta_numerical_boundary_condition_list){.count = count, .x = x, .y = y, .z = z, .i_point = i_pts, .type = type};

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
    end:
    JDM_LEAVE_FUNCTION;
    return res;
}
