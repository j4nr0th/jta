//
// Created by jan on 24.7.2023.
//

#include <jdm.h>
#include <jio/iocsv.h>
#include "color_map.h"

static jta_color color_from_floats(vec4 c)
{
    c = vec4_mul_one(c, 255.0f);
    return (jta_color)
            {
                    .r = (uint8_t)(c.s0),
                    .g = (uint8_t)(c.s1),
                    .b = (uint8_t)(c.s2),
                    .a = (uint8_t)(c.s3),
            };
}


jta_color jta_scalar_cmap_color(const jta_scalar_cmap* cmap, float v)
{
    assert(v >= 0);
    assert(v <= 1);

    if (v == 1.0f)
    {
        //  special case
        return color_from_floats(cmap->colors[cmap->count - 1]);
    }

    //  Use binary search to determine the cmap->value[i], such that cmap->value[i] <= v
    uint32_t pos = 0, len = cmap->count, step = cmap->count / 2;
    while (step > 1)
    {
        if (cmap->values[pos + step] <= v)
        {
            pos += step;
            len = cmap->count - pos;
        }
        else
        {
            len -= step;
        }
        step = len / 2;
    }

    if (step && cmap->values[pos + 1] <= v)
    {
        pos += 1;
    }

    const float f_lo = cmap->values[pos];
    const float f_hi = cmap->values[pos + 1];

    const float ratio = (v - f_lo) / (f_hi - f_lo);

    assert(ratio >= 0);
    assert(ratio <= 1);

    vec4 c1 = cmap->colors[pos];
    vec4 c2 = cmap->colors[pos + 1];

    vec4 c_mixed = vec4_add(c1, vec4_mul_one(vec4_sub(c2, c1), ratio));

    return color_from_floats(c_mixed);
}

void jta_scalar_cmap_free(jta_scalar_cmap* cmap)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_JALLOCATOR, cmap->values);
    ill_jfree(G_JALLOCATOR, cmap->colors);

    memset(cmap, 0, sizeof(*cmap));

    JDM_LEAVE_FUNCTION;
}

static const jio_string_segment SCALAR_CMAP_HEADER_ARRAY[5] =
        {
        [0] = {.begin = "value", .len = sizeof("value") - 1},
        [1] = {.begin = "r", .len = sizeof("r") - 1},
        [2] = {.begin = "g", .len = sizeof("g") - 1},
        [3] = {.begin = "b", .len = sizeof("b") - 1},
        [4] = {.begin = "a", .len = sizeof("a") - 1},
        };

static const uint64_t SCALAR_CMAP_HEADER_COUNT = sizeof(SCALAR_CMAP_HEADER_ARRAY) / sizeof(*SCALAR_CMAP_HEADER_ARRAY);

struct converter_value_T
{
    uint32_t count;
    float* values;
};

typedef struct converter_value_T converter_value;

static bool convert_ss_to_value(jio_string_segment* segment, void* param)
{
    converter_value* data = param;
    char* end_p;
    float v = strtof(segment->begin, &end_p);
    if (end_p != segment->begin + segment->len)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single float", (int)segment->len, segment->begin);
        return false;
    }
    if (v < 0 || v > 1)
    {
        JDM_ERROR("Value %g is not in the allowed range [0, 1]", v);
        return false;
    }
    data->values[data->count] = v;
    data->count += 1;
    return true;
}

struct converter_color_component_T
{
    uint32_t count;
    const uint32_t component;
    vec4* values;
};

typedef struct converter_color_component_T converter_color_component;

static bool convert_ss_to_color_component(jio_string_segment* segment, void* param)
{
    converter_color_component* data = param;
    char* end_p;
    float v = strtof(segment->begin, &end_p);
    if (end_p != segment->begin + segment->len)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single float", (int)segment->len, segment->begin);
        return false;
    }
    if (v < 0 || v > 1)
    {
        JDM_ERROR("Value %g is not in the allowed range [0, 1]", v);
        return false;
    }
    data->values[data->count].data[data->component] = v;
    data->count += 1;
    return true;
}

