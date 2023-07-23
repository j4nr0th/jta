//
// Created by jan on 23.7.2023.
//

#include <inttypes.h>
#include <jdm.h>
#include <jio/iobase.h>
#include <jio/iocfg.h>
#include "config_loading.h"

static void* jio_alloc(void* param, uint64_t size)
{
    assert(param == G_JALLOCATOR);
    return ill_jalloc(param, size);
}

static void* jio_realloc(void* param, void* ptr, uint64_t new_size)
{
    assert(param == G_JALLOCATOR);
    return ill_jrealloc(param, ptr, new_size);
}

static void jio_free(void* param, void* ptr)
{
    assert(param == G_JALLOCATOR);
    ill_jfree(param, ptr);
}


static char* get_str_from_section(const jio_cfg_section* section, const char* key_name, bool optional, bool* p_found)
{
    JDM_ENTER_FUNCTION;
    jio_cfg_value val;
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        if (p_found)
        {
            *p_found = false;
        }
        if (!optional)
        {
            JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section->name.len, section->name.begin, jio_result_to_str(res));
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
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not a string but %s", key_name, (int)section->name.len, section->name.begin,
                  jio_cfg_type_to_str(val.type));
        JDM_LEAVE_FUNCTION;
        return NULL;
    }
    char* out_ptr = ill_jalloc(G_JALLOCATOR, val.value.value_string.len + 1);
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
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section->name.len, section->name.begin,
                  jio_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return false;
    }
    if (val.type != JIO_CFG_TYPE_ARRAY)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not an array of length %u but %s", key_name, (int)section->name.len, section->name.begin, n_out,
                  jio_cfg_type_to_str(val.type));
        JDM_LEAVE_FUNCTION;
        return false;
    }
    const jio_cfg_array* array = &val.value.value_array;
    if (array->count != n_out)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" had %u elements, but it should have had %"PRIu32" instead", key_name, (int)section->name.len, section->name.begin, array->count, n_out);
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
            JDM_ERROR("Element %u of the array \"%s\" in section \"%.*s\" was not numeric but was %s", i, key_name, (int)section->name.len, section->name.begin, jio_cfg_type_to_str(v->type));
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
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section->name.len, section->name.begin,
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
    JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not numeric but was %s", key_name, (int)section->name.len, section->name.begin, jio_cfg_type_to_str(val.type));
        JDM_LEAVE_FUNCTION;
        return false;
    }
    assert(min < max);
    if (float_value < min || float_value > max)
    {
        JDM_ERROR("Value of entry \"%s\" in section \"%.*s\" was outside of allowed range [%g, %g] (value was %g)", key_name, (int)section->name.len, section->name.begin, min, max, float_value);
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
    jio_result res = jio_cfg_get_value_by_key(section, key_name, &val);
    if (res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", key_name, (int)section->name.len, section->name.begin,
                  jio_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return false;
    }

    unsigned u_value;
    if (val.type != JIO_CFG_TYPE_INT)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not numeric but was %s", key_name, (int)section->name.len, section->name.begin, jio_cfg_type_to_str(val.type));
            JDM_LEAVE_FUNCTION;
        return false;
    }
    u_value = (unsigned) val.value.value_int;


    assert(min < max);
    if (u_value < min || u_value > max)
    {
        JDM_ERROR("Value of entry \"%s\" in section \"%.*s\" was outside of allowed range [%u, %u] (value was %u)", key_name, (int)section->name.len, section->name.begin, min, max, u_value);
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
    jio_cfg_section* definitions_section;
    jio_cfg_section* sim_sol_section;
    jio_result jio_res = jio_cfg_get_subsection(section, "definitions", &definitions_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get subsection \"%s\" from section \"%.*s\", reason: %s", "definitions", (int)section->name.len, section->name.begin,
                  jio_result_to_str(jio_res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }

    jio_res = jio_cfg_get_subsection(section, "simulation and solver", &sim_sol_section);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get subsection \"%s\" from section \"%.*s\", reason: %s", "simulation and solver", (int)section->name.len, section->name.begin,
                  jio_result_to_str(jio_res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }
    memset(cfg, 0, sizeof(*cfg));

    if (!(cfg->definition.points_file = get_str_from_section(definitions_section, "points", false, NULL))
    ||  !(cfg->definition.materials_file = get_str_from_section(definitions_section, "material_list", false, NULL))
    ||  !(cfg->definition.profiles_file = get_str_from_section(definitions_section, "profiles", false, NULL))
    ||  !(cfg->definition.elements_file = get_str_from_section(definitions_section, "elements", false, NULL))
    ||  !(cfg->definition.natural_bcs_file = get_str_from_section(definitions_section, "natural BCs", false, NULL))
    ||  !(cfg->definition.numerical_bcs_file = get_str_from_section(definitions_section, "numerical BCs", false, NULL)))
    {
        JDM_ERROR("Could not get required entry from section \"%.*s\"", (int)definitions_section->name.len, definitions_section->name.begin);
        ill_jfree(G_JALLOCATOR, cfg->definition.points_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.profiles_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.natural_bcs_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.numerical_bcs_file);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }

    if (!get_uint_from_section(sim_sol_section, "thread count", &cfg->sim_and_sol.thrd_count, 0, UINT32_MAX))
    {
        JDM_WARN("Could not get optional entry \"thread count\" from section \"%.*s\", defaulting to single-threaded", (int)definitions_section->name.len, definitions_section->name.begin);
        cfg->sim_and_sol.thrd_count = 0;
    }

    if (!get_float_array_from_section(sim_sol_section, "gravity", 3, cfg->sim_and_sol.gravity)
    ||  !get_float_from_section(sim_sol_section, "convergence criterion", &cfg->sim_and_sol.convergence_criterion, FLT_EPSILON, FLT_MAX)
    ||  !get_uint_from_section(sim_sol_section, "maximum iterations", &cfg->sim_and_sol.max_iterations, 1, UINT32_MAX)
    ||  !get_float_from_section(sim_sol_section, "relaxation factor", &cfg->sim_and_sol.relaxation_factor, FLT_MIN, FLT_MAX))
    {
        JDM_ERROR("Could not get required entry from section \"%.*s\"", (int)definitions_section->name.len, definitions_section->name.begin);
        ill_jfree(G_JALLOCATOR, cfg->definition.points_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.profiles_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.materials_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.natural_bcs_file);
        ill_jfree(G_JALLOCATOR, cfg->definition.numerical_bcs_file);
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_CFG_ENTRY;
    }

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

static void free_problem_config(jta_config_problem* cfg)
{
    ill_jfree(G_JALLOCATOR, cfg->definition.points_file);
    ill_jfree(G_JALLOCATOR, cfg->definition.materials_file);
    ill_jfree(G_JALLOCATOR, cfg->definition.profiles_file);
    ill_jfree(G_JALLOCATOR, cfg->definition.elements_file);
    ill_jfree(G_JALLOCATOR, cfg->definition.natural_bcs_file);
    ill_jfree(G_JALLOCATOR, cfg->definition.numerical_bcs_file);
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
    ill_jfree(G_JALLOCATOR, cfg->material_cmap_file);
    ill_jfree(G_JALLOCATOR, cfg->stress_cmap_file);
    memset(cfg, 0, sizeof(*cfg));
}

static jta_result load_display_config(const jio_cfg_section* section, jta_config_display* cfg)
{
    JDM_ENTER_FUNCTION;
    //  Load the definition entries

    float def_color[4];
    bool found;
    cfg->material_cmap_file = get_str_from_section(section, "material color map", true, &found);
    if (!cfg->material_cmap_file)
    {
        if (!found)
        {
            JDM_WARN("Entry \"material color map\" was not specified in section \"%.*s\", so default value will be used", (int)section->name.len, section->name.begin);
        }
        else
        {
            JDM_ERROR("Could not get entry from section \"%.*s\"", (int)section->name.len, section->name.begin);
            goto failed;
        }
    }

    cfg->stress_cmap_file = get_str_from_section(section, "stress color map", true, &found);
    if (!cfg->stress_cmap_file)
    {
        if (!found)
        {
            JDM_WARN("Entry \"stress color map\" was not specified in section \"%.*s\", so default value will be used", (int)section->name.len, section->name.begin);
        }
        else
        {
            JDM_ERROR("Could not get entry from section \"%.*s\"", (int)section->name.len, section->name.begin);
            ill_jfree(G_JALLOCATOR, cfg->material_cmap_file);
            goto failed;
        }
    }

    if (!get_float_from_section(section, "deformation scale", &cfg->deformation_scale, 0, FLT_MAX)
    ||  !get_float_array_from_section(section, "deformed color", 4, def_color)
    ||  !get_float_array_from_section(section, "DoF point scales", 4, cfg->dof_point_scales)
    ||  !get_float_from_section(section, "force radius ratio", &cfg->force_radius_ratio, 0, FLT_MAX)
    ||  !get_float_from_section(section, "force head ratio", &cfg->force_head_ratio, 0, FLT_MAX)
    ||  !get_float_from_section(section, "force length ratio", &cfg->force_length_ratio, 0, FLT_MAX)
    ||  !get_float_from_section(section, "radius scale", &cfg->radius_scale, 0, FLT_MAX)
    )
    {
        JDM_ERROR("Could not get required entry from section \"%.*s\"", (int)section->name.len, section->name.begin);
        ill_jfree(G_JALLOCATOR, cfg->stress_cmap_file);
        ill_jfree(G_JALLOCATOR, cfg->material_cmap_file);
        goto failed;
    }

    cfg->deformed_color = jta_color_from_floats(def_color);

    jio_cfg_value value_color_array;
    jio_result jio_res = jio_cfg_get_value_by_key(section, "DoF point colors", &value_color_array);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get entry \"%s\" from section \"%.*s\", reason: %s", "DoF point colors", (int)section->name.len, section->name.begin,
                  jio_result_to_str(jio_res));
        goto failed;
    }
    if (value_color_array.type != JIO_CFG_TYPE_ARRAY)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" was not an array of length 4 but %s", "DoF point colors", (int)section->name.len, section->name.begin,
                  jio_cfg_type_to_str(value_color_array.type));
        goto failed;
    }
    const jio_cfg_array* main_array = &value_color_array.value.value_array;
    if (main_array->count != 4)
    {
        JDM_ERROR("Entry \"%s\" in section \"%.*s\" had %u elements, but it should have had %"PRIu32" instead", "DoF point colors", (int)section->name.len, section->name.begin, main_array->count, 4);
        goto failed;
    }

    for (uint32_t i = 0; i < 4; ++i)
    {
        const jio_cfg_value* entry = main_array->values + i;
        if (entry->type != JIO_CFG_TYPE_ARRAY)
        {
            JDM_ERROR("Element %u of entry \"%s\" in section \"%.*s\" was not an array of %u numbers, but was %s instead", i, "DoF point colors", (int)section->name.len, section->name.begin, 4,
                      jio_cfg_type_to_str(entry->type));
            goto failed;
        }
        const jio_cfg_array* array = &entry->value.value_array;
        if (array->count != 4)
        {
            JDM_ERROR("Element %u of entry \"%s\" in section \"%.*s\" had %u elements, but it should have had %"PRIu32" instead", i, "DoF point colors", (int)section->name.len, section->name.begin, array->count, 4u);
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
            JDM_ERROR("Element %u element %u of the array \"%s\" in section \"%.*s\" was not numeric but was %s", j, i, "DoF point colors", (int)section->name.len, section->name.begin, jio_cfg_type_to_str(v->type));
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
    ill_jfree(G_JALLOCATOR, cfg->stress_cmap_file);
    ill_jfree(G_JALLOCATOR, cfg->material_cmap_file);
    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_BAD_CFG_ENTRY;
}


static void free_output_configuration(jta_config_output* cfg)
{
    ill_jfree(G_JALLOCATOR, cfg->point_output_file);
    ill_jfree(G_JALLOCATOR, cfg->element_output_file);
    ill_jfree(G_JALLOCATOR, cfg->general_output_file);
    ill_jfree(G_JALLOCATOR, cfg->matrix_output_file);
    ill_jfree(G_JALLOCATOR, cfg->figure_output_file);
}

static jta_result load_output_config(const jio_cfg_section* section, jta_config_output* cfg)
{
    JDM_ENTER_FUNCTION;
    memset(cfg, 0, sizeof(*cfg));

    static const char* const entry_names[5] =
            {
            "point output",
            "element output",
            "general output",
            "matrix output",
            "figure output"
            };
    char** const output_ptrs[5] =
            {
                    &cfg->point_output_file,
                    &cfg->element_output_file,
                    &cfg->general_output_file,
                    &cfg->matrix_output_file,
                    &cfg->figure_output_file,
            };
    for (uint32_t i = 0; i < sizeof(entry_names) / sizeof(*entry_names); ++i)
    {
        bool found;
        if (!((*output_ptrs[i]) = get_str_from_section(section, entry_names[i], true, &found)))
        {
            if (!found)
            {
                JDM_WARN("Entry \"%s\" in section \"%.*s\" was not specified, default value will be used", entry_names[i], (int)section->name.len, section->name.begin);
            }
            else
            {
                free_output_configuration(cfg);
                JDM_LEAVE_FUNCTION;
                return JTA_RESULT_BAD_CFG_ENTRY;
            }
        }
    }

    JDM_LEAVE_FUNCTION;
    return JTA_RESULT_SUCCESS;
}

jta_result jta_load_configuration(const char* filename, jta_config* p_out)
{
    JDM_ENTER_FUNCTION;
    JDM_TRACE("Loading configuration from file \"%s\"", filename);
    jta_result res = JTA_RESULT_SUCCESS;

    jio_memory_file cfg_file;
    jio_result jio_res = jio_memory_file_create(filename, &cfg_file, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not open config file \"%s\", reason: %s", filename, jio_result_to_str(jio_res));
        JDM_LEAVE_FUNCTION;
        return JTA_RESULT_BAD_IO;
    }

    jio_cfg_section* root_cfg_section;
    const jio_allocator_callbacks jio_allocator =
            {
            .alloc = jio_alloc,
            .realloc = jio_realloc,
            .free = jio_free,
            .param = G_JALLOCATOR,
            };
    jio_res = jio_cfg_parse(&cfg_file, &root_cfg_section, &jio_allocator);

    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not parse the configuration file, reason: %s", jio_result_to_str(jio_res));
        jio_memory_file_destroy(&cfg_file);
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

    res = load_output_config(display_section, &p_out->output);
    if (res != JTA_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not load output config");
        free_display_config(&p_out->display);
        free_problem_config(&p_out->problem);
        goto end;
    }

end:
    jio_cfg_section_destroy(root_cfg_section);
    jio_memory_file_destroy(&cfg_file);
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
