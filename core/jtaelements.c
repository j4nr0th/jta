//
// Created by jan on 5.7.2023.
//

#include "jtaelements.h"
#include <jdm.h>

static const jio_string_segment ELEMENT_FILE_HEADERS[] =
        {
                {.begin = "element label", .len = sizeof("element label") - 1},
                {.begin = "material label", .len = sizeof("material label") - 1},
                {.begin = "profile label", .len = sizeof("profile label") - 1},
                {.begin = "point label 1", .len = sizeof("point label 1") - 1},
                {.begin = "point label 2", .len = sizeof("point label 2") - 1},
        };

static const size_t ELEMENT_FILE_HEADER_COUNT = sizeof(ELEMENT_FILE_HEADERS) / sizeof(*ELEMENT_FILE_HEADERS);

typedef struct element_parse_ss_data_T element_ss_parse_data;
struct element_parse_ss_data_T
{
    uint32_t count;
    jio_string_segment* values;
};

typedef struct element_parse_pt_data_T element_pt_parse_data;
struct element_parse_pt_data_T
{
    uint32_t count;
    jio_string_segment* labels;
    const jta_point_list* points;
    uint32_t* values;
};

typedef struct element_parse_pro_data_T element_pro_parse_data;
struct element_parse_pro_data_T
{
    uint32_t count;
    jio_string_segment* labels;
    const jta_profile_list* profiles;
    uint32_t* values;
};

typedef struct element_parse_mat_data_T element_parse_mat_data;
struct element_parse_mat_data_T
{
    uint32_t count;
    jio_string_segment* labels;
    const jta_material_list* materials;
    uint32_t* values;
};



