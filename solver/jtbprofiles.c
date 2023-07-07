//
// Created by jan on 5.7.2023.
//

#include "jtbprofiles.h"

static const jio_string_segment PROFILE_FILE_HEADERS[] =
        {
                {.begin = "profile label", .len = sizeof("profile label") - 1},
                {.begin = "area", .len = sizeof("area") - 1},
                {.begin = "second moment of area", .len = sizeof("second moment of area") - 1},
        };

static const size_t PROFILE_FILE_HEADER_COUNT = sizeof(PROFILE_FILE_HEADERS) / sizeof(*PROFILE_FILE_HEADERS);

typedef struct profile_parse_float_data_struct profile_parse_float_data;
struct profile_parse_float_data_struct
{
    uint32_t count;
    f32* values;
};

typedef struct profile_parse_ss_data_struct profile_parse_ss_data;
struct profile_parse_ss_data_struct
{
    uint32_t count;
    jio_string_segment* values;
};

static bool converter_profile_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    profile_parse_ss_data* const data = (profile_parse_ss_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (string_segment_equal(data->values + i, v))
        {
            JDM_ERROR("Profile label \"%.*s\" was already defined as profile %u", (int)v->len, v->begin, i);
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
    profile_parse_float_data* const data = (profile_parse_float_data*)param;
    char* end;
    const f32 v_d = strtof(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    if (v_d <= 0.0f)
    {
        JDM_ERROR("Profile can not have a negative area or SMOA (%g was specified)", (f64)v_d);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->values[data->count++] = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_profile_label_function,
                converter_float_function,
                converter_float_function,
        };

jtb_result jtb_load_profiles(const jio_memory_file* mem_file, jtb_profile_list* profile_list)
{
    JDM_ENTER_FUNCTION;
    jtb_result res;
    uint32_t line_count = 0;
    jio_result jio_res = jio_memory_file_count_lines(mem_file, &line_count);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not count lines in profile input file, reason: %s", jio_result_to_str(jio_res));
        res = JTB_RESULT_BAD_IO;
        goto end;
    }
    f32* area = jalloc(G_JALLOCATOR, sizeof(*area) * (line_count - 1));
    if (!area)
    {
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* smoa = jalloc(G_JALLOCATOR, sizeof(*smoa) * (line_count - 1));
    if (!smoa)
    {
        jfree(G_JALLOCATOR, area);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    jio_string_segment* ss = jalloc(G_JALLOCATOR, sizeof(*ss) * (line_count - 1));
    if (!ss)
    {
        jfree(G_JALLOCATOR, area);
        jfree(G_JALLOCATOR, smoa);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }

    profile_parse_float_data parse_float_data[] =
            {
                    {.values = area, .count = 0},
                    {.values = smoa, .count = 0},
            };
    profile_parse_ss_data parse_ss_data =
            {
            .values = ss, .count = 0,
            };
    void* param_array[] = { &parse_ss_data, parse_float_data + 0, parse_float_data + 1};
    jio_res = jio_process_csv_exact(mem_file, ",", PROFILE_FILE_HEADER_COUNT, PROFILE_FILE_HEADERS, converter_functions, param_array, G_LIN_JALLOCATOR);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the profile input file failed, reason: %s", jio_result_to_str(jio_res));
        jfree(G_JALLOCATOR, area);
        jfree(G_JALLOCATOR, smoa);
        jfree(G_JALLOCATOR, ss);
        res = JTB_RESULT_BAD_INPUT;
        goto end;
    }
    assert(parse_float_data[0].count == parse_float_data[1].count);
    assert(parse_ss_data.count == parse_float_data[0].count);


    const uint32_t count = parse_float_data[0].count;
    assert(count <= line_count - 1);
    f32* const equivalent_radius = jalloc(G_JALLOCATOR, sizeof(*equivalent_radius) * count);
    if (!equivalent_radius)
    {
        jfree(G_JALLOCATOR, area);
        jfree(G_JALLOCATOR, smoa);
        jfree(G_JALLOCATOR, ss);
        JDM_ERROR("Could not allocate memory for profile array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    if (count != line_count - 1)
    {
        f32* new_ptr = jrealloc(G_JALLOCATOR, area, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the profile array from %zu to %zu bytes", sizeof(*area) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            area = new_ptr;
        }
        new_ptr = jrealloc(G_JALLOCATOR, smoa, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the profile array from %zu to %zu bytes", sizeof(*smoa) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            smoa = new_ptr;
        }
        jio_string_segment* const new_ptr1 = jrealloc(G_JALLOCATOR, ss, sizeof(*new_ptr1) * count);
        if (!new_ptr1)
        {
            JDM_WARN("Failed shrinking the profile array from %zu to %zu bytes", sizeof(*ss) * (line_count - 1), sizeof(*new_ptr1) * count);
        }
        else
        {
            ss = new_ptr1;
        }
    }
    memset(equivalent_radius, 0, count * sizeof(*equivalent_radius));
    //  Compute equivalent radii
    const __m128 two = _mm_set1_ps(2.0f);
    __m128 one_over_two_pi = _mm_set1_ps((f32)(M_PI * 2.0));
    one_over_two_pi = _mm_rcp_ps(one_over_two_pi);
    u32 i = 0;
//    for (i = 0; i < (count >> 4); ++i)
//    {
//        __m128 A = _mm_loadu_ps(area + (i << 4));
//        A = _mm_mul_ps(A, one_over_two_pi);
//        __m128 I = _mm_loadu_ps(smoa + (i << 4));
//        __m128 K1 = _mm_rcp_ps(A);
//        K1 = _mm_mul_ps(two, K1);
//        __m128 eqr = _mm_fmadd_ps(K1, I, A);
//        eqr = _mm_sqrt_ps(eqr);
//        _mm_storeu_ps(equivalent_radius + (i << 4), eqr);
//    }
    for (i <<= 2; i < count; ++i)
    {
        equivalent_radius[i] = sqrtf(2.0f * smoa[i] / area[i] + area[i] / (2.0f) * (f32)M_1_PI);
    }

    *profile_list = (jtb_profile_list){.count = count, .labels = ss, .area = area, .second_moment_of_area = smoa, .equivalent_radius = equivalent_radius};

    JDM_LEAVE_FUNCTION;
    return JTB_RESULT_SUCCESS;
    end:
    JDM_LEAVE_FUNCTION;
    return res;
}
