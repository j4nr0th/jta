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

typedef struct point_parse_data_struct point_parse_data;
struct point_parse_data_struct
{
    uint32_t count;
    jtb_point* points;
};

static bool converter_point_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    point_parse_data* const data = (point_parse_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (string_segment_equal(&data->points[i].label, v))
        {
            JDM_ERROR("Point label \"%.*s\" was already defined as point %u", (int)v->len, v->begin, i);
            JDM_LEAVE_FUNCTION;
            return false;
        }
    }
    data->points[data->count++].label = *v;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_x_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    point_parse_data* const data = (point_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->points[data->count++].x = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_y_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    point_parse_data* const data = (point_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->points[data->count++].y = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_z_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    point_parse_data* const data = (point_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->points[data->count++].z = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_point_label_function,
                converter_x_function,
                converter_y_function,
                converter_z_function,
        };

jtb_result jtb_load_points(const jio_memory_file* mem_file, u32* n_pts, jtb_point** pp_points)
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
    jtb_point* point_array = jalloc(G_JALLOCATOR, sizeof(*point_array) * (line_count - 1));
    if (!point_array)
    {
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    point_parse_data parse_data[] =
            {
                    {.points = point_array, .count = 0},
                    {.points = point_array, .count = 0},
                    {.points = point_array, .count = 0},
                    {.points = point_array, .count = 0},
            };
    void* param_array[] = {parse_data + 0, parse_data + 1, parse_data + 2, parse_data + 3};
    jio_res = jio_process_csv_exact(mem_file, ",", POINT_FILE_HEADER_COUNT, POINT_FILE_HEADERS, converter_functions, param_array, G_LIN_JALLOCATOR);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the point input file failed, reason: %s", jio_result_to_str(jio_res));
        jfree(G_JALLOCATOR, point_array);
        res = JTB_RESULT_BAD_INPUT;
        goto end;
    }
    assert(parse_data[0].count == parse_data[1].count);
    assert(parse_data[2].count == parse_data[3].count);
    assert(parse_data[1].count == parse_data[2].count);


    const uint32_t count = parse_data[0].count;
    assert(count <= line_count - 1);
    if (count != line_count - 1)
    {
        jtb_point* const new_ptr = jrealloc(G_JALLOCATOR, point_array, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the point array from %zu to %zu bytes", sizeof(*point_array) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            point_array = new_ptr;
        }
    }
    *n_pts = count;
    *pp_points = point_array;

    JDM_LEAVE_FUNCTION;
    return JTB_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}
