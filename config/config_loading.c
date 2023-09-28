//
// Created by jan on 23.7.2023.
//

#include <inttypes.h>
#include <jdm.h>
#include <jio/iobase.h>
#include <jio/iocfg.h>
#include "config_loading.h"


static char* get_str_from_section(const jio_cfg_section* section, const char* key_name, bool optional, bool* p_found)
{
    JDM_ENTER_FUNCTION;
    jio_cfg_value val;
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    const jio_string_segment section_name = jio_cfg_section_get_name(section);
    if (res != JIO_RESULT_SUCCESS)
    {
        if (p_found)
        {
            *p_found = false;
        }
        if (!optional)
        {
            JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section_name.len, section_name.begin, jio_result_to_str(res));
        }
        JDM_LEAVE_FUNCTION;
        return NULL;
    }
    if (p_found)
    {
        *p_found = true;
    }
    if (val.type != JIO_CFG_TYPE_STRING)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not a string but %s", key_name, (int)section_name.len, section_name.begin,
                  jio_cfg_type_to_str(val.type));
        JDM_LEAVE_FUNCTION;
        return NULL;
    }
    char* out_ptr = ill_alloc(G_ALLOCATOR, val.value.value_string.len + 1);
    if (!out_ptr)
    {
        JDM_ERROR("Could not allocate %zu bytes of memory for the entry", (size_t)val.value.value_string.len + 1);
        JDM_LEAVE_FUNCTION;
        return NULL;
    }
    memcpy(out_ptr, val.value.value_string.begin, val.value.value_string.len);
    out_ptr[val.value.value_string.len] = 0;
    JDM_LEAVE_FUNCTION;
    return out_ptr;
}

static bool get_float_array_from_section(const jio_cfg_section* section, const char* key_name, uint32_t n_out, float* p_out)
{
    JDM_ENTER_FUNCTION;
    jio_cfg_value val;
    const jio_string_segment section_name = jio_cfg_section_get_name(section);
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section_name.len, section_name.begin,
                  jio_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return false;
    }
    if (val.type != JIO_CFG_TYPE_ARRAY)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not an array of length %u but %s", key_name, (int)section_name.len, section_name.begin, n_out,
                  jio_cfg_type_to_str(val.type));
        JDM_LEAVE_FUNCTION;
        return false;
    }
    const jio_cfg_array* array = &val.value.value_array;
    if (array->count != n_out)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" had %u elements, but it should have had %"PRIu32" instead", key_name, (int)section_name.len, section_name.begin, array->count, n_out);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    for (uint32_t i = 0; i < n_out; ++i)
    {
        const jio_cfg_value* v = array->values + i;
        float float_value;
        switch (v->type)
        {
        case JIO_CFG_TYPE_INT:
            float_value = (float)v->value.value_int;
            break;
        case JIO_CFG_TYPE_REAL:
            float_value = (float)v->value.value_real;
            break;
        default:
            JDM_ERROR("Element %u of the array \"%s\" in section \"%.*s\" was not numeric but was %s", i, key_name, (int)section_name.len, section_name.begin, jio_cfg_type_to_str(v->type));
            JDM_LEAVE_FUNCTION;
            return false;
        }
        p_out[i] = float_value;
    }

    JDM_LEAVE_FUNCTION;
    return true;
}

static bool get_float_from_section(const jio_cfg_section* section, const char* key_name, float* p_out, float min, float max)
{
    JDM_ENTER_FUNCTION;
    jio_cfg_value val;
    const jio_string_segment section_name = jio_cfg_section_get_name(section);
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section_name.len, section_name.begin,
                  jio_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return false;
    }

    float float_value;
    switch (val.type)
    {
    case JIO_CFG_TYPE_INT:
        float_value = (float)val.value.value_int;
        break;
    case JIO_CFG_TYPE_REAL:
        float_value = (float)val.value.value_real;
        break;
    default:
    JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not numeric but was %s", key_name, (int)section_name.len, section_name.begin, jio_cfg_type_to_str(val.type));
        JDM_LEAVE_FUNCTION;
        return false;
    }
    assert(min < max);
    if (float_value < min || float_value > max)
    {
        JDM_ERROR("Value of entry \"%s\" in section \"%.*s\" was outside of allowed range [%g, %g] (value was %g)", key_name, (int)section_name.len, section_name.begin, min, max, float_value);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    *p_out = float_value;
    JDM_LEAVE_FUNCTION;
    return true;
}

