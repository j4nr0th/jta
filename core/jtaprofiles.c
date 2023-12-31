//
// Created by jan on 5.7.2023.
//

#include "jtaprofiles.h"
#include <jdm.h>

static const jio_string_segment PROFILE_FILE_HEADERS[] =
        {
                {.begin = "profile label", .len = sizeof("profile label") - 1},
                {.begin = "area", .len = sizeof("area") - 1},
                {.begin = "second moment of area", .len = sizeof("second moment of area") - 1},
        };

static const size_t PROFILE_FILE_HEADER_COUNT = sizeof(PROFILE_FILE_HEADERS) / sizeof(*PROFILE_FILE_HEADERS);

typedef struct profile_parse_float_data_T profile_parse_float_data;
struct profile_parse_float_data_T
{
    uint32_t count;
    f32* values;
};

typedef struct profile_parse_ss_data_T profile_parse_ss_data;
struct profile_parse_ss_data_T
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
        if (strncmp(data->values[i].begin, v->begin, v->len) == 0)
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

jta_result jta_load_profiles(const jio_context* io_ctx, const jio_memory_file* mem_file, jta_profile_list* profile_list)
{
    JDM_ENTER_FUNCTION;
    jta_result res;
    uint32_t line_count = jio_memory_file_count_lines(mem_file);
    f32* area = ill_alloc(G_ALLOCATOR, sizeof(*area) * (line_count - 1));
    if (!area)
    {
        JDM_ERROR("Could not allocate memory for point array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    f32* smoa = ill_alloc(G_ALLOCATOR, sizeof(*smoa) * (line_count - 1));
    if (!smoa)
    {
        ill_jfree(G_ALLOCATOR, area);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    jio_string_segment* ss = ill_alloc(G_ALLOCATOR, sizeof(*ss) * (line_count - 1));
    if (!ss)
    {
        ill_jfree(G_ALLOCATOR, area);
        ill_jfree(G_ALLOCATOR, smoa);
        JDM_ERROR("Could not allocate memory for point array");
        res = JTA_RESULT_BAD_ALLOC;
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

    jio_result jio_res = jio_process_csv_exact(io_ctx, mem_file, ",", PROFILE_FILE_HEADER_COUNT, PROFILE_FILE_HEADERS, converter_functions, param_array);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the profile input file failed, reason: %s", jio_result_to_str(jio_res));
        ill_jfree(G_ALLOCATOR, area);
        ill_jfree(G_ALLOCATOR, smoa);
        ill_jfree(G_ALLOCATOR, ss);
        res = JTA_RESULT_BAD_INPUT;
        goto end;
    }
    assert(parse_float_data[0].count == parse_float_data[1].count);
    assert(parse_ss_data.count == parse_float_data[0].count);


    const uint32_t count = parse_float_data[0].count;
    assert(count <= line_count - 1);
    f32* const equivalent_radius = ill_alloc(G_ALLOCATOR, sizeof(*equivalent_radius) * count);
    if (!equivalent_radius)
    {
        ill_jfree(G_ALLOCATOR, area);
        ill_jfree(G_ALLOCATOR, smoa);
        ill_jfree(G_ALLOCATOR, ss);
        JDM_ERROR("Could not allocate memory for profile array");
        res = JTA_RESULT_BAD_ALLOC;
        goto end;
    }
    if (count != line_count - 1)
    {
        f32* new_ptr = ill_jrealloc(G_ALLOCATOR, area, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the profile array from %zu to %zu bytes", sizeof(*area) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            area = new_ptr;
        }
        new_ptr = ill_jrealloc(G_ALLOCATOR, smoa, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the profile array from %zu to %zu bytes", sizeof(*smoa) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            smoa = new_ptr;
        }
        jio_string_segment* const new_ptr1 = ill_jrealloc(G_ALLOCATOR, ss, sizeof(*new_ptr1) * count);
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
//    const __m128 two = _mm_set1_ps(2.0f);
//    __m128 one_over_two_pi = _mm_set1_ps((f32)(M_PI * 2.0));
//    one_over_two_pi = _mm_rcp_ps(one_over_two_pi);
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
    f32 min_r = +INFINITY, max_r = -INFINITY;
    for (i = 0; i < count; ++i)
    {
        const f32 r = equivalent_radius[i];
        if (r < min_r)
        {
            min_r = r;
        }
        if (r > max_r)
        {
            max_r = r;
        }
    }

    *profile_list = (jta_profile_list){.count = count, .labels = ss, .area = area, .second_moment_of_area = smoa, .equivalent_radius = equivalent_radius, .min_equivalent_radius = min_r, .max_equivalent_radius = max_r};

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
    end:
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_free_profiles(jta_profile_list* profile_list)
{
    JDM_ENTER_FUNCTION;

    ill_jfree(G_ALLOCATOR, profile_list->labels);
    ill_jfree(G_ALLOCATOR, profile_list->area);
    ill_jfree(G_ALLOCATOR, profile_list->second_moment_of_area);
    ill_jfree(G_ALLOCATOR, profile_list->equivalent_radius);

    JDM_LEAVE_FUNCTION;
}