static bool (*SCALAR_CMAP_CONVERTER_FUNCTIONS[5])(jio_string_segment* segment, void* param) =
        {
                convert_ss_to_value,            //  Converter for value
                convert_ss_to_color_component,  //  Converter for red
                convert_ss_to_color_component,  //  Converter for green
                convert_ss_to_color_component,  //  Converter for blue
                convert_ss_to_color_component,  //  Converter for alpha?
        };


jta_result jta_scalar_cmap_from_csv(const jio_context* io_ctx, const char* filename, jta_scalar_cmap* out_cmap)
{
    JDM_ENTER_FUNCTION;
    jta_result res;
    float* values = NULL;
    vec4* colors = NULL;

    jio_memory_file* file;
    jio_result jio_res = jio_memory_file_create(io_ctx, filename, &file, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open colormap file \"%s\", reason: %s", filename, jio_result_to_str(jio_res));
        res = JTA_RESULT_BAD_IO;
        goto failed;
    }

    uint32_t line_count = jio_memory_file_count_lines(file);

    values = ill_jalloc(G_JALLOCATOR, sizeof(*values) * (line_count - 1));
    if (!values)
    {
        JDM_ERROR("Could not allocate memory for colormap values");
        (void)jio_memory_file_destroy(file);
        res = JTA_RESULT_BAD_ALLOC;
        goto failed;
    }

    colors = ill_jalloc(G_JALLOCATOR, sizeof(*colors) * (line_count - 1));
    if (!colors)
    {
        JDM_ERROR("Could not allocate memory for colormap colors");
        (void)jio_memory_file_destroy(file);
        res = JTA_RESULT_BAD_ALLOC;
        goto failed;
    }

    converter_value value_cvt =
            {
            .count = 0,
            .values = values,
            };
    converter_color_component color_cvts[4] =
            {
                    {.count = 0, .values = colors, .component = 0},
                    {.count = 0, .values = colors, .component = 1},
                    {.count = 0, .values = colors, .component = 2},
                    {.count = 0, .values = colors, .component = 3},
            };
    void* param_array[5] =
            {
            &value_cvt,
            color_cvts + 0,
            color_cvts + 1,
            color_cvts + 2,
            color_cvts + 3,
            };
    jio_res = jio_process_csv_exact(io_ctx, file, ",", SCALAR_CMAP_HEADER_COUNT, SCALAR_CMAP_HEADER_ARRAY, SCALAR_CMAP_CONVERTER_FUNCTIONS, param_array);
    (void)jio_memory_file_destroy(file);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not process the colormap file \"%s\", reason: %s", filename, jio_result_to_str(jio_res));
        goto failed;
    }

    assert(value_cvt.count == color_cvts[0].count);
    assert(color_cvts[1].count == color_cvts[2].count);
    assert(color_cvts[0].count == color_cvts[1].count);
    assert(color_cvts[0].count == color_cvts[3].count);
    
    const uint32_t count = value_cvt.count;
    if (line_count - 1 > count)
    {
        float* const new_values = ill_jrealloc(G_JALLOCATOR, values, sizeof(*values) * count);
        if (!new_values)
        {
            JDM_WARN("Could not shrink the values array for the colormap from \"%s\"", filename);
        }
        else
        {
            values = new_values;
        }
        vec4* const new_colors = ill_jrealloc(G_JALLOCATOR, colors, sizeof(*colors) * count);
        if (!new_colors)
        {
            JDM_WARN("Could not shrink the colors array for the colormap from \"%s\"", filename);
        }
        else
        {
            colors = new_colors;
        }
    }
    out_cmap->count = count;
    out_cmap->values = values;
    out_cmap->colors = colors;
    
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;

failed:
    ill_jfree(G_JALLOCATOR, values);
    ill_jfree(G_JALLOCATOR, colors);
    return res;
}