static bool get_uint_from_section(const jio_cfg_section* section, const char* key_name, unsigned* p_out, unsigned min, unsigned max)
{
    JDM_ENTER_FUNCTION;
    jio_cfg_value val;
    const jio_string_segment section_name = jio_cfg_section_get_name(section);
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section_name.len, section_name.begin,
                  jio_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return false;
    }

    unsigned u_value;
    if (val.type != JIO_CFG_TYPE_INT)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not numeric but was %s", key_name, (int)section_name.len, section_name.begin, jio_cfg_type_to_str(val.type));
            JDM_LEAVE_FUNCTION;
        return false;
    }
    u_value = (unsigned) val.value.value_int;


    assert(min < max);
    if (u_value < min || u_value > max)
    {
        JDM_ERROR("Value of entry \"%s\" in section \"%.*s\" was outside of allowed range [%u, %u] (value was %u)", key_name, (int)section_name.len, section_name.begin, min, max, u_value);
        JDM_LEAVE_FUNCTION;
        return false;
    }
    *p_out = u_value;
    JDM_LEAVE_FUNCTION;
    return true;
}

static jta_result load_problem_config(const jio_cfg_section* section, jta_config_problem* cfg)
{
    JDM_ENTER_FUNCTION;
    //  Load the definition entries
    const jio_string_segment section_name = jio_cfg_section_get_name(section);
    jio_cfg_section* definitions_section;
    jio_cfg_section* sim_sol_section;
    jio_result jio_res = jio_cfg_get_subsection(section, "definitions", &definitions_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get subsection \"%s\" from section \"%.*s\", reason: %s", "definitions", (int)section_name.len, section_name.begin,
                  jio_result_to_str(jio_res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }

    jio_res = jio_cfg_get_subsection(section, "simulation and solver", &sim_sol_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get subsection \"%s\" from section \"%.*s\", reason: %s", "simulation and solver", (int)section_name.len, section_name.begin,
                  jio_result_to_str(jio_res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }
    memset(cfg, 0, sizeof(*cfg));
    const jio_string_segment definitions_section_name = jio_cfg_section_get_name(definitions_section);

    if (!(cfg->definition.points_file = get_str_from_section(definitions_section, "points", false, NULL))
    ||  !(cfg->definition.materials_file = get_str_from_section(definitions_section, "material_list", false, NULL))
    ||  !(cfg->definition.profiles_file = get_str_from_section(definitions_section, "profiles", false, NULL))
    ||  !(cfg->definition.elements_file = get_str_from_section(definitions_section, "elements", false, NULL))
    ||  !(cfg->definition.natural_bcs_file = get_str_from_section(definitions_section, "natural BCs", false, NULL))
    ||  !(cfg->definition.numerical_bcs_file = get_str_from_section(definitions_section, "numerical BCs", false, NULL)))
    {
        JDM_ERROR("Could not get required entry from section \"%.*s\"", (int)definitions_section_name.len, definitions_section_name.begin);
        ill_jfree(G_ALLOCATOR, cfg->definition.points_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.profiles_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.natural_bcs_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.numerical_bcs_file);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }

    if (!get_uint_from_section(sim_sol_section, "thread count", &cfg->sim_and_sol.thrd_count, 0, UINT32_MAX))
    {
        JDM_WARN("Could not get optional entry \"thread count\" from section \"%.*s\", defaulting to single-threaded", (int)definitions_section_name.len, definitions_section_name.begin);
        cfg->sim_and_sol.thrd_count = 0;
    }

    if (!get_float_array_from_section(sim_sol_section, "gravity", 3, cfg->sim_and_sol.gravity)
    ||  !get_float_from_section(sim_sol_section, "convergence criterion", &cfg->sim_and_sol.convergence_criterion, FLT_EPSILON, FLT_MAX)
    ||  !get_uint_from_section(sim_sol_section, "maximum iterations", &cfg->sim_and_sol.max_iterations, 1, UINT32_MAX)
    ||  !get_float_from_section(sim_sol_section, "relaxation factor", &cfg->sim_and_sol.relaxation_factor, FLT_MIN, FLT_MAX))
    {
        JDM_ERROR("Could not get required entry from section \"%.*s\"", (int)definitions_section_name.len, definitions_section_name.begin);
        ill_jfree(G_ALLOCATOR, cfg->definition.points_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.profiles_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.natural_bcs_file);
        ill_jfree(G_ALLOCATOR, cfg->definition.numerical_bcs_file);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

static void free_problem_config(jta_config_problem* cfg)
{
    ill_jfree(G_ALLOCATOR, cfg->definition.points_file);
    ill_jfree(G_ALLOCATOR, cfg->definition.materials_file);
    ill_jfree(G_ALLOCATOR, cfg->definition.profiles_file);
    ill_jfree(G_ALLOCATOR, cfg->definition.elements_file);
    ill_jfree(G_ALLOCATOR, cfg->definition.natural_bcs_file);
    ill_jfree(G_ALLOCATOR, cfg->definition.numerical_bcs_file);
    memset(cfg, 0, sizeof(*cfg));
}

static jta_color jta_color_from_floats(const float array[4])
{
    return (jta_color)
            {
        .r = (unsigned char)(array[0] * 255.0f),
        .g = (unsigned char)(array[1] * 255.0f),
        .b = (unsigned char)(array[2] * 255.0f),
        .a = (unsigned char)(array[3] * 255.0f),
            };
}

static void free_display_config(jta_config_display* cfg)
{
//    ill_jfree(G_ALLOCATOR, cfg->material_cmap_file);
//    ill_jfree(G_ALLOCATOR, cfg->stress_cmap_file);
    memset(cfg, 0, sizeof(*cfg));
}

static jta_result load_display_config(const jio_cfg_section* section, jta_config_display* cfg)
{
    JDM_ENTER_FUNCTION;
    //  Load the definition entries

    float def_color[4];
//    bool found;
    const jio_string_segment section_name = jio_cfg_section_get_name(section);
//    cfg->material_cmap_file = get_str_from_section(section, "material color map", true, &found);
//    if (!cfg->material_cmap_file)
//    {
//        if (!found)
//        {
//            JDM_WARN("Entry \"material color map\" was not specified in section \"%.*s\", so default value will be used", (int)section_name.len, section_name.begin);
//        }
//        else
//        {
//            JDM_ERROR("Could not get entry from section \"%.*s\"", (int)section_name.len, section_name.begin);
//            goto failed;
//        }
//    }

//    cfg->stress_cmap_file = get_str_from_section(section, "stress color map", true, &found);
//    if (!cfg->stress_cmap_file)
//    {
//        if (!found)
//        {
//            JDM_WARN("Entry \"stress color map\" was not specified in section \"%.*s\", so default value will be used", (int)section_name.len, section_name.begin);
//        }
//        else
//        {
//            JDM_ERROR("Could not get entry from section \"%.*s\"", (int)section_name.len, section_name.begin);
//            ill_jfree(G_ALLOCATOR, cfg->material_cmap_file);
//            goto failed;
//        }
//    }

    if (!get_float_from_section(section, "deformation scale", &cfg->deformation_scale, 0, FLT_MAX)
    ||  !get_float_array_from_section(section, "deformed color", 4, def_color)
    ||  !get_float_array_from_section(section, "DoF point scales", 4, cfg->dof_point_scales)
    ||  !get_float_from_section(section, "force radius ratio", &cfg->force_radius_ratio, 0, FLT_MAX)
    ||  !get_float_from_section(section, "force head ratio", &cfg->force_head_ratio, 0, FLT_MAX)
    ||  !get_float_from_section(section, "force length ratio", &cfg->force_length_ratio, 0, FLT_MAX)
    ||  !get_float_from_section(section, "radius scale", &cfg->radius_scale, 0, FLT_MAX)
    )
    {
        JDM_ERROR("Could not get required entry from section \"%.*s\"", (int)section_name.len, section_name.begin);
//        ill_jfree(G_ALLOCATOR, cfg->stress_cmap_file);
//        ill_jfree(G_ALLOCATOR, cfg->material_cmap_file);
        goto failed;
    }

    cfg->deformed_color = jta_color_from_floats(def_color);

    jio_cfg_value value_color_array;
    jio_result jio_res = jio_cfg_get_value_by_key(section, "DoF point colors", &value_color_array);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", "DoF point colors", (int)section_name.len, section_name.begin,
                  jio_result_to_str(jio_res));
        goto failed;
    }
    if (value_color_array.type != JIO_CFG_TYPE_ARRAY)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not an array of length 4 but %s", "DoF point colors", (int)section_name.len, section_name.begin,
                  jio_cfg_type_to_str(value_color_array.type));
        goto failed;
    }
    const jio_cfg_array* main_array = &value_color_array.value.value_array;
    if (main_array->count != 4)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" had %u elements, but it should have had %"PRIu32" instead", "DoF point colors", (int)section_name.len, section_name.begin, main_array->count, 4);
        goto failed;
    }

    for (uint32_t i = 0; i < 4; ++i)
    {
        const jio_cfg_value* entry = main_array->values + i;
        if (entry->type != JIO_CFG_TYPE_ARRAY)
        {
            JDM_ERROR("Element %u of entry \"%s\" in section \"%.*s\" was not an array of %u numbers, but was %s instead", i, "DoF point colors", (int)section_name.len, section_name.begin, 4,
                      jio_cfg_type_to_str(entry->type));
            goto failed;
        }
        const jio_cfg_array* array = &entry->value.value_array;
        if (array->count != 4)
        {
            JDM_ERROR("Element %u of entry \"%s\" in section \"%.*s\" had %u elements, but it should have had %"PRIu32" instead", i, "DoF point colors", (int)section_name.len, section_name.begin, array->count, 4u);
            goto failed;
        }
        for (uint32_t j = 0; j < 4; ++j)
        {
            const jio_cfg_value* v = array->values + j;
            float float_value;
            switch (v->type)
            {
            case JIO_CFG_TYPE_INT:
                float_value = (float)v->value.value_int;
                break;
            case JIO_CFG_TYPE_REAL:
                float_value = (float)v->value.value_real;
                break;
            default:
            JDM_ERROR("Element %u element %u of the array \"%s\" in section \"%.*s\" was not numeric but was %s", j, i, "DoF point colors", (int)section_name.len, section_name.begin, jio_cfg_type_to_str(v->type));
                JDM_LEAVE_FUNCTION;
                return false;
            }
            def_color[j] = float_value;
        }
        cfg->dof_point_colors[i] = jta_color_from_floats(def_color);
    }

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
failed:
//    ill_jfree(G_ALLOCATOR, cfg->stress_cmap_file);
//    ill_jfree(G_ALLOCATOR, cfg->material_cmap_file);
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_BAD_CFG_ENTRY;
}