static bool converter_element_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_ss_parse_data* const data = (element_ss_parse_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (strncmp(data->values[i].begin , v->begin, v->len) == 0)
        {
            JDM_ERROR("Element label \"%.*s\" was already defined as element %u", (int)v->len, v->begin, i);
            JDM_LEAVE_FUNCTION;
            return false;
        }
    }
    data->values[data->count++] = *v;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_material_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_parse_mat_data* const data = (element_parse_mat_data*)param;
    for (uint32_t i = 0; i < data->materials->count; ++i)
    {
        if (strncmp(data->materials->labels[i].begin, v->begin, v->len) == 0)
        {
            data->values[data->count++] = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified material \"%.*s\", which was not defined", (int)(data->labels[data->count - 1].len), data->labels[data->count].begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool converter_profile_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_pro_parse_data* const data = (element_pro_parse_data*)param;
    for (uint32_t i = 0; i < data->profiles->count; ++i)
    {
        if (strncmp(data->profiles->labels[i].begin, v->begin, v->len) == 0)
        {
            data->values[data->count++] = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified profile \"%.*s\", which was not defined", (int)(data->labels[data->count - 1].len), data->labels[data->count - 1].begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool converter_point_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    element_pt_parse_data* const data = (element_pt_parse_data*)param;
    for (uint32_t i = 0; i < data->points->count; ++i)
    {
        if (strncmp(data->points->label[i].begin, v->begin, v->len) == 0)
        {
            data->values[data->count++] = i;
            JDM_LEAVE_FUNCTION;
            return true;
        }
    }
    JDM_ERROR("Element \"%.*s\" specified point 0 \"%.*s\", which was not defined", (int)(data->labels[data->count - 1].len), data->labels[data->count - 1].begin, (int)v->len, v->begin);
    JDM_LEAVE_FUNCTION;
    return false;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_element_label_function,
                converter_material_label_function,
                converter_profile_label_function,
                converter_point_label_function,
                converter_point_label_function,
        };

jta_result jta_load_elements(
        const jio_context* io_ctx, const jio_memory_file* mem_file, const jta_point_list* points,
        const jta_material_list* materials, const jta_profile_list* profiles, jta_element_list* element_list)
{
    JDM_ENTER_FUNCTION;
    jta_result res;
    uint32_t line_count = jio_memory_file_count_lines(mem_file);
    uint32_t* i_point0 = ill_alloc(G_ALLOCATOR, sizeof(*i_point0) * (line_count - 1));
    if (!i_point0)
    {
        JDM_ERROR("Could not allocate memory for element array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    uint32_t* i_point1 = ill_alloc(G_ALLOCATOR, sizeof(*i_point1) * (line_count - 1));
    if (!i_point1)
    {
        JDM_ERROR("Could not allocate memory for element array");
        ill_jfree(G_ALLOCATOR, i_point0);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }

    uint32_t* i_material = ill_alloc(G_ALLOCATOR, sizeof(*i_material) * (line_count - 1));
    if (!i_material)
    {
        JDM_ERROR("Could not allocate memory for element array");
        ill_jfree(G_ALLOCATOR, i_point0);
        ill_jfree(G_ALLOCATOR, i_point1);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    uint32_t* i_profile = ill_alloc(G_ALLOCATOR, sizeof(*i_profile) * (line_count - 1));
    if (!i_profile)
    {
        JDM_ERROR("Could not allocate memory for element array");
        ill_jfree(G_ALLOCATOR, i_point0);
        ill_jfree(G_ALLOCATOR, i_point1);
        ill_jfree(G_ALLOCATOR, i_material);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    jio_string_segment* labels = ill_alloc(G_ALLOCATOR, sizeof(*labels) * (line_count - 1));
    if (!labels)
    {
        JDM_ERROR("Could not allocate memory for element array");
        ill_jfree(G_ALLOCATOR, i_point0);
        ill_jfree(G_ALLOCATOR, i_point1);
        ill_jfree(G_ALLOCATOR, i_material);
        ill_jfree(G_ALLOCATOR, i_profile);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* lengths = ill_alloc(G_ALLOCATOR, sizeof(*lengths) * (line_count - 1));
    if (!lengths)
    {
        JDM_ERROR("Could not allocate memory for element array");
        ill_jfree(G_ALLOCATOR, i_point0);
        ill_jfree(G_ALLOCATOR, i_point1);
        ill_jfree(G_ALLOCATOR, i_material);
        ill_jfree(G_ALLOCATOR, i_profile);
        ill_jfree(G_ALLOCATOR, labels);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }

    element_ss_parse_data label_parse_data = {.count = 0, .values = labels};
    element_parse_mat_data mat_parse_data = {.count = 0, .labels = labels, .materials = materials, .values = i_material};
    element_pro_parse_data pro_parse_data = {.count = 0, .labels = labels, .profiles = profiles, .values = i_profile};
    element_pt_parse_data  pts_parse_data0 = {.count = 0, .labels = labels, .points = points, .values = i_point0};
    element_pt_parse_data  pts_parse_data1 = {.count = 0, .labels = labels, .points = points, .values = i_point1};
    void* param_array[] = {&label_parse_data, &mat_parse_data, &pro_parse_data, &pts_parse_data0, &pts_parse_data1};

    jio_result jio_res = jio_process_csv_exact(io_ctx, mem_file, ",", ELEMENT_FILE_HEADER_COUNT, ELEMENT_FILE_HEADERS, converter_functions, param_array);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the element input file failed, reason: %s", jio_result_to_str(jio_res));
        ill_jfree(G_ALLOCATOR, i_point1);
        res = JTA_RESULT_BAD_INPUT;
        goto end;
    }
    assert(label_parse_data.count == mat_parse_data.count);
    assert(pro_parse_data.count == pts_parse_data0.count);
    assert(pts_parse_data1.count == mat_parse_data.count);
    assert(pts_parse_data1.count == pts_parse_data0.count);


    const uint32_t count = label_parse_data.count;
    assert(count <= line_count - 1);
    if (count != line_count - 1)
    {
        //  Point 0 array
        {
            uint32_t* const new_ptr = ill_jrealloc(G_ALLOCATOR, i_point0, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the element array from %zu to %zu bytes",
                         sizeof(*new_ptr) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                i_point0 = new_ptr;
            }
        }
        //  Point 1 array
        {
            uint32_t* const new_ptr = ill_jrealloc(G_ALLOCATOR, i_point1, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the element array from %zu to %zu bytes",
                         sizeof(*new_ptr) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                i_point1 = new_ptr;
            }
        }
        //  Profile array
        {
            uint32_t* const new_ptr = ill_jrealloc(G_ALLOCATOR, i_profile, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the element array from %zu to %zu bytes",
                         sizeof(*new_ptr) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                i_profile = new_ptr;
            }
        }
        //  Material array
        {
            uint32_t* const new_ptr = ill_jrealloc(G_ALLOCATOR, i_material, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the element array from %zu to %zu bytes",
                         sizeof(*new_ptr) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                i_material = new_ptr;
            }
        }
        //  Label array
        {
            jio_string_segment * const new_ptr = ill_jrealloc(G_ALLOCATOR, labels, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the element array from %zu to %zu bytes",
                         sizeof(*new_ptr) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                labels = new_ptr;
            }
        }
        //  Length array
        {
            f32 * const new_ptr = ill_jrealloc(G_ALLOCATOR, lengths, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the element array from %zu to %zu bytes",
                         sizeof(*new_ptr) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                lengths = new_ptr;
            }
        }
    }

    //  Determine the maximum/minimum lengths of elements
    f32 l_min = +INFINITY, l_max = -INFINITY;
    u32 i = 0;

    for (i <<= 2; i < count; ++i)
    {
        f32 v = hypotf(points->p_x[i_point0[i]] - points->p_x[i_point1[i]], hypotf(points->p_y[i_point0[i]] - points->p_y[i_point1[i]], points->p_z[i_point0[i]] - points->p_z[i_point1[i]]));
        lengths[i] = v;
    }
    for (i = 0; i < count; ++i)
    {
        const f32 v = lengths[i];
        if (v < l_min)
        {
            l_min = v;
        }
        if (v > l_max)
        {
            l_max = v;
        }
    }

    //  Determine maximum radius of elements connected to each point
    for (i = 0; i < count; ++i)
    {
        const f32 eq_radius = profiles->equivalent_radius[i_profile[i]];
        const f32 current_radius0 = points->max_radius[i_point0[i]];
        if (eq_radius > current_radius0)
        {
            points->max_radius[i_point0[i]] = eq_radius;
        }
        const f32 current_radius1 = points->max_radius[i_point1[i]];
        if (eq_radius > current_radius1)
        {
            points->max_radius[i_point1[i]] = eq_radius;
        }
    }

    *element_list = (jta_element_list){.count = count, .labels = labels, .i_profile = i_profile, .i_point0 = i_point0, .i_point1 = i_point1, .i_material = i_material, .lengths = lengths, .max_len = l_max, .min_len = l_min};
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_free_elements(jta_element_list* element_list)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_ALLOCATOR, element_list->labels);
    ill_jfree(G_ALLOCATOR, element_list->i_point0);
    ill_jfree(G_ALLOCATOR, element_list->i_point1);
    ill_jfree(G_ALLOCATOR, element_list->i_profile);
    ill_jfree(G_ALLOCATOR, element_list->i_material);
    ill_jfree(G_ALLOCATOR, element_list->lengths);

    JDM_LEAVE_FUNCTION;
}
