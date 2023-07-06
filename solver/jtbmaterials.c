//
// Created by jan on 5.7.2023.
//

#include "jtbmaterials.h"

static const jio_string_segment MATERIAL_FILE_HEADERS[] =
        {
                {.begin = "material label", .len = sizeof("material label") - 1},
                {.begin = "elastic modulus", .len = sizeof("elastic modulus") - 1},
                {.begin = "density", .len = sizeof("density") - 1},
                {.begin = "tensile strength", .len = sizeof("tensile strength") - 1},
                {.begin = "compressive strength", .len = sizeof("compressive strength") - 1},
        };

static const size_t MATERIAL_FILE_HEADER_COUNT = sizeof(MATERIAL_FILE_HEADERS) / sizeof(*MATERIAL_FILE_HEADERS);

typedef struct material_parse_data_struct material_parse_data;
struct material_parse_data_struct
{
    uint32_t count;
    jtb_material* materials;
};

static bool converter_material_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    material_parse_data* const data = (material_parse_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (string_segment_equal(&data->materials[i].label, v))
        {
            JDM_ERROR("Material label \"%.*s\" was already defined as material %u", (int)v->len, v->begin, i);
            JDM_LEAVE_FUNCTION;
            return false;
        }
    }
    data->materials[data->count++].label = *v;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_elastic_modulus_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    material_parse_data* const data = (material_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->materials[data->count++].elastic_modulus = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_density_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    material_parse_data* const data = (material_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->materials[data->count++].density = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_tensile_strength_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    material_parse_data* const data = (material_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->materials[data->count++].tensile_strength = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_compressive_strength_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    material_parse_data* const data = (material_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->materials[data->count++].compressive_strength = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_material_label_function,
                converter_elastic_modulus_function,
                converter_density_function,
                converter_tensile_strength_function,
                converter_compressive_strength_function,
        };

jtb_result jtb_load_materials(const jio_memory_file* mem_file, u32* n_mat, jtb_material** pp_materials)
{
    JDM_ENTER_FUNCTION;
    jtb_result res;
    uint32_t line_count = 0;
    jio_result jio_res = jio_memory_file_count_lines(mem_file, &line_count);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not count lines in material input file, reason: %s", jio_result_to_str(jio_res));
        res = JTB_RESULT_BAD_IO;
        goto end;
    }
    jtb_material* material_array = jalloc(G_JALLOCATOR, sizeof(*material_array) * (line_count - 1));
    if (!material_array)
    {
        JDM_ERROR("Could not allocate memory for material array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    material_parse_data parse_data[] =
            {
                    {.materials = material_array, .count = 0},
                    {.materials = material_array, .count = 0},
                    {.materials = material_array, .count = 0},
                    {.materials = material_array, .count = 0},
                    {.materials = material_array, .count = 0},
            };
    void* param_array[] = {parse_data + 0, parse_data + 1, parse_data + 2, parse_data + 3, parse_data + 4};
    jio_res = jio_process_csv_exact(mem_file, ",", MATERIAL_FILE_HEADER_COUNT, MATERIAL_FILE_HEADERS, converter_functions, param_array, G_LIN_JALLOCATOR);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the material input file failed, reason: %s", jio_result_to_str(jio_res));
        jfree(G_JALLOCATOR, material_array);
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
        jtb_material* const new_ptr = jrealloc(G_JALLOCATOR, material_array, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the material array from %zu to %zu bytes", sizeof(*material_array) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            material_array = new_ptr;
        }
    }
    *n_mat = count;
    *pp_materials = material_array;

    JDM_LEAVE_FUNCTION;
    return JTB_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}
