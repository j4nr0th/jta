//
// Created by jan on 5.7.2023.
//

#include "jtbpoints.h"

static const jio_string_segment POINT_FILE_HEADERS[] =
        {
                {.begin = "point label", .len = sizeof("point label") - 1},
                {.begin = "x", .len = sizeof("x") - 1},
                {.begin = "y", .len = sizeof("y") - 1},
                {.begin = "z", .len = sizeof("z") - 1},
        };

static const size_t POINT_FILE_HEADER_COUNT = sizeof(POINT_FILE_HEADERS) / sizeof(*POINT_FILE_HEADERS);

typedef struct point_parse_data_float_struct point_parse_data_float;
struct point_parse_data_float_struct
{
    uint32_t count;
    f32* values;
};
typedef struct point_parse_data_ss_struct point_parse_data_ss;
struct point_parse_data_ss_struct
{
    uint32_t count;
    jio_string_segment * values;
};

static bool converter_point_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    point_parse_data_ss* const data = (point_parse_data_ss*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (string_segment_equal(data->values + i , v))
        {
            JDM_ERROR("Point label \"%.*s\" was already defined as point %u", (int)v->len, v->begin, i);
            JDM_LEAVE_FUNCTION;
            return false;
        }
    }
    data->values[data->count++] = *v;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_float_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    point_parse_data_float* const data = (point_parse_data_float*)param;
    char* end;
    const f32 v_d = strtof(v->begin, &end);
    if (v->begin + v->len != end)
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
                converter_point_label_function,
                converter_float_function,
                converter_float_function,
                converter_float_function,
        };

jtb_result jtb_load_points(const jio_memory_file* mem_file, jtb_point_list* p_list)
{
    JDM_ENTER_FUNCTION;
    jtb_result res;
    uint32_t line_count = 0;
    jio_result jio_res = jio_memory_file_count_lines(mem_file, &line_count);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not count lines in point input file, reason: %s", jio_result_to_str(jio_res));
        res = JTB_RESULT_BAD_IO;
        goto end;
    }

    f32* x = jalloc(G_JALLOCATOR, sizeof(*x) * (line_count - 1));
    if (!x)
    {
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* y = jalloc(G_JALLOCATOR, sizeof(*y) * (line_count - 1));
    if (!y)
    {
        jfree(G_JALLOCATOR, x);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* z = jalloc(G_JALLOCATOR, sizeof(*z) * (line_count - 1));
    if (!z)
    {
        jfree(G_JALLOCATOR, x);
        jfree(G_JALLOCATOR, y);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    jio_string_segment* ss = jalloc(G_JALLOCATOR, sizeof(*ss) * (line_count - 1));
    if (!ss)
    {
        jfree(G_JALLOCATOR, x);
        jfree(G_JALLOCATOR, y);
        jfree(G_JALLOCATOR, z);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    point_parse_data_float float_data[] =
            {
                    {.values = x, .count = 0},
                    {.values = y, .count = 0},
                    {.values = z, .count = 0},
            };
    point_parse_data_ss ss_data = {.values = ss, .count = 0};
    void* param_array[] = { &ss_data, float_data + 0, float_data + 1, float_data + 2, };
    jio_res = jio_process_csv_exact(mem_file, ",", POINT_FILE_HEADER_COUNT, POINT_FILE_HEADERS, converter_functions, param_array, G_LIN_JALLOCATOR);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the point input file failed, reason: %s", jio_result_to_str(jio_res));
        jfree(G_JALLOCATOR, ss);
        jfree(G_JALLOCATOR, x);
        jfree(G_JALLOCATOR, y);
        jfree(G_JALLOCATOR, z);
        res = JTB_RESULT_BAD_INPUT;
        goto end;
    }
    assert(float_data[0].count == float_data[1].count);
    assert(float_data[1].count == float_data[2].count);
    assert(float_data[1].count == ss_data.count);


    const uint32_t count = float_data[0].count;
    assert(count <= line_count - 1);
    f32* const max_radius = jalloc(G_JALLOCATOR, sizeof(*max_radius) * count);
    if (!max_radius)
    {
        jfree(G_JALLOCATOR, x);
        jfree(G_JALLOCATOR, y);
        jfree(G_JALLOCATOR, z);
        jfree(G_JALLOCATOR, ss);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    if (count != line_count - 1)
    {
        f32* new_ptr = jrealloc(G_JALLOCATOR, x, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the point array from %zu to %zu bytes", sizeof(*x) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            x = new_ptr;
        }
        new_ptr = jrealloc(G_JALLOCATOR, y, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the point array from %zu to %zu bytes", sizeof(*y) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            y = new_ptr;
        }
        new_ptr = jrealloc(G_JALLOCATOR, z, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the point array from %zu to %zu bytes", sizeof(*z) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            z = new_ptr;
        }
        jio_string_segment* const new_ptr1 = jrealloc(G_JALLOCATOR, ss, sizeof(*new_ptr1) * count);
        if (!new_ptr1)
        {
            JDM_WARN("Failed shrinking the point array from %zu to %zu bytes", sizeof(*ss) * (line_count - 1), sizeof(*new_ptr1) * count);
        }
        else
        {
            ss = new_ptr1;
        }
    }
    memset(max_radius, 0, count * sizeof(*max_radius));
    *p_list = (jtb_point_list){.count = count, .label = ss, .p_x = x, .p_y = y, .p_z = z, .max_radius = max_radius};

    JDM_LEAVE_FUNCTION;
    return JTB_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}