static void free_output_configuration(jta_config_output* cfg)
{
    ill_jfree(G_ALLOCATOR, cfg->point_output_file);
    ill_jfree(G_ALLOCATOR, cfg->element_output_file);
//    ill_jfree(G_ALLOCATOR, cfg->general_output_file);
//    ill_jfree(G_ALLOCATOR, cfg->matrix_output_file);
//    ill_jfree(G_ALLOCATOR, cfg->figure_output_file);
    ill_jfree(G_ALLOCATOR, cfg->configuration_file);
}

static jta_result load_output_config(const char* filename, const jio_cfg_section* section, jta_config_output* cfg)
{
    JDM_ENTER_FUNCTION;
    memset(cfg, 0, sizeof(*cfg));
    const size_t filename_len = strlen(filename);
    assert(filename_len != 0);
    char* const filename_copy = ill_alloc(G_ALLOCATOR, sizeof(*filename_copy) * (filename_len + 1));
    if (!filename_copy)
    {
        JDM_ERROR("Could not allocate memory for filename");
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_ALLOC;
    }
    memcpy(filename_copy, filename, sizeof(*filename) * (filename_len + 1));
    cfg->configuration_file = filename_copy;

    const jio_string_segment section_name = jio_cfg_section_get_name(section);
    static const char* const entry_names[] =
            {
            "point output",
            "element output",
//            "general output",
//            "matrix output",
//            "figure output"
            };
    char** const output_ptrs[] =
            {
                    &cfg->point_output_file,
                    &cfg->element_output_file,
//                    &cfg->general_output_file,
//                    &cfg->matrix_output_file,
//                    &cfg->figure_output_file,
            };
    static_assert(sizeof(entry_names) / sizeof(*entry_names) == sizeof(output_ptrs) / sizeof(*output_ptrs));
    for (uint32_t i = 0; i < sizeof(entry_names) / sizeof(*entry_names); ++i)
    {
        bool found;
        if (!((*output_ptrs[i]) = get_str_from_section(section, entry_names[i], true, &found)))
        {
            if (!found)
            {
                JDM_INFO("Entry \"%s\" in section \"%.*s\" was not specified, default value will be used", entry_names[i], (int)section_name.len, section_name.begin);
            }
            else
            {
                ill_jfree(G_ALLOCATOR, filename_copy);
                free_output_configuration(cfg);
                JDM_LEAVE_FUNCTION;
                return JTA_RESULT_BAD_CFG_ENTRY;
            }
        }
    }

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_configuration(const jio_context* io_ctx, const char* filename, jta_config* p_out)
{
    JDM_ENTER_FUNCTION;
    JDM_TRACE("Loading configuration from file \"%s\"", filename);
    jta_result res;


    jio_memory_file* cfg_file;
    jio_result jio_res = jio_memory_file_create(io_ctx, filename, &cfg_file, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open config file \"%s\", reason: %s", filename, jio_result_to_str(jio_res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    jio_cfg_section* root_cfg_section;
    jio_res = jio_cfg_parse(io_ctx, cfg_file, &root_cfg_section);

    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not parse the configuration file, reason: %s", jio_result_to_str(jio_res));
        jio_memory_file_destroy(cfg_file);
        res = JTA_RESULT_BAD_IO;
        goto end;
    }

    memset(p_out, 0, sizeof(*p_out));

    jio_cfg_section* problem_section;
    jio_res = jio_cfg_get_subsection(root_cfg_section, "problem", &problem_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get the problem subsection, reason: %s", jio_result_to_str(jio_res));
        res = JTA_RESULT_BAD_IO;
        goto end;
    }
    jio_cfg_section* display_section;
    jio_res = jio_cfg_get_subsection(root_cfg_section, "display", &display_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get the display subsection, reason: %s", jio_result_to_str(jio_res));
        res = JTA_RESULT_BAD_IO;
        goto end;
    }
    jio_cfg_section* output_section;
    jio_res = jio_cfg_get_subsection(root_cfg_section, "output", &output_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get the output subsection, reason: %s", jio_result_to_str(jio_res));
        res = JTA_RESULT_BAD_IO;
        goto end;
    }



    res = load_problem_config(problem_section, &p_out->problem);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load problem config");
        goto end;
    }

    res = load_display_config(display_section, &p_out->display);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load display config");
        free_problem_config(&p_out->problem);
        goto end;
    }

    res = load_output_config(filename, display_section, &p_out->output);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load output config");
        free_display_config(&p_out->display);
        free_problem_config(&p_out->problem);
        goto end;
    }

