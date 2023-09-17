//
// Created by jan on 5.7.2023.
//

#include "jtamaterials.h"
#include <jdm.h>

static const jio_string_segment MATERIAL_FILE_HEADERS[] =
        {
                {.begin = "material label", .len = sizeof("material label") - 1},
                {.begin = "elastic modulus", .len = sizeof("elastic modulus") - 1},
                {.begin = "density", .len = sizeof("density") - 1},
                {.begin = "tensile strength", .len = sizeof("tensile strength") - 1},
                {.begin = "compressive strength", .len = sizeof("compressive strength") - 1},
        };

static const size_t MATERIAL_FILE_HEADER_COUNT = sizeof(MATERIAL_FILE_HEADERS) / sizeof(*MATERIAL_FILE_HEADERS);

typedef struct material_parse_float_data_T material_parse_float_data;
struct material_parse_float_data_T
{
    uint32_t count;
    f32* values;
};

typedef struct material_parse_ss_data_T material_parse_ss_data;
struct material_parse_ss_data_T
{
    uint32_t count;
    jio_string_segment* values;
};

static bool converter_material_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    material_parse_ss_data* const data = (material_parse_ss_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (strncmp(data->values[i].begin, v->begin, v->len) == 0)
        {
            JDM_ERROR("Material label \"%.*s\" was already defined as material %u", (int)v->len, v->begin, i);
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
    material_parse_float_data* const data = (material_parse_float_data*)param;
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
                converter_material_label_function,
                converter_float_function,
                converter_float_function,
                converter_float_function,
                converter_float_function,
        };

jta_result
jta_load_materials(const jio_context* io_ctx, const jio_memory_file* mem_file, jta_material_list* material_list)
{
    JDM_ENTER_FUNCTION;
    jta_result res;
    uint32_t line_count = jio_memory_file_count_lines(mem_file);

    f32* density = ill_jalloc(G_JALLOCATOR, sizeof(*density) * (line_count - 1));
    if (!density)
    {
        JDM_ERROR("Could not allocate memory for material array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* elastic_modulus = ill_jalloc(G_JALLOCATOR, sizeof(*elastic_modulus) * (line_count - 1));
    if (!elastic_modulus)
    {
        JDM_ERROR("Could not allocate memory for material array");
        ill_jfree(G_JALLOCATOR, density);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* tensile_strength = ill_jalloc(G_JALLOCATOR, sizeof(*tensile_strength) * (line_count - 1));
    if (!tensile_strength)
    {
        JDM_ERROR("Could not allocate memory for material array");
        ill_jfree(G_JALLOCATOR, elastic_modulus);
        ill_jfree(G_JALLOCATOR, density);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* compressive_strength = ill_jalloc(G_JALLOCATOR, sizeof(*compressive_strength) * (line_count - 1));
    if (!compressive_strength)
    {
        JDM_ERROR("Could not allocate memory for material array");
        ill_jfree(G_JALLOCATOR, tensile_strength);
        ill_jfree(G_JALLOCATOR, elastic_modulus);
        ill_jfree(G_JALLOCATOR, density);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    jio_string_segment* labels = ill_jalloc(G_JALLOCATOR, sizeof(*labels) * (line_count - 1));
    if (!labels)
    {
        JDM_ERROR("Could not allocate memory for material array");
        ill_jfree(G_JALLOCATOR, compressive_strength);
        ill_jfree(G_JALLOCATOR, tensile_strength);
        ill_jfree(G_JALLOCATOR, elastic_modulus);
        ill_jfree(G_JALLOCATOR, density);
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    material_parse_float_data float_parse_data[] =
            {
                    {.values = elastic_modulus, .count = 0},
                    {.values = density, .count = 0},
                    {.values = tensile_strength, .count = 0},
                    {.values = compressive_strength, .count = 0},
            };
    material_parse_ss_data ss_parse_data = {.values = labels, .count = 0};
    void* param_array[] = { &ss_parse_data, float_parse_data + 0, float_parse_data + 1, float_parse_data + 2, float_parse_data + 3};


    jio_result jio_res = jio_process_csv_exact(io_ctx, mem_file, ",", MATERIAL_FILE_HEADER_COUNT, MATERIAL_FILE_HEADERS, converter_functions, param_array);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the material input file failed, reason: %s", jio_result_to_str(jio_res));
        ill_jfree(G_JALLOCATOR, labels);
        ill_jfree(G_JALLOCATOR, compressive_strength);
        ill_jfree(G_JALLOCATOR, tensile_strength);
        ill_jfree(G_JALLOCATOR, elastic_modulus);
        ill_jfree(G_JALLOCATOR, density);
        res = JTA_RESULT_BAD_INPUT;
        goto end;
    }
    assert(float_parse_data[0].count == float_parse_data[1].count);
    assert(float_parse_data[2].count == float_parse_data[3].count);
    assert(float_parse_data[1].count == ss_parse_data.count);
    assert(float_parse_data[1].count == float_parse_data[2].count);


    const uint32_t count = ss_parse_data.count;
    assert(count <= line_count - 1);
    if (count != line_count - 1)
    {
        {
            f32* const new_ptr = ill_jrealloc(G_JALLOCATOR, elastic_modulus, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the material array from %zu to %zu bytes",
                         sizeof(*elastic_modulus) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                elastic_modulus = new_ptr;
            }
        }
        {
            f32* const new_ptr = ill_jrealloc(G_JALLOCATOR, density, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the material array from %zu to %zu bytes",
                         sizeof(*density) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                density = new_ptr;
            }
        }
        {
            f32* const new_ptr = ill_jrealloc(G_JALLOCATOR, tensile_strength, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the material array from %zu to %zu bytes",
                         sizeof(*tensile_strength) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                tensile_strength = new_ptr;
            }
        }
        {
            f32* const new_ptr = ill_jrealloc(G_JALLOCATOR, compressive_strength, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the material array from %zu to %zu bytes",
                         sizeof(*compressive_strength) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                compressive_strength = new_ptr;
            }
        }
        {
            jio_string_segment * const new_ptr = ill_jrealloc(G_JALLOCATOR, labels, sizeof(*new_ptr) * count);
            if (!new_ptr)
            {
                JDM_WARN("Failed shrinking the material array from %zu to %zu bytes",
                         sizeof(*labels) * (line_count - 1), sizeof(*new_ptr) * count);
            }
            else
            {
                labels = new_ptr;
            }
        }
    }
    *material_list = (jta_material_list){.count = count, .elastic_modulus = elastic_modulus, .density = density, .compressive_strength = compressive_strength, .tensile_strength = tensile_strength, .labels = labels};

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
end:
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_free_materials(jta_material_list* material_list)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_JALLOCATOR, material_list->density);
    ill_jfree(G_JALLOCATOR, material_list->labels);
    ill_jfree(G_JALLOCATOR, material_list->elastic_modulus);
    ill_jfree(G_JALLOCATOR, material_list->compressive_strength);
    ill_jfree(G_JALLOCATOR, material_list->tensile_strength);

    JDM_LEAVE_FUNCTION;
}
