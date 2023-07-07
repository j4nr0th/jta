//
// Created by jan on 5.7.2023.
//

#include "jtbelements.h"

static const jio_string_segment ELEMENT_FILE_HEADERS[] =
        {
                {.begin = "element label", .len = sizeof("element label") - 1},
                {.begin = "material label", .len = sizeof("material label") - 1},
                {.begin = "profile label", .len = sizeof("profile label") - 1},
                {.begin = "point label 1", .len = sizeof("point label 1") - 1},
                {.begin = "point label 2", .len = sizeof("point label 2") - 1},
        };

static const size_t ELEMENT_FILE_HEADER_COUNT = sizeof(ELEMENT_FILE_HEADERS) / sizeof(*ELEMENT_FILE_HEADERS);

typedef struct element_parse_data_struct element_parse_data;
struct element_parse_data_struct
{
    uint32_t count;
    jtb_element* elements;
    const jtb_point_list* p_pts;
    const u32 n_mat;
    const jtb_material* p_mat;
    const jtb_profile_list* p_pro;
};

static bool converter_element_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_parse_data* const data = (element_parse_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (string_segment_equal(&data->elements[i].label, v))
        {
            JDM_ERROR("Element label \"%.*s\" was already defined as element %u", (int)v->len, v->begin, i);
            JDM_LEAVE_FUNCTION;
            return false;
        }
    }
    data->elements[data->count++].label = *v;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_material_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_parse_data* const data = (element_parse_data*)param;
    for (uint32_t i = 0; i < data->n_mat; ++i)
    {
        if (string_segment_equal(&data->p_mat[i].label, v))
        {
            data->elements[data->count++].i_material = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified material \"%.*s\", which was not defined", (int)(data->elements[data->count - 1].label.len), data->elements[data->count].label.begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool converter_profile_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_parse_data* const data = (element_parse_data*)param;
    for (uint32_t i = 0; i < data->p_pro->count; ++i)
    {
        if (string_segment_equal(data->p_pro->labels + i, v))
        {
            data->elements[data->count++].i_profile = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified profile \"%.*s\", which was not defined", (int)(data->elements[data->count - 1].label.len), data->elements[data->count].label.begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool converter_point0_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_parse_data* const data = (element_parse_data*)param;
    for (uint32_t i = 0; i < data->p_pts->count; ++i)
    {
        if (string_segment_equal(data->p_pts->label + i, v))
        {
            data->elements[data->count++].i_point0 = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified point 0 \"%.*s\", which was not defined", (int)(data->elements[data->count - 1].label.len), data->elements[data->count].label.begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool converter_point1_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_parse_data* const data = (element_parse_data*)param;
    for (uint32_t i = 0; i < data->p_pts->count; ++i)
    {
        if (string_segment_equal(data->p_pts->label + i, v))
        {
            data->elements[data->count++].i_point1 = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified point 1 \"%.*s\", which was not defined", (int)(data->elements[data->count - 1].label.len), data->elements[data->count].label.begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_element_label_function,
                converter_material_label_function,
                converter_profile_label_function,
                converter_point0_label_function,
                converter_point1_label_function,
        };

jtb_result jtb_load_elements(
        const jio_memory_file* mem_file, const jtb_point_list* points, u32 n_mat, const jtb_material* materials,
        const jtb_profile_list* profiles, u32* n_elm, jtb_element** pp_elements)
{
    JDM_ENTER_FUNCTION;
    jtb_result res;
    uint32_t line_count = 0;
    jio_result jio_res = jio_memory_file_count_lines(mem_file, &line_count);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not count lines in element input file, reason: %s", jio_result_to_str(jio_res));
        res = JTB_RESULT_BAD_IO;
        goto end;
    }
    jtb_element* element_array = jalloc(G_JALLOCATOR, sizeof(*element_array) * (line_count - 1));
    if (!element_array)
    {
        JDM_ERROR("Could not allocate memory for element array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    element_parse_data parse_data[] =
            {
                    {.elements = element_array, .count = 0, .n_mat = n_mat, .p_pts = points, .p_mat = materials, .p_pro = profiles},
                    {.elements = element_array, .count = 0, .n_mat = n_mat, .p_pts = points, .p_mat = materials, .p_pro = profiles},
                    {.elements = element_array, .count = 0, .n_mat = n_mat, .p_pts = points, .p_mat = materials, .p_pro = profiles},
                    {.elements = element_array, .count = 0, .n_mat = n_mat, .p_pts = points, .p_mat = materials, .p_pro = profiles},
                    {.elements = element_array, .count = 0, .n_mat = n_mat, .p_pts = points, .p_mat = materials, .p_pro = profiles},
            };
    void* param_array[] = {parse_data + 0, parse_data + 1, parse_data + 2, parse_data + 3, parse_data + 4};
    jio_res = jio_process_csv_exact(mem_file, ",", ELEMENT_FILE_HEADER_COUNT, ELEMENT_FILE_HEADERS, converter_functions, param_array, G_LIN_JALLOCATOR);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the element input file failed, reason: %s", jio_result_to_str(jio_res));
        jfree(G_JALLOCATOR, element_array);
        res = JTB_RESULT_BAD_INPUT;
        goto end;
    }
    assert(parse_data[0].count == parse_data[1].count);
    assert(parse_data[2].count == parse_data[3].count);
    assert(parse_data[1].count == parse_data[4].count);
    assert(parse_data[1].count == parse_data[2].count);


    const uint32_t count = parse_data[0].count;
    assert(count <= line_count - 1);
    if (count != line_count - 1)
    {
        jtb_element* const new_ptr = jrealloc(G_JALLOCATOR, element_array, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the element array from %zu to %zu bytes", sizeof(*element_array) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            element_array = new_ptr;
        }
    }

    //  Determine maximum radius of elements connected to each point
    for (u32 i = 0; i < count; ++i)
    {
        const f32 eq_radius = profiles->equivalent_radius[element_array[i].i_profile];
        const f32 current_radius0 = points->max_radius[element_array[i].i_point0];
        if (eq_radius > current_radius0)
        {
            points->max_radius[element_array[i].i_point0] = eq_radius;
        }
        const f32 current_radius1 = points->max_radius[element_array[i].i_point1];
        if (eq_radius > current_radius1)
        {
            points->max_radius[element_array[i].i_point1] = eq_radius;
        }
    }

    *pp_elements = element_array;
    *n_elm = count;
    JDM_LEAVE_FUNCTION;
    return JTB_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}