end:
    jio_cfg_section_destroy(io_ctx, root_cfg_section, 1);
    jio_memory_file_destroy(cfg_file);
    JDM_LEAVE_FUNCTION;
    return res;
}

jta_result jta_free_configuration(jta_config* cfg)
{
    JDM_ENTER_FUNCTION;
    free_output_configuration(&cfg->output);
    free_display_config(&cfg->display);
    free_problem_config(&cfg->problem);
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_store_configuration(const jio_context* io_ctx, const char* filename, const jta_config* config)
{
    JDM_ENTER_FUNCTION;

    jio_cfg_section* root_section = NULL;
    jio_result res = jio_cfg_section_create(io_ctx, (jio_string_segment) { 0 }, &root_section);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create root cfg section, reason: %s", jio_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    //  The problem section
    jio_cfg_section* problem_section;
    const jio_string_segment name_problem = {.begin = "problem", .len = sizeof("problem") - 1};
    res = jio_cfg_section_create(io_ctx, name_problem, &problem_section);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not create problem section");
        goto failed;
    }

    //      The definitions subsection
    jio_cfg_section* definitions;
    const jio_string_segment name_defs = {.begin = "definitions", .len = sizeof("definitions") - 1};
    res = jio_cfg_section_create(io_ctx, name_defs, &definitions);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, definitions, 1);
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not create definitions section");
        goto failed;
    }
    static const char* definition_key_strings[] =
            {
                "points",
                "material_list",
                "profiles",
                "elements",
                "natural BCs",
                "numerical BCs",
            };
    const jio_cfg_value definition_values[] =
            {
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string.begin = config->problem.definition.points_file},
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string.begin = config->problem.definition.materials_file},
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string.begin = config->problem.definition.profiles_file},
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string.begin = config->problem.definition.elements_file},
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string.begin = config->problem.definition.natural_bcs_file},
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string.begin = config->problem.definition.numerical_bcs_file},
            };
    static_assert(sizeof(definition_key_strings) / sizeof(*definition_key_strings) == sizeof(definition_values) / sizeof(*definition_values));
    const unsigned def_entries = sizeof(definition_key_strings) / sizeof(*definition_key_strings);
    for (unsigned i = 0; i < def_entries; ++i)
    {
        jio_cfg_value v = definition_values[i];
        if (v.type == JIO_CFG_TYPE_STRING && v.value.value_string.begin)
        {
            v.value.value_string.len = strlen(v.value.value_string.begin);
        }
        const jio_cfg_element e = {.value = v, .key = {.begin = definition_key_strings[i], .len = strlen(definition_key_strings[i])}};
        res = jio_cfg_element_insert(io_ctx, definitions, e);
        if (res != JIO_RESULT_SUCCESS)
        {
            jio_cfg_section_destroy(io_ctx, definitions, 1);
            jio_cfg_section_destroy(io_ctx, problem_section, 1);
            JDM_ERROR("Could not insert element \"%s\" in the section \"%s\"", definition_key_strings[i], "definitions");
            goto failed;
        }
    }

    res = jio_cfg_section_insert(io_ctx, problem_section, definitions);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, definitions, 1);
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not insert definitions subsection in the problem section, reason: %s", jio_result_to_str(res));
        goto failed;
    }


    //  The simulation and solve subsection
    jio_cfg_section* sim_and_sol;
    const jio_string_segment name_sim_and_sol = {.begin = "simulation and solver", .len = sizeof("simulation and solver") - 1};
    res = jio_cfg_section_create(io_ctx, name_sim_and_sol, &sim_and_sol);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, sim_and_sol, 1);
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not create simulation and solve section");
        goto failed;
    }
    static const char* sim_and_sol_key_strings[] =
            {
                    "gravity",
                    "thread count",
                    "convergence criterion",
                    "maximum iterations",
                    "relaxation factor",
            };
    jio_cfg_value values_gravity[3] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->problem.sim_and_sol.gravity[0]},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->problem.sim_and_sol.gravity[1]},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->problem.sim_and_sol.gravity[2]},
            };
    jio_cfg_value sim_and_sol_values[] =
            {
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.count = 3, .capacity = 3, .values = values_gravity}},
                    {.type = JIO_CFG_TYPE_INT, .value.value_int = config->problem.sim_and_sol.max_iterations},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->problem.sim_and_sol.convergence_criterion},
                    {.type = JIO_CFG_TYPE_INT, .value.value_int = config->problem.sim_and_sol.max_iterations},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->problem.sim_and_sol.relaxation_factor},
            };
    static_assert(sizeof(sim_and_sol_key_strings) / sizeof(*sim_and_sol_key_strings) == sizeof(sim_and_sol_values) / sizeof(*sim_and_sol_values));
    const unsigned sim_and_sol_entries = sizeof(sim_and_sol_key_strings) / sizeof(*sim_and_sol_key_strings);
    for (unsigned i = 0; i < sim_and_sol_entries; ++i)
    {
        const jio_cfg_element e = {.value = sim_and_sol_values[i], .key = {.begin = sim_and_sol_key_strings[i], .len = strlen(sim_and_sol_key_strings[i])}};
        res = jio_cfg_element_insert(io_ctx, sim_and_sol, e);
        if (res != JIO_RESULT_SUCCESS)
        {
            jio_cfg_section_destroy(io_ctx, sim_and_sol, 1);
            jio_cfg_section_destroy(io_ctx, problem_section, 1);
            JDM_ERROR("Could not insert element \"%s\" in the section \"%s\"", sim_and_sol_key_strings[i], "simulation and solver");
            goto failed;
        }
    }

    res = jio_cfg_section_insert(io_ctx, problem_section, sim_and_sol);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, sim_and_sol, 1);
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not insert simulation and solver subsection in the problem section, reason: %s", jio_result_to_str(res));
        goto failed;
    }

    res = jio_cfg_section_insert(io_ctx, root_section, problem_section);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not insert problem section in the root section, reason: %s", jio_result_to_str(res));
        goto failed;
    }

    //      The display section
    jio_cfg_section* display;
    const jio_string_segment name_display = {.begin = "display", .len = sizeof("display") - 1};
    res = jio_cfg_section_create(io_ctx, name_display, &display);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, display, 1);
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not create display section");
        goto failed;
    }
    static const char* display_key_strings[] =
            {
                    "radius scale",
                    "deformation scale",
                    "deformed color",
                    "DoF point colors",
                    "DoF point scales",
                    "force radius ratio",
                    "force head ratio",
                    "force length ratio",
            };

    jio_cfg_value array_deformed_color[4] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.deformed_color.r / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.deformed_color.g / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.deformed_color.b / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.deformed_color.a / 255.0 },
            };

    jio_cfg_value dof0_color[4] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[0].r / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[0].g / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[0].b / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[0].a / 255.0 },
            };
    jio_cfg_value dof1_color[4] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[1].r / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[1].g / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[1].b / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[1].a / 255.0 },
            };
    jio_cfg_value dof2_color[4] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[2].r / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[2].g / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[2].b / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[2].a / 255.0 },
            };
    jio_cfg_value dof3_color[4] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[3].r / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[3].g / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[3].b / 255.0 },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = (double)config->display.dof_point_colors[3].a / 255.0 },
            };
    jio_cfg_value array_dof_point_colors[4] =
            {
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.capacity = 4, .count = 4, .values = dof0_color}},
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.capacity = 4, .count = 4, .values = dof1_color}},
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.capacity = 4, .count = 4, .values = dof2_color}},
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.capacity = 4, .count = 4, .values = dof3_color}},
            };

    jio_cfg_value array_dof_point_scales[4] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.dof_point_scales[0] },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.dof_point_scales[1] },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.dof_point_scales[2] },
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.dof_point_scales[3] },
            };
    const jio_cfg_value display_values[] =
            {
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.radius_scale},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.deformation_scale},
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.count = 4, .capacity = 4, .values = array_deformed_color}},
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.count = 4, .capacity = 4, .values = array_dof_point_colors}},
                    {.type = JIO_CFG_TYPE_ARRAY, .value.value_array = {.count = 4, .capacity = 4, .values = array_dof_point_scales}},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.force_radius_ratio},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.force_head_ratio},
                    {.type = JIO_CFG_TYPE_REAL, .value.value_real = config->display.force_length_ratio},
            };
    static_assert(sizeof(display_key_strings) / sizeof(*display_key_strings) == sizeof(display_values) / sizeof(*display_values));
    const unsigned display_entries = sizeof(display_key_strings) / sizeof(*display_key_strings);
    for (unsigned i = 0; i < display_entries; ++i)
    {
        const jio_cfg_element e = {.value = display_values[i], .key = {.begin = display_key_strings[i], .len = strlen(display_key_strings[i])}};
        res = jio_cfg_element_insert(io_ctx, display, e);
        if (res != JIO_RESULT_SUCCESS)
        {
            jio_cfg_section_destroy(io_ctx, display, 1);
            JDM_ERROR("Could not insert element \"%s\" in the section \"%s\"", display_key_strings[i], "display");
            goto failed;
        }
    }

    res = jio_cfg_section_insert(io_ctx, root_section, display);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, display, 1);
        JDM_ERROR("Could not insert display section in the root section, reason: %s", jio_result_to_str(res));
        goto failed;
    }

    //      The output section
    jio_cfg_section* output;
    const jio_string_segment name_output = {.begin = "output", .len = sizeof("output") - 1};
    res = jio_cfg_section_create(io_ctx, name_output, &output);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, output, 1);
        jio_cfg_section_destroy(io_ctx, problem_section, 1);
        JDM_ERROR("Could not create output section");
        goto failed;
    }
    static const char* output_key_strings[] =
            {
                    "point output",
                    "element output",
            };

    const jio_cfg_value output_values[] =
            {
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string = {.begin = config->output.point_output_file}},
                    {.type = JIO_CFG_TYPE_STRING, .value.value_string = {.begin = config->output.element_output_file}},
            };
    static_assert(sizeof(output_key_strings) / sizeof(*output_key_strings) == sizeof(output_values) / sizeof(*output_values));
    const unsigned output_entries = sizeof(output_key_strings) / sizeof(*output_key_strings);
    for (unsigned i = 0; i < output_entries; ++i)
    {
        jio_cfg_value v = definition_values[i];
        if (v.type == JIO_CFG_TYPE_STRING && v.value.value_string.begin)
        {
            v.value.value_string.len = strlen(v.value.value_string.begin);
        }
        const jio_cfg_element e = {.value = v, .key = {.begin = output_key_strings[i], .len = strlen(output_key_strings[i])}};
        res = jio_cfg_element_insert(io_ctx, output, e);
        if (res != JIO_RESULT_SUCCESS)
        {
            jio_cfg_section_destroy(io_ctx, output, 1);
            JDM_ERROR("Could not insert element \"%s\" in the section \"%s\"", output_key_strings[i], "output");
            goto failed;
        }
    }

    res = jio_cfg_section_insert(io_ctx, root_section, output);
    if (res != JIO_RESULT_SUCCESS)
    {
        jio_cfg_section_destroy(io_ctx, output, 1);
        JDM_ERROR("Could not insert output section in the root section, reason: %s", jio_result_to_str(res));
        goto failed;
    }

    const size_t required_size = jio_cfg_print_size(root_section, 1, true, true);
//    char* out_buffer = ill_alloc(G_ALLOCATOR, required_size + 1);
//    assert(out_buffer);

    jio_memory_file* out_file;
    res = jio_memory_file_create(io_ctx, filename, &out_file, 1, 1, required_size);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create output memory file for writing, reason: %s", jio_result_to_str(res));
        goto failed;
    }
    jio_memory_file_info f_info = jio_memory_file_get_info(out_file);
    
    const size_t real_size = jio_cfg_print(root_section, (char*)f_info.memory, "=", true, true, false);
    (void)real_size;
    assert(real_size <= required_size);

    jio_memory_file_destroy(out_file);
    jio_cfg_section_destroy(io_ctx, root_section, 0);

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
    
failed:
    jio_cfg_section_destroy(io_ctx, root_section, 0);
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_BAD_IO;
}
