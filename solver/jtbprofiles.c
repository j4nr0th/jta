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

typedef struct profile_parse_data_struct profile_parse_data;
struct profile_parse_data_struct
{
    uint32_t count;
    jtb_profile* profiles;
};

static bool converter_profile_label_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    profile_parse_data* const data = (profile_parse_data*)param;
    for (uint32_t i = 0; i < data->count; ++i)
    {
        if (string_segment_equal(&data->profiles[i].label, v))
        {
            JDM_ERROR("Profile label \"%.*s\" was already defined as profile %u", (int)v->len, v->begin, i);
            JDM_LEAVE_FUNCTION;
            return false;
        }
    }
    data->profiles[data->count++].label = *v;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_area_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    profile_parse_data* const data = (profile_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->profiles[data->count++].area = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool converter_smoa_function(jio_string_segment* v, void* param)
{
    JDM_ENTER_FUNCTION;
    profile_parse_data* const data = (profile_parse_data*)param;
    char* end;
    const f64 v_d = strtod(v->begin, &end);
    if (v->begin + v->len != end)
    {
        JDM_ERROR("Could not convert \"%.*s\" into a single double", (int)v->len, v->begin);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    data->profiles[data->count++].second_moment_of_area = v_d;

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool (*converter_functions[])(jio_string_segment* v, void* param) =
        {
                converter_profile_label_function,
                converter_area_function,
                converter_smoa_function,
        };

jtb_result jtb_load_profiles(const jio_memory_file* mem_file, u32* n_pro, jtb_profile** pp_profiles)
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
    jtb_profile* profile_array = jalloc(G_JALLOCATOR, sizeof(*profile_array) * (line_count - 1));
    if (!profile_array)
    {
        JDM_ERROR("Could not allocate memory for profile array");
        res = JTB_RESULT_BAD_ALLOC;
        goto end;
    }
    profile_parse_data parse_data[] =
            {
                    {.profiles = profile_array, .count = 0},
                    {.profiles = profile_array, .count = 0},
                    {.profiles = profile_array, .count = 0},
            };
    void* param_array[] = {parse_data + 0, parse_data + 1, parse_data + 2};
    jio_res = jio_process_csv_exact(mem_file, ",", PROFILE_FILE_HEADER_COUNT, PROFILE_FILE_HEADERS, converter_functions, param_array, G_LIN_JALLOCATOR);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Processing the profile input file failed, reason: %s", jio_result_to_str(jio_res));
        jfree(G_JALLOCATOR, profile_array);
        res = JTB_RESULT_BAD_INPUT;
        goto end;
    }
    assert(parse_data[0].count == parse_data[1].count);
    assert(parse_data[2].count == parse_data[0].count);


    const uint32_t count = parse_data[0].count;
    assert(count <= line_count - 1);
    if (count != line_count - 1)
    {
        jtb_profile * const new_ptr = jrealloc(G_JALLOCATOR, profile_array, sizeof(*new_ptr) * count);
        if (!new_ptr)
        {
            JDM_WARN("Failed shrinking the profile array from %zu to %zu bytes", sizeof(*profile_array) * (line_count - 1), sizeof(*new_ptr) * count);
        }
        else
        {
            profile_array = new_ptr;
        }
    }
    *n_pro = count;
    *pp_profiles = profile_array;

    JDM_LEAVE_FUNCTION;
    return JTB_RESULT_SUCCESS;
    end:
    JDM_LEAVE_FUNCTION;
    return res;
}
