//
// Created by jan on 17.9.2023.
//

#include "top_menu.h"
#include "../common/common.h"
#include "../jta_state.h"
#include "../core/jtaload.h"
#include <stdlib.h>
#include <jdm.h>
#include "../core/jtaexport.h"

#ifdef __GNUC__
//  Platform header
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#endif

enum top_menu_button_T
{
    TOP_MENU_PROBLEM,
    TOP_MENU_DISPLAY,
    TOP_MENU_OUTPUT,
    TOP_MENU_MESH,
    TOP_MENU_SOLVE,
    TOP_MENU_COUNT,
};
typedef enum top_menu_button_T top_menu_button;


static const char* const TOP_MENU_LABELS[TOP_MENU_COUNT] =
        {
                [TOP_MENU_PROBLEM] = "top menu:problem",
                [TOP_MENU_DISPLAY] = "top menu:display",
                [TOP_MENU_OUTPUT] = "top menu:output",
                [TOP_MENU_MESH] = "top menu:mesh",
                [TOP_MENU_SOLVE] = "top menu:solve",
        };

static int file_exists(const char* filename)
{
#ifdef __GNUC__
    return (access(filename, F_OK) == 0);
#endif
}

typedef struct delete_mesh_job_data_T delete_mesh_job_data;
struct delete_mesh_job_data_T
{
    jta_structure_meshes* p_mesh;
    jta_vulkan_window_context* ctx;
};

static void delete_mesh_job(void* param)
{
    delete_mesh_job_data* const data = param;
    jta_structure_meshes_destroy(data->ctx, data->p_mesh);
    ill_jfree(G_JALLOCATOR, data);
}

static void regenerate_meshes(jta_state* p_state, int skip_undeformed, int skip_deformed)
{
    JDM_ENTER_FUNCTION;
    JDM_TRACE("Regenerating meshes (skip undeformed: %d, skip deformed: %d)", skip_undeformed, skip_deformed);
    if (!skip_undeformed && (p_state->problem_state & JTA_PROBLEM_STATE_PROBLEM_LOADED))
    {
        jta_structure_meshes* const old_mesh = p_state->draw_state.undeformed_mesh;
        jta_structure_meshes* new_mesh;
        const gfx_result res = jta_structure_meshes_generate_undeformed(&new_mesh, &p_state->master_config.display, &p_state->problem_setup, p_state->draw_state.wnd_ctx);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not regenerate undeformed mesh, reason: %s", gfx_result_to_str(res));
        }
        else
        {
            if (old_mesh)
            {
                delete_mesh_job_data* const data = ill_jalloc(G_JALLOCATOR, sizeof(delete_mesh_job_data));
                if (data)
                {
                    JDM_TRACE("Replacing undeformed mesh");
                    p_state->draw_state.undeformed_mesh = new_mesh;
                    data->p_mesh = old_mesh;
                    data->ctx = p_state->draw_state.wnd_ctx;
                    jta_vulkan_context_enqueue_frame_job(p_state->draw_state.wnd_ctx, delete_mesh_job, data);
                }
            }
            else
            {
                JDM_TRACE("Replacing undeformed mesh");
                p_state->draw_state.undeformed_mesh = new_mesh;
            }
        }
        vec4 geo_base;
        f32 geo_radius;
        gfx_find_bounding_sphere(&p_state->problem_setup.point_list, &geo_base, &geo_radius);
        jta_camera_3d camera;
        jta_camera_set(
                &camera,                                    //  Camera
                geo_base,                                   //  View target
                geo_base,                                   //  Geometry center
                geo_radius,                                 //  Geometry radius
                vec4_add(geo_base, vec4_mul_one(VEC4(1, 1, 1), geo_radius)), //  Camera position
                VEC4(0, 0, -1),                             //  Down
                4.0f,                                       //  Turn sensitivity
                1.0f                                        //  Move sensitivity
                      );
        p_state->draw_state.camera = camera;
        p_state->draw_state.original_camera = camera;
    }
    if (!skip_deformed && (p_state->problem_state & JTA_PROBLEM_STATE_HAS_SOLUTION))
    {
        jta_structure_meshes* const old_mesh = p_state->draw_state.deformed_mesh;
        jta_structure_meshes* new_mesh;
        const gfx_result res = jta_structure_meshes_generate_deformed(&new_mesh, &p_state->master_config.display, &p_state->problem_setup, &p_state->problem_solution, p_state->draw_state.wnd_ctx);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not regenerate deformed mesh, reason: %s", gfx_result_to_str(res));
        }
        else
        {
            if (old_mesh)
            {
                delete_mesh_job_data* const data = ill_jalloc(G_JALLOCATOR, sizeof(delete_mesh_job_data));
                if (data)
                {
                    JDM_TRACE("Replacing deformed mesh");
                    p_state->draw_state.deformed_mesh = new_mesh;
                    data->p_mesh = old_mesh;
                    data->ctx = p_state->draw_state.wnd_ctx;
                    jta_vulkan_context_enqueue_frame_job(p_state->draw_state.wnd_ctx, delete_mesh_job, data);
                }
            }
            else
            {
                JDM_TRACE("Replacing deformed mesh");
                p_state->draw_state.deformed_mesh = new_mesh;
            }
        }
    }
    JDM_LEAVE_FUNCTION;
}

static void mark_solution_invalid(jta_state* p_state)
{
    p_state->problem_state &= ~JTA_PROBLEM_STATE_HAS_SOLUTION;
    p_state->display_state &= ~JTA_DISPLAY_DEFORMED;
    jrui_widget_base* const toggle_button = jrui_get_by_label(p_state->ui_state.ui_context, "display:mode deformed");
    //  Button may not exist, so it can be NULL
    if (toggle_button)
    {
        jrui_update_toggle_button_state(toggle_button, 0);
    }
}

static void check_for_complete_problem(jta_state* p_state)
{
    static const jta_problem_load_state complete_load_state =
            JTA_PROBLEM_LOAD_STATE_HAS_POINTS|
            JTA_PROBLEM_LOAD_STATE_HAS_MATERIALS|
            JTA_PROBLEM_LOAD_STATE_HAS_PROFILES|
            JTA_PROBLEM_LOAD_STATE_HAS_NUMBC|
            JTA_PROBLEM_LOAD_STATE_HAS_NATBC|
            JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS;
    if ((p_state->problem_setup.load_state & complete_load_state) == complete_load_state)
    {
        p_state->problem_state |= JTA_PROBLEM_STATE_PROBLEM_LOADED;
        regenerate_meshes(p_state, 0, 1);
    }
    else
    {
        p_state->problem_state &= ~JTA_PROBLEM_STATE_PROBLEM_LOADED;
        p_state->display_state &= ~JTA_DISPLAY_UNDEFORMED;
        jrui_widget_base* const toggle_button = jrui_get_by_label(p_state->ui_state.ui_context, "display:mode undeformed");
        if (toggle_button)
        {
            //  Button may not exist yet, so it can be NULL
            jrui_update_toggle_button_state(toggle_button, 0);
        }
    }
}

static void
change_config_input_file(const char* string, jrui_widget_base* widget, const char* status_widget_label, char** p_dest)
{
    while (*string && isspace(*string)) {string += 1;}
    size_t name_len = strlen(string);
    if (!name_len) return;
    JDM_ENTER_FUNCTION;
    char* const copy = ill_jalloc(G_JALLOCATOR, sizeof(*copy) * (name_len + 1));
    if (!copy)
    {
        JDM_ERROR("Could not allocate %zu bytes for string buffer", sizeof(*copy) * (name_len + 1));
        goto end;
    }

    memcpy(copy, string, name_len + 1);
    //  Trim the back
    while (name_len && isspace(copy[name_len - 1])) {name_len -= 1;}
    if (!name_len)
    {
        ill_jfree(G_JALLOCATOR, copy);
        goto end;
    }

    copy[name_len] = 0;

    jrui_context* ctx = jrui_widget_get_context(widget);
    if (!file_exists(copy))
    {
        jrui_update_text_h_by_label(ctx, status_widget_label, "Not Found", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        jrui_update_text_input_text(widget, *p_dest);
        ill_jfree(G_JALLOCATOR, copy);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, status_widget_label, "File Found", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        jrui_update_text_input_text(widget, copy);
        ill_jfree(G_JALLOCATOR, *p_dest);
        *p_dest = copy;
    }

end:
    JDM_LEAVE_FUNCTION;
}

static int convert_to_float_value(jrui_widget_base* widget, const char* string, float v_min, float v_max, float* p_out)
{
    JDM_ENTER_FUNCTION;
    const char* const begin_str = string;
    //  Trim leading spaces
    while (isspace(*string)) {string += 1;}
    char* end_p;
    const float value = strtof(string, &end_p);
    //  Trim trailing spaces
    while (*end_p)
    {
        if (!isspace(*end_p))
        {
            //  There was something else behind the float, so invalid input
            goto failed;
        }
        end_p += 1;
    }
    if (v_min > value || v_max < value)
    {
        //  Outside the allowed range
        goto failed;
    }
    *p_out = value;
    char buffer[32] = { 0 };
    snprintf(buffer, sizeof(buffer), "%g", value);
    jrui_update_text_input_hint(widget, buffer);

    JDM_LEAVE_FUNCTION;
    return 1;

    failed:
    jrui_update_text_input_clear(widget);
    JDM_ERROR("Could not convert \"%s\" to a single float in the range [%g, %g]", begin_str, v_min, v_max);
    JDM_LEAVE_FUNCTION;
    return 0;
}

static int update_color_component(jrui_widget_base* widget, const char* string, unsigned component_idx, jta_color* p_color)
{
    JDM_ENTER_FUNCTION;
    while (*string && isspace(*string)) {string += 1;}
    size_t name_len = strlen(string);
    if (!name_len)
    {
        goto no_replace;
    }
    char* const copy = ill_jalloc(G_JALLOCATOR, sizeof(*copy) * (name_len + 1));
    if (!copy)
    {
        JDM_ERROR("Could not allocate %zu bytes for string buffer", sizeof(*copy) * (name_len + 1));
        goto no_replace;
    }

    memcpy(copy, string, name_len + 1);
    //  Trim the back
    while (name_len && isspace(copy[name_len - 1])) {name_len -= 1;}
    if (!name_len)
    {
        ill_jfree(G_JALLOCATOR, copy);
        goto no_replace;
    }

    copy[name_len] = 0;

    //  Convert to color values
    char* end_p;
    const float v = strtof(copy, &end_p);
    if (end_p != copy + name_len || v < 0.0f || v > 1.0f)
    {
        JDM_ERROR("Could not convert input string \"%s\" to a float in the range [0, 1]", copy);
        ill_jfree(G_JALLOCATOR, copy);
        goto no_replace;
    }
    p_color->data[component_idx] = (unsigned char)(v * 256.0f) - 1;
    jrui_update_text_input_hint(widget, copy);
    ill_jfree(G_JALLOCATOR, copy);

    JDM_LEAVE_FUNCTION;
    return 1;

    no_replace:;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%1.03f", (float)p_color->data[component_idx] / 255.0f);
    jrui_update_text_input_text(widget, buffer);
    jrui_update_text_input_hint(widget, buffer);
    JDM_LEAVE_FUNCTION;
    return 0;
}

static int convert_to_unsigned_value(jrui_widget_base* widget, const char* string, unsigned v_min, unsigned v_max, unsigned* p_out)
{
    JDM_ENTER_FUNCTION;
    const char* const begin_str = string;
    //  Trim leading spaces
    while (isspace(*string)) {string += 1;}
    char* end_p;
    const unsigned long value = strtoul(string, &end_p, 10);
    //  Trim trailing spaces
    while (*end_p)
    {
        if (!isspace(*end_p))
        {
            //  There was something else behind the float, so invalid input
            goto failed;
        }
        end_p += 1;
    }
    if (v_min > value || v_max < value)
    {
        //  Outside the allowed range
        goto failed;
    }
    *p_out = value;
    char buffer[32] = { 0 };
    snprintf(buffer, sizeof(buffer), "%lu", value);
    jrui_update_text_input_hint(widget, buffer);

    JDM_LEAVE_FUNCTION;
    return 1;

failed:
    jrui_update_text_input_clear(widget);
    JDM_ERROR("Could not convert \"%s\" to a single float in the range [%u, %u]", begin_str, v_min, v_max);
    JDM_LEAVE_FUNCTION;
    return 0;
}


//  Function checks if file exists, it updates the associated status widget and possibly config
static void submit_point_file(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    change_config_input_file(
            string, widget, "problem:point status", &p_state->master_config.problem.definition.points_file);
}

static void submit_material_file(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    change_config_input_file(
            string, widget, "problem:material status", &p_state->master_config.problem.definition.materials_file);
}

static void submit_profile_file(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    change_config_input_file(
            string, widget, "problem:profile status", &p_state->master_config.problem.definition.profiles_file);
}

static void submit_natbc_file(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    change_config_input_file(
            string, widget, "problem:natural BC status", &p_state->master_config.problem.definition.natural_bcs_file);
}

static void submit_numbc_file(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    change_config_input_file(
            string, widget, "problem:numerical BC status",
            &p_state->master_config.problem.definition.numerical_bcs_file);
}

static void submit_element_file(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    change_config_input_file(
            string, widget, "problem:element status", &p_state->master_config.problem.definition.elements_file);
}

static void load_points_wrapper(jrui_widget_base* widget, void* param)
{
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_points_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.points_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:point status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:point status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
        check_for_complete_problem(state);
    }
}

static void load_materials_wrapper(jrui_widget_base* widget, void* param)
{
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_materials_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.materials_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void) snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:material status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:material status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
        check_for_complete_problem(state);
    }
}

static void load_profiles_wrapper(jrui_widget_base* widget, void* param)
{
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_profiles_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.profiles_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:profile status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:profile status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
        check_for_complete_problem(state);
    }
}

static void load_natbc_wrapper(jrui_widget_base* widget, void* param)
{
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_natbc_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.natural_bcs_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:natural BC status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:natural BC status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
        check_for_complete_problem(state);
    }
}

static void load_numbc_wrapper(jrui_widget_base* widget, void* param)
{
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_numbc_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.numerical_bcs_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:numerical BC status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:numerical BC status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
        check_for_complete_problem(state);
    }
}

static void load_elements_wrapper(jrui_widget_base* widget, void* param)
{
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_elements_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.elements_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:element status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:element status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
        check_for_complete_problem(state);
    }
}

static void load_all_problem(jrui_widget_base* widget, void* param)
{jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    jta_result res = jta_load_points_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.points_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:point status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:point status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
    }
    res = jta_load_materials_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.materials_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:material status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:material status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
    }
    res = jta_load_profiles_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.profiles_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:profile status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:profile status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
    }
    res = jta_load_natbc_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.natural_bcs_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:natural BC status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:natural BC status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
    }
    res = jta_load_numbc_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.numerical_bcs_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:numerical BC status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:numerical BC status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
    }
    res = jta_load_elements_from_file(
            state->io_ctx, &state->problem_setup, state->master_config.problem.definition.elements_file);
    if (res != JTA_RESULT_SUCCESS)
    {
        char err_buffer[64] = { 0 };
        (void)snprintf(err_buffer, sizeof(err_buffer), "Failed: %s", jta_result_to_str(res));
        jrui_update_text_h_by_label(ctx, "problem:numerical BC status", err_buffer, JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
    }
    else
    {
        jrui_update_text_h_by_label(ctx, "problem:numerical BC status", "Loaded", JRUI_ALIGN_RIGHT, JRUI_ALIGN_CENTER);
        mark_solution_invalid(state);
    }
    check_for_complete_problem(state);
}

static jrui_result top_menu_problem_replace(jta_config_problem* cfg, jrui_widget_base* body)
{
    jrui_widget_create_info point_children[] =
            {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Point file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = !cfg->definition.points_file ? "Not found" : "Loaded", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:point status"}},

            };
    jrui_widget_create_info material_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Material file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = !cfg->definition.materials_file ? "Not found" : "Loaded", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:material status"}},

            };
    jrui_widget_create_info profile_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Profile file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = !cfg->definition.profiles_file ? "Not found" : "Loaded", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:profile status"}},

            };
    jrui_widget_create_info natural_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Natural BC file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = !cfg->definition.natural_bcs_file ? "Not found" : "Loaded", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:natural BC status"}},

            };
    jrui_widget_create_info numerical_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Numerical BC file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = !cfg->definition.numerical_bcs_file ? "Not found" : "Loaded", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:numerical BC status"}},

            };
    jrui_widget_create_info element_children[] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Element file:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_LEFT}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = !cfg->definition.elements_file ? "Not found" : "Loaded", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT, .base_info.label = "problem:element status"}},
            };
    jrui_widget_create_info col_entry_rows[] =
            {
                    //  Point data
                    {
                        .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(point_children) / sizeof(*point_children), .children = point_children,},
                    },
                    {
                        .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_point_file, .current_text = cfg->definition.points_file},
                    },
                    //  Material data
                    {
                        .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(material_children) / sizeof(*material_children), .children = material_children,},
                    },
                    {
                        .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_material_file, .current_text = cfg->definition.materials_file},
                    },
                    //  Profile data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(profile_children) / sizeof(*profile_children), .children = profile_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_profile_file, .current_text = cfg->definition.profiles_file},
                    },
                    //  Natural BCs data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(natural_children) / sizeof(*natural_children), .children = natural_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_natbc_file, .current_text = cfg->definition.natural_bcs_file},
                    },
                    //  Numerical BCs data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(numerical_children) / sizeof(*numerical_children), .children = numerical_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_numbc_file, .current_text = cfg->definition.numerical_bcs_file},
                    },
                    //  Element data
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(element_children) / sizeof(*element_children), .children = element_children,},
                    },
                    {
                            .text_input = { .base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_hint = JRUI_ALIGN_LEFT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_hint = JRUI_ALIGN_CENTER, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_element_file, .current_text = cfg->definition.elements_file},
                    },
            };
    jrui_widget_create_info button_entries[] =
            {
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_points_wrapper}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load Points"}},

                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_materials_wrapper}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load Materials"}},

                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_profiles_wrapper}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load Profiles"}},

                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_natbc_wrapper}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load NatBC"}},

                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_numbc_wrapper}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load NumBC"}},

                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_elements_wrapper}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load Elements"}},

                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_all_problem}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Load All"}},
            };
    jrui_widget_create_info button_stacks[] =
            {
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 0}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 2}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 4}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 6}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 8}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 10}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = button_entries + 12}},
            };
    jrui_widget_create_info col_entry_columns[] =
            {
                    //  Point data
                    {
                            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = 2, .children = col_entry_rows + 0,},
                    },
                    //  Material data
                    {
                            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = 2, .children = col_entry_rows + 2,},
                    },
                    //  Profile data
                    {
                            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = 2, .children = col_entry_rows + 4,},
                    },
                    //  Natural BCs data
                    {
                            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = 2, .children = col_entry_rows + 6,},
                    },
                    //  Numerical BCs data
                    {
                            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = 2, .children = col_entry_rows + 8,},
                    },
                    //  Element data
                    {
                            .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = 2, .children = col_entry_rows + 10,},
                    },
                    //  Load buttons
                    {
                            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(button_stacks) / sizeof(*button_stacks), .children = button_stacks},
                    },
            };
    jrui_widget_create_info col_entry_borders[] =
            {
                    //  Point data
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 0,},
                    },
                    //  Material data
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 1,},
                    },
                    //  Profile data
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 2,},
                    },
                    //  Natural BCs data
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 3,},
                    },
                    //  Numerical BCs data
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 4,},
                    },
                    //  Element data
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 5,},
                    },
                    //  Load buttons
                    {
                            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .child = col_entry_columns + 6,},
                    },
            };
    jrui_widget_create_info col_section =
            {
            .fixed_column = {.base_info.type = JRUI_WIDGET_TYPE_FIXED_COLUMN, .base_info.label = "body", .child_count = sizeof(col_entry_borders) / sizeof(*col_entry_borders), .children = col_entry_borders, .column_alignment = JRUI_ALIGN_TOP, .child_height = 64.0f},
            };

    return jrui_replace_widget(body, col_section);
}


static void submit_radius_scale(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (convert_to_float_value(widget, string, 0.0f, INFINITY, &p_state->master_config.display.radius_scale))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_force_radius_scale(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (convert_to_float_value(widget, string, 0.0f, INFINITY, &p_state->master_config.display.force_radius_ratio))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_force_head_scale(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (convert_to_float_value(widget, string, 0.0f, INFINITY, &p_state->master_config.display.force_head_ratio))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_force_length_scale(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (convert_to_float_value(widget, string, 0.0f, INFINITY, &p_state->master_config.display.force_length_ratio))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_deformation_scale(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (convert_to_float_value(widget, string, 0.0f, INFINITY, &p_state->master_config.display.deformation_scale))
    {
        regenerate_meshes(p_state, 1, 0);
    }
}

static void submit_deformation_color(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (update_color_component(widget, string, (unsigned)(uintptr_t)param, &p_state->master_config.display.deformed_color))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_DoF_color_0(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (update_color_component(widget, string, (unsigned)(uintptr_t)param, p_state->master_config.display.dof_point_colors + 0))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_DoF_color_1(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (update_color_component(widget, string, (unsigned)(uintptr_t)param, p_state->master_config.display.dof_point_colors + 1))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_DoF_color_2(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (update_color_component(widget, string, (unsigned)(uintptr_t)param, p_state->master_config.display.dof_point_colors + 2))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_DoF_color_3(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (update_color_component(widget, string, (unsigned)(uintptr_t)param, p_state->master_config.display.dof_point_colors + 3))
    {
        regenerate_meshes(p_state, 0, 0);
    }
}

static void submit_DoF_scale(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    uintptr_t idx = (uintptr_t) param;
    if (idx < sizeof(p_state->master_config.display.dof_point_scales) / sizeof(*p_state->master_config.display.dof_point_scales))
    {
        if (convert_to_float_value(
                widget, string, 0.0f, INFINITY, p_state->master_config.display.dof_point_scales + idx))
        {
            regenerate_meshes(p_state, 0, 0);
        }
    }
}

static void submit_background_color(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    update_color_component(widget, string, (unsigned)(uintptr_t)param, &p_state->master_config.display.background_color);
}

static void toggle_display_mode(jrui_widget_base* widget, int pressed, void* param)
{
    const jta_display_state dpy_state = (jta_display_state)(uintptr_t)param;
    jta_state* const state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    switch (dpy_state)
    {
    default:return;
    case JTA_DISPLAY_DEFORMED:if ((state->problem_state & JTA_PROBLEM_STATE_HAS_SOLUTION) == 0)
        {
            jrui_update_toggle_button_state(widget, 0);
            return;
        }
        break;
    case JTA_DISPLAY_UNDEFORMED: if ((state->problem_state & JTA_PROBLEM_STATE_PROBLEM_LOADED) == 0)
        {
            jrui_update_toggle_button_state(widget, 0);
            return;
        }
        break;
    case JTA_DISPLAY_NONE:break;
    }
    if (pressed)
    {
        state->display_state |= dpy_state;
        if (dpy_state == JTA_DISPLAY_UNDEFORMED && !state->draw_state.undeformed_mesh)
        {
            regenerate_meshes(state, 0, 1);
        }
    }
    else
    {
        state->display_state &= ~dpy_state;
    }
}

static jrui_result top_menu_display_replace(int show_undeformed, int show_deformed, jta_config_display* cfg, jrui_widget_base* body)
{
    char radius_buffer[16] = { 0 };
    snprintf(radius_buffer, sizeof(radius_buffer), "%g", cfg->radius_scale);
    //  Radius scale
    jrui_widget_create_info radius_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Radius scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:radius", .submit_callback = submit_radius_scale, .current_text = radius_buffer}},
            };
    jrui_widget_create_info radius_row =
            {
            .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(radius_elements) / sizeof(*radius_elements), .children = radius_elements},
            };
    jrui_widget_create_info radius_border =
            {
            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &radius_row},
            };

    char deform_buffer[16];
    snprintf(deform_buffer, sizeof(deform_buffer), "%g", cfg->deformation_scale);
    //  Deformation scale
    jrui_widget_create_info deformation_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Deformation scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation scale", .submit_callback = submit_deformation_scale, .current_text = deform_buffer}},
            };
    jrui_widget_create_info deformation_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(deformation_elements) / sizeof(*deformation_elements), .children = deformation_elements},
            };
    jrui_widget_create_info deformation_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &deformation_row},
            };


    char def_buffer_r[16];
    char def_buffer_g[16];
    char def_buffer_b[16];
    char def_buffer_a[16];
    snprintf(def_buffer_r, sizeof(def_buffer_r), "%1.03f", (float)cfg->deformed_color.r / 255.0f);
    snprintf(def_buffer_g, sizeof(def_buffer_g), "%1.03f", (float)cfg->deformed_color.g / 255.0f);
    snprintf(def_buffer_b, sizeof(def_buffer_b), "%1.03f", (float)cfg->deformed_color.b / 255.0f);
    snprintf(def_buffer_a, sizeof(def_buffer_a), "%1.03f", (float)cfg->deformed_color.a / 255.0f);
    //  Deformation color
    jrui_widget_create_info deformation_color_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 0", .submit_callback = submit_deformation_color, .submit_param = (void*)0, .current_text = def_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 1", .submit_callback = submit_deformation_color, .submit_param = (void*)1, .current_text = def_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 2", .submit_callback = submit_deformation_color, .submit_param = (void*)2, .current_text = def_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:deformation color 3", .submit_callback = submit_deformation_color, .submit_param = (void*)3, .current_text = def_buffer_a}},
            };
    jrui_widget_create_info deformation_color_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = deformation_color_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = deformation_color_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = deformation_color_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = deformation_color_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info deformation_color_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Deformation color", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(deformation_color_pads) / sizeof(*deformation_color_pads), .children = deformation_color_pads}},
            };
    jrui_widget_create_info deformation_color_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(deformation_color_elements) / sizeof(*deformation_color_elements), .children = deformation_color_elements},
            };
    jrui_widget_create_info deformation_color_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &deformation_color_row},
            };

    char dof0_buffer_r[16];
    char dof0_buffer_g[16];
    char dof0_buffer_b[16];
    char dof0_buffer_a[16];
    snprintf(dof0_buffer_r, sizeof(dof0_buffer_r), "%1.03f", (float)cfg->dof_point_colors[0].r / 255.0f);
    snprintf(dof0_buffer_g, sizeof(dof0_buffer_g), "%1.03f", (float)cfg->dof_point_colors[0].g / 255.0f);
    snprintf(dof0_buffer_b, sizeof(dof0_buffer_b), "%1.03f", (float)cfg->dof_point_colors[0].b / 255.0f);
    snprintf(dof0_buffer_a, sizeof(dof0_buffer_a), "%1.03f", (float)cfg->dof_point_colors[0].a / 255.0f);
    //  Dof 0 color
    jrui_widget_create_info color_dof0_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 0", .submit_callback = submit_DoF_color_0, .submit_param = (void*)0, .current_text = dof0_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 1", .submit_callback = submit_DoF_color_0, .submit_param = (void*)1, .current_text = dof0_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 2", .submit_callback = submit_DoF_color_0, .submit_param = (void*)2, .current_text = dof0_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 0 color 3", .submit_callback = submit_DoF_color_0, .submit_param = (void*)3, .current_text = dof0_buffer_a}},
            };
    jrui_widget_create_info color_dof0_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof0_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof0_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof0_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof0_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info color_dof0_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 0 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof0_pads) / sizeof(*color_dof0_pads), .children = color_dof0_pads}},
            };
    jrui_widget_create_info color_dof0_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof0_elements) / sizeof(*color_dof0_elements), .children = color_dof0_elements},
            };
    jrui_widget_create_info color_dof0_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof0_row},
            };

    char dof1_buffer_r[16];
    char dof1_buffer_g[16];
    char dof1_buffer_b[16];
    char dof1_buffer_a[16];
    snprintf(dof1_buffer_r, sizeof(dof1_buffer_r), "%1.03f", (float)cfg->dof_point_colors[1].r / 255.0f);
    snprintf(dof1_buffer_g, sizeof(dof1_buffer_g), "%1.03f", (float)cfg->dof_point_colors[1].g / 255.0f);
    snprintf(dof1_buffer_b, sizeof(dof1_buffer_b), "%1.03f", (float)cfg->dof_point_colors[1].b / 255.0f);
    snprintf(dof1_buffer_a, sizeof(dof1_buffer_a), "%1.03f", (float)cfg->dof_point_colors[1].a / 255.0f);
    //  Dof 1 color
    jrui_widget_create_info color_dof1_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 0", .submit_callback = submit_DoF_color_1, .submit_param = (void*)0, .current_text = dof1_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 1", .submit_callback = submit_DoF_color_1, .submit_param = (void*)1, .current_text = dof1_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 2", .submit_callback = submit_DoF_color_1, .submit_param = (void*)2, .current_text = dof1_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 1 color 3", .submit_callback = submit_DoF_color_1, .submit_param = (void*)3, .current_text = dof1_buffer_a}},
            };
    jrui_widget_create_info color_dof1_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof1_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof1_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof1_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof1_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info color_dof1_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 1 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof1_pads) / sizeof(*color_dof1_pads), .children = color_dof1_pads}},
            };
    jrui_widget_create_info color_dof1_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof1_elements) / sizeof(*color_dof1_elements), .children = color_dof1_elements},
            };
    jrui_widget_create_info color_dof1_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof1_row},
            };


    char dof2_buffer_r[16];
    char dof2_buffer_g[16];
    char dof2_buffer_b[16];
    char dof2_buffer_a[16];
    snprintf(dof2_buffer_r, sizeof(dof2_buffer_r), "%1.03f", (float)cfg->dof_point_colors[2].r / 255.0f);
    snprintf(dof2_buffer_g, sizeof(dof2_buffer_g), "%1.03f", (float)cfg->dof_point_colors[2].g / 255.0f);
    snprintf(dof2_buffer_b, sizeof(dof2_buffer_b), "%1.03f", (float)cfg->dof_point_colors[2].b / 255.0f);
    snprintf(dof2_buffer_a, sizeof(dof2_buffer_a), "%1.03f", (float)cfg->dof_point_colors[2].a / 255.0f);
    //  Dof 2 color
    jrui_widget_create_info color_dof2_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 0", .submit_callback = submit_DoF_color_2, .submit_param = (void*)0, .current_text = dof2_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 1", .submit_callback = submit_DoF_color_2, .submit_param = (void*)1, .current_text = dof2_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 2", .submit_callback = submit_DoF_color_2, .submit_param = (void*)2, .current_text = dof2_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 2 color 3", .submit_callback = submit_DoF_color_2, .submit_param = (void*)3, .current_text = dof2_buffer_a}},
            };
    jrui_widget_create_info color_dof2_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof2_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof2_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof2_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof2_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info color_dof2_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 2 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof2_pads) / sizeof(*color_dof2_pads), .children = color_dof2_pads}},
            };
    jrui_widget_create_info color_dof2_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof2_elements) / sizeof(*color_dof2_elements), .children = color_dof2_elements},
            };
    jrui_widget_create_info color_dof2_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof2_row},
            };


    char dof3_buffer_r[16];
    char dof3_buffer_g[16];
    char dof3_buffer_b[16];
    char dof3_buffer_a[16];
    snprintf(dof3_buffer_r, sizeof(dof3_buffer_r), "%1.03f", (float)cfg->dof_point_colors[3].r / 255.0f);
    snprintf(dof3_buffer_g, sizeof(dof3_buffer_g), "%1.03f", (float)cfg->dof_point_colors[3].g / 255.0f);
    snprintf(dof3_buffer_b, sizeof(dof3_buffer_b), "%1.03f", (float)cfg->dof_point_colors[3].b / 255.0f);
    snprintf(dof3_buffer_a, sizeof(dof3_buffer_a), "%1.03f", (float)cfg->dof_point_colors[3].a / 255.0f);
    //  Dof 3 color
    jrui_widget_create_info color_dof3_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 0", .submit_callback = submit_DoF_color_3, .submit_param = (void*)0, .current_text = dof3_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 1", .submit_callback = submit_DoF_color_3, .submit_param = (void*)1, .current_text = dof3_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 2", .submit_callback = submit_DoF_color_3, .submit_param = (void*)2, .current_text = dof3_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF 3 color 3", .submit_callback = submit_DoF_color_3, .submit_param = (void*)3, .current_text = dof3_buffer_a}},
            };
    jrui_widget_create_info color_dof3_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof3_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof3_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof3_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = color_dof3_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info color_dof3_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF 3 color:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof3_pads) / sizeof(*color_dof3_pads), .children = color_dof3_pads}},
            };
    jrui_widget_create_info color_dof3_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(color_dof3_elements) / sizeof(*color_dof3_elements), .children = color_dof3_elements},
            };
    jrui_widget_create_info color_dof3_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &color_dof3_row},
            };


    char dof_scale_buffer_r[16];
    char dof_scale_buffer_g[16];
    char dof_scale_buffer_b[16];
    char dof_scale_buffer_a[16];
    snprintf(dof_scale_buffer_r, sizeof(dof_scale_buffer_r), "%g", cfg->dof_point_scales[0]);
    snprintf(dof_scale_buffer_g, sizeof(dof_scale_buffer_g), "%g", cfg->dof_point_scales[1]);
    snprintf(dof_scale_buffer_b, sizeof(dof_scale_buffer_b), "%g", cfg->dof_point_scales[2]);
    snprintf(dof_scale_buffer_a, sizeof(dof_scale_buffer_a), "%g", cfg->dof_point_scales[3]);
    //  Dof scale
    jrui_widget_create_info dof_scale_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 0", .submit_callback = submit_DoF_scale, .submit_param = (void*)0, .current_text = dof_scale_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 1", .submit_callback = submit_DoF_scale, .submit_param = (void*)1, .current_text = dof_scale_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 2", .submit_callback = submit_DoF_scale, .submit_param = (void*)2, .current_text = dof_scale_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:DoF scale 3", .submit_callback = submit_DoF_scale, .submit_param = (void*)3, .current_text = dof_scale_buffer_a}},
            };
    jrui_widget_create_info dof_scale_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = dof_scale_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = dof_scale_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = dof_scale_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = dof_scale_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info dof_scale_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "DoF scales:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(dof_scale_pads) / sizeof(*dof_scale_pads), .children = dof_scale_pads}},
            };
    jrui_widget_create_info dof_scale_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(dof_scale_elements) / sizeof(*dof_scale_elements), .children = dof_scale_elements},
            };
    jrui_widget_create_info dof_scale_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &dof_scale_row},
            };

    char buffer_fr[16];
    snprintf(buffer_fr, sizeof(buffer_fr), "%g", cfg->force_radius_ratio);
    //  Force radius ratio
    jrui_widget_create_info force_radius_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Force radius scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:force radius", .submit_callback = submit_force_radius_scale, .current_text = buffer_fr}},
            };
    jrui_widget_create_info force_radius_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(force_radius_elements) / sizeof(*force_radius_elements), .children = force_radius_elements},
            };
    jrui_widget_create_info force_radius_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &force_radius_row},
            };


    char buffer_fh[16];
    snprintf(buffer_fh, sizeof(buffer_fh), "%g", cfg->force_head_ratio);
    //  Force head ratio
    jrui_widget_create_info force_head_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Force head scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:force head", .submit_callback = submit_force_head_scale, .current_text = buffer_fh}},
            };
    jrui_widget_create_info force_head_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(force_head_elements) / sizeof(*force_head_elements), .children = force_head_elements},
            };
    jrui_widget_create_info force_head_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &force_head_row},
            };


    char buffer_lr[16];
    snprintf(buffer_lr, sizeof(buffer_lr), "%g", cfg->force_length_ratio);
    //  Force length ratio
    jrui_widget_create_info force_length_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Force length scale", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:force length", .submit_callback = submit_force_length_scale, .current_text = buffer_lr}},
            };
    jrui_widget_create_info force_length_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(force_length_elements) / sizeof(*force_length_elements), .children = force_length_elements},
            };
    jrui_widget_create_info force_length_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &force_length_row},
            };


    char bg_buffer_r[16];
    char bg_buffer_g[16];
    char bg_buffer_b[16];
    char bg_buffer_a[16];
    snprintf(bg_buffer_r, sizeof(bg_buffer_r), "%1.03f", (float)cfg->background_color.r / 255.0f);
    snprintf(bg_buffer_g, sizeof(bg_buffer_g), "%1.03f", (float)cfg->background_color.g / 255.0f);
    snprintf(bg_buffer_b, sizeof(bg_buffer_b), "%1.03f", (float)cfg->background_color.b / 255.0f);
    snprintf(bg_buffer_a, sizeof(bg_buffer_a), "%1.03f", (float)cfg->background_color.a / 255.0f);
    //  Background color
    jrui_widget_create_info background_color_components[4] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:background color 0", .submit_callback = submit_background_color, .submit_param = (void*)0, .current_text = bg_buffer_r}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:background color 1", .submit_callback = submit_background_color, .submit_param = (void*)1, .current_text = bg_buffer_g}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:background color 2", .submit_callback = submit_background_color, .submit_param = (void*)2, .current_text = bg_buffer_b}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "display:background color 3", .submit_callback = submit_background_color, .submit_param = (void*)3, .current_text = bg_buffer_a}},
            };
    jrui_widget_create_info background_color_pads[4] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = background_color_components + 0, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = background_color_components + 1, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = background_color_components + 2, .pad_left = 1.0f, .pad_right = 1.0f}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .child = background_color_components + 3, .pad_left = 1.0f, .pad_right = 1.0f}},
            };
    jrui_widget_create_info background_color_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Deformation color", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(background_color_pads) / sizeof(*background_color_pads), .children = background_color_pads}},
            };
    jrui_widget_create_info background_color_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(background_color_elements) / sizeof(*background_color_elements), .children = background_color_elements},
            };
    jrui_widget_create_info background_color_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &background_color_row},
            };


    //  Display mode button
    jrui_widget_create_info display_mode_undeformed_elements[2] =
            {
                    {.toggle_button = {.base_info.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .base_info.label = "display:mode undeformed", .toggle_callback = toggle_display_mode, .toggle_param = (void*)JTA_DISPLAY_UNDEFORMED, .initial_state = show_undeformed}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Show Undeformed"}},
            };

    jrui_widget_create_info display_mode_deformed_elements[2] =
            {
                    {.toggle_button = {.base_info.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .base_info.label = "display:mode deformed", .toggle_callback = toggle_display_mode, .toggle_param = (void*)JTA_DISPLAY_DEFORMED, .initial_state = show_deformed}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Show Deformed"}},
            };

    jrui_widget_create_info display_mode_stacks[2] =
            {
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = sizeof(display_mode_undeformed_elements) / sizeof(*display_mode_undeformed_elements), .children = display_mode_undeformed_elements}},
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = sizeof(display_mode_deformed_elements) / sizeof(*display_mode_deformed_elements), .children = display_mode_deformed_elements}}
            };

    jrui_widget_create_info display_mode_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(display_mode_stacks) / sizeof(*display_mode_stacks), .children = display_mode_stacks},
            };
    jrui_widget_create_info display_mode_border =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &display_mode_row},
            };
    
    
    

    jrui_widget_create_info body_entries[] =
            {
            radius_border, deformation_border, deformation_color_border, color_dof0_border, color_dof1_border, color_dof2_border, color_dof3_border, dof_scale_border, force_radius_border, force_head_border, force_length_border, background_color_border, display_mode_border
            };

    jrui_widget_create_info replacement_body =
            {
            .fixed_column = {.base_info = {.type = JRUI_WIDGET_TYPE_FIXED_COLUMN,
//                                           .label = "body"
                                                   }, .child_count = sizeof(body_entries) / sizeof(*body_entries), .children = body_entries, .column_alignment = JRUI_ALIGN_TOP, .child_height = 32.0f}
            };

    jrui_widget_create_info body_constraint =
            {
            .containerf = {.base_info = {.type = JRUI_WIDGET_TYPE_CONTAINERF, .label = "body"}, .align_vertical = JRUI_ALIGN_TOP, .align_horizontal = JRUI_ALIGN_LEFT, .width = 0.33f, .height = 1.0f, .child = &replacement_body}
            };
    return jrui_replace_widget(body, body_constraint);
}


static void update_file_option(jrui_widget_base* widget, const char* string, char** p_dest)
{
    while (*string && isspace(*string)) {string += 1;}
    size_t name_len = strlen(string);
    JDM_ENTER_FUNCTION;
    if (!name_len)
    {
        ill_jfree(G_JALLOCATOR, *p_dest);
        *p_dest = NULL;
        goto end;
    }
    char* const copy = ill_jalloc(G_JALLOCATOR, sizeof(*copy) * (name_len + 1));
    if (!copy)
    {
        JDM_ERROR("Could not allocate %zu bytes for string buffer", sizeof(*copy) * (name_len + 1));
        goto end;
    }

    memcpy(copy, string, name_len + 1);
    //  Trim the back
    while (name_len && isspace(copy[name_len - 1])) {name_len -= 1;}
    if (!name_len)
    {
        ill_jfree(G_JALLOCATOR, copy);
        ill_jfree(G_JALLOCATOR, *p_dest);
        *p_dest = NULL;
        goto end;
    }

    copy[name_len] = 0;

    jrui_update_text_input_text(widget, copy);
    ill_jfree(G_JALLOCATOR, *p_dest);
    *p_dest = copy;

end:
    JDM_LEAVE_FUNCTION;
}

static void submit_point_output(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    update_file_option(widget, string, &p_state->master_config.output.point_output_file);
}

static void submit_element_output(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    update_file_option(widget, string, &p_state->master_config.output.element_output_file);
}

//static void submit_general_output(jrui_widget_base* widget, const char* string, void* param)
//{
//    (void) param;
//    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
//    update_file_option(widget, string, &p_state->master_config.output.general_output_file);
//}
//
//static void submit_matrix_output(jrui_widget_base* widget, const char* string, void* param)
//{
//    (void) param;
//    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
//    update_file_option(widget, string, &p_state->master_config.output.matrix_output_file);
//}
//
//static void submit_figure_output(jrui_widget_base* widget, const char* string, void* param)
//{
//    (void) param;
//    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
//    update_file_option(widget, string, &p_state->master_config.output.figure_output_file);
//}

static void submit_config_output(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    update_file_option(widget, string, &p_state->master_config.output.configuration_file);
}

static void save_point_outputs(jrui_widget_base* widget, void* param)
{
    (void) param;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const p_state = jrui_context_get_user_param(ctx);
    if ((p_state->problem_state & JTA_PROBLEM_STATE_HAS_SOLUTION) && p_state->master_config.output.point_output_file)
    {
        const jta_result res = jta_save_point_solution(p_state->master_config.output.point_output_file, &p_state->problem_setup.point_list, &p_state->problem_solution);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not save the point solution to file");
        }
    }
    else
    {
        JDM_INFO("Can not save results with no solution and/or no output file");
    }
}

static void save_element_outputs(jrui_widget_base* widget, void* param)
{
    (void) param;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const p_state = jrui_context_get_user_param(ctx);
    if ((p_state->problem_state & JTA_PROBLEM_STATE_HAS_SOLUTION) && p_state->master_config.output.element_output_file)
    {
        const jta_result res = jta_save_element_solution(p_state->master_config.output.element_output_file, &p_state->problem_setup.element_list, &p_state->problem_setup.point_list, &p_state->problem_solution);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not save the element solution to file");
        }
    }
    else
    {
        JDM_INFO("Can not save results with no solution and/or no output file");
    }
}

static void save_config(jrui_widget_base* widget, void* param)
{
    JDM_ENTER_FUNCTION;
    const jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    if (p_state->master_config.output.configuration_file)
    {
        const jta_result res = jta_store_configuration(
                p_state->io_ctx, p_state->master_config.output.configuration_file, &p_state->master_config);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not save config to file \"%s\", reason: %s", p_state->master_config.output.configuration_file, jta_result_to_str(res));
        }
    }
    JDM_LEAVE_FUNCTION;
}

static jrui_result top_menu_output_replace(jta_config_output* cfg, jrui_widget_base* body);

static void load_config(jrui_widget_base* widget, void* param)
{
    JDM_ENTER_FUNCTION;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const p_state = jrui_context_get_user_param(ctx);
    if (p_state->master_config.output.configuration_file)
    {
        jta_config new_cfg;
        jta_result res = jta_load_configuration(
                p_state->io_ctx, p_state->master_config.output.configuration_file, &new_cfg);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not load new configuration from \"%s\", reason: %s", p_state->master_config.output.configuration_file,
                      jta_result_to_str(res));
        }
        else
        {
            jta_config old_cfg = p_state->master_config;
            p_state->master_config = new_cfg;
            jta_free_configuration(&old_cfg);
            mark_solution_invalid(p_state);
            jta_free_problem(&p_state->problem_setup);
            jta_solution_free(&p_state->problem_solution);
            p_state->problem_state = 0;
            p_state->display_state = 0;
            jrui_widget_base* body = jrui_get_by_label(ctx, "body");
            if (body)
            {
                jrui_widget_create_info dummy_replace = {.base_info = {.type = JRUI_WIDGET_TYPE_EMPTY, .label = "body"}};
                jrui_replace_widget(body, dummy_replace);
                body = jrui_get_by_label(ctx, "body");
                if (body)
                {
                    top_menu_output_replace(&p_state->master_config.output, body);
                }
            }
        }
    }
    JDM_LEAVE_FUNCTION;
}

static jrui_result top_menu_output_replace(jta_config_output* cfg, jrui_widget_base* body)
{
    const float row_ratios[2] = {1.0f, 3.0f};
    //  Point outputs
    jrui_widget_create_info point_children[2] =
            {
                {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Point output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:point"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_point_output, .current_text = cfg->point_output_file}},
            };
    jrui_widget_create_info point_row =
            {
            .adjustable_row = {.base_info.type = JRUI_WIDGET_TYPE_ADJROW, .child_count = sizeof(point_children) / sizeof(*point_children), .children = point_children, .proportions = row_ratios},
            };
    jrui_widget_create_info border_point =
            {
            .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &point_row}
            };
    static_assert(sizeof(row_ratios) / sizeof(*row_ratios) == sizeof(point_children) / sizeof(*point_children));

    //  Element outputs
    jrui_widget_create_info element_children[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Element output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:element"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_element_output, .current_text = cfg->element_output_file}},
            };
    jrui_widget_create_info element_row =
            {
                    .adjustable_row = {.base_info.type = JRUI_WIDGET_TYPE_ADJROW, .child_count = sizeof(element_children) / sizeof(*element_children), .children = element_children, .proportions = row_ratios},
            };
    jrui_widget_create_info border_element =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &element_row}
            };
    static_assert(sizeof(row_ratios) / sizeof(*row_ratios) == sizeof(element_children) / sizeof(*element_children));

    //  General outputs
//    jrui_widget_create_info general_children[2] =
//            {
//                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "General output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
//                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:general"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_general_output, .current_text = cfg->general_output_file}},
//            };
//    jrui_widget_create_info general_row =
//            {
//                    .adjustable_row = {.base_info.type = JRUI_WIDGET_TYPE_ADJROW, .child_count = sizeof(general_children) / sizeof(*general_children), .children = general_children, .proportions = row_ratios},
//            };
//    jrui_widget_create_info border_general =
//            {
//                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &general_row}
//            };
//    static_assert(sizeof(row_ratios) / sizeof(*row_ratios) == sizeof(general_children) / sizeof(*general_children));

    //  Matrix outputs
//    jrui_widget_create_info matrix_children[2] =
//            {
//                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Matrix output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
//                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:matrix"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_matrix_output, .current_text = cfg->matrix_output_file}},
//            };
//    jrui_widget_create_info matrix_row =
//            {
//                    .adjustable_row = {.base_info.type = JRUI_WIDGET_TYPE_ADJROW, .child_count = sizeof(matrix_children) / sizeof(*matrix_children), .children = matrix_children, .proportions = row_ratios},
//            };
//    jrui_widget_create_info border_matrix =
//            {
//                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &matrix_row}
//            };
//    static_assert(sizeof(row_ratios) / sizeof(*row_ratios) == sizeof(matrix_children) / sizeof(*matrix_children));

    //  Figure outputs
//    jrui_widget_create_info figure_children[2] =
//            {
//                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Figure output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
//                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:figure"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_figure_output, .current_text = cfg->figure_output_file}},
//            };
//    jrui_widget_create_info figure_row =
//            {
//                    .adjustable_row = {.base_info.type = JRUI_WIDGET_TYPE_ADJROW, .child_count = sizeof(figure_children) / sizeof(*figure_children), .children = figure_children, .proportions = row_ratios},
//            };
//    jrui_widget_create_info border_figure =
//            {
//                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &figure_row}
//            };
//    static_assert(sizeof(row_ratios) / sizeof(*row_ratios) == sizeof(figure_children) / sizeof(*figure_children));

    //  Configuration outputs
    jrui_widget_create_info configuration_children[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Config output:", .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_RIGHT}},
                    {.text_input = {.base_info = {.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .label = "output:configuration"}, .align_h_hint = JRUI_ALIGN_LEFT, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .submit_callback = submit_config_output, .current_text = cfg->configuration_file}},
            };
    jrui_widget_create_info configuration_row =
            {
                    .adjustable_row = {.base_info.type = JRUI_WIDGET_TYPE_ADJROW, .child_count = sizeof(configuration_children) / sizeof(*configuration_children), .children = configuration_children, .proportions = row_ratios},
            };
    jrui_widget_create_info border_configuration =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &configuration_row}
            };
    static_assert(sizeof(row_ratios) / sizeof(*row_ratios) == sizeof(configuration_children) / sizeof(*configuration_children));

    jrui_widget_create_info stack_contents_elements[] =
            {
            //  Point outputs
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = save_point_outputs}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Save point"}},

            //  Element outputs
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = save_element_outputs}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Save element"}},

            //  General outputs
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Save general"}},

            //  Matrix outputs
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Save matrix"}},

            //  Figure outputs
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Save figure"}},

            //  Config save
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = save_config}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Save config"}},

            //  Config load
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = load_config}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Load config"}},
            };
    jrui_widget_create_info button_stacks[] =
            {
                    //  Point outputs
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 0}},

                    //  Element outputs
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 2}},

                    //  General outputs
//                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 4}},
//
//                    //  Matrix outputs
//                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 6}},
//
//                    //  Figure outputs
//                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 8}},

                    //  Config save
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 10}},
//
                    //  Config load
                    {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = stack_contents_elements + 12}},
            };
    jrui_widget_create_info button_row =
            {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(button_stacks) / sizeof(*button_stacks), .children = button_stacks}};

    jrui_widget_create_info contents[] =
            {
            border_point, border_element,
//            border_general, border_matrix, border_figure,
            border_configuration, button_row
            };

    jrui_widget_create_info replacement_body =
            {
            .fixed_column = {.base_info = {.type = JRUI_WIDGET_TYPE_FIXED_COLUMN,
//                                           .label = "body"
                                                   }, .child_count = sizeof(contents) / sizeof(*contents), .children = contents, .column_alignment = JRUI_ALIGN_TOP, .child_height = 32.0f},
            };

    jrui_widget_create_info constrained_body =
            {
                    .containerf = {.base_info = {.type = JRUI_WIDGET_TYPE_CONTAINERF, .label = "body"}, .child = &replacement_body, .align_horizontal = JRUI_ALIGN_LEFT, .align_vertical = JRUI_ALIGN_TOP, .width = 0.7f, .height = 1.0f,},
            };

    return jrui_replace_widget(body, constrained_body);
}


static void submit_gravity(jrui_widget_base* widget, const char* string, void* param)
{
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    const uintptr_t idx = (uintptr_t)param;
    if (idx < sizeof(p_state->master_config.problem.sim_and_sol.gravity) / sizeof(*p_state->master_config.problem.sim_and_sol.gravity))
    {
        convert_to_float_value(widget, string, -INFINITY, INFINITY, p_state->master_config.problem.sim_and_sol.gravity + idx);
        p_state->problem_setup.gravity.x = p_state->master_config.problem.sim_and_sol.gravity[0];
        p_state->problem_setup.gravity.y = p_state->master_config.problem.sim_and_sol.gravity[1];
        p_state->problem_setup.gravity.z = p_state->master_config.problem.sim_and_sol.gravity[2];
        mark_solution_invalid(p_state);
    }
}

static void submit_threads(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    convert_to_unsigned_value(widget, string, 1, ~0u, &p_state->master_config.problem.sim_and_sol.thrd_count);
}

static void submit_convergence(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const p_state = jrui_context_get_user_param(ctx);
    if (convert_to_float_value(widget, string, 1e-6f, 1.0f, &p_state->master_config.problem.sim_and_sol.convergence_criterion))
    {
        jrui_widget_base* drag = jrui_get_by_label(ctx, "solve:drag convergence");
        mark_solution_invalid(p_state);
        if (drag)
        {
            //  Reverse the transformation
            const float v = p_state->master_config.problem.sim_and_sol.convergence_criterion;
            const float arg = 1.0f + log10f(v) / 6.0f;
            jrui_update_drag_h_position(drag, arg);
        }
    }
}

static void submit_convergence_drag(jrui_widget_base* widget, float v, void* param)
{
    JDM_ENTER_FUNCTION;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);
    
    const float exponent = -6.0f * (1.0f - v);
    const float residual_value = powf(10.0f, exponent);
    char buffer[32] = { 0 };
    snprintf(buffer, sizeof(buffer), "%g", residual_value);
    if (residual_value < state->master_config.problem.sim_and_sol.convergence_criterion)
    {
        mark_solution_invalid(state);
    }
    state->master_config.problem.sim_and_sol.convergence_criterion = residual_value;
    jrui_widget_base* text = jrui_get_by_label(ctx, "solve:convergence");
    if (text)
    {
        jrui_update_text_input_hint(text, buffer);
        jrui_update_text_input_text(text, buffer);
    }

    JDM_LEAVE_FUNCTION;
}

static void submit_iterations(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jta_state* const p_state = jrui_context_get_user_param(jrui_widget_get_context(widget));
    const unsigned old_value = p_state->master_config.problem.sim_and_sol.thrd_count;
    convert_to_unsigned_value(widget, string, 1, ~0u, &p_state->master_config.problem.sim_and_sol.max_iterations);
    if (old_value < p_state->master_config.problem.sim_and_sol.max_iterations)
    {
        mark_solution_invalid(p_state);
    }
}

static void submit_relaxation(jrui_widget_base* widget, const char* string, void* param)
{
    (void) param;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const p_state = jrui_context_get_user_param(ctx);
    if (convert_to_float_value(widget, string, 0.0f, 1.0f, &p_state->master_config.problem.sim_and_sol.relaxation_factor))
    {
        jrui_widget_base* drag = jrui_get_by_label(ctx, "solve:drag relaxation");
        if (drag)
        {
            //  Reverse the transformation
            const float v = p_state->master_config.problem.sim_and_sol.relaxation_factor;
            jrui_update_drag_h_position(drag, v);
        }
    }
}

static void submit_relaxation_drag(jrui_widget_base* widget, float v, void* param)
{
    JDM_ENTER_FUNCTION;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const state = jrui_context_get_user_param(ctx);


    char buffer[32] = { 0 };
    snprintf(buffer, sizeof(buffer), "%g", v);
    state->master_config.problem.sim_and_sol.relaxation_factor = v;
    jrui_widget_base* text = jrui_get_by_label(ctx, "solve:relaxation");
    if (text)
    {
        jrui_update_text_input_hint(text, buffer);
        jrui_update_text_input_text(text, buffer);
    }

    JDM_LEAVE_FUNCTION;
}

static void submit_the_problem_to_solve(jrui_widget_base* widget, void* param)
{
    JDM_ENTER_FUNCTION;
    jrui_context* ctx = jrui_widget_get_context(widget);
    jta_state* const p_state = jrui_context_get_user_param(ctx);
    if ((p_state->problem_state & JTA_PROBLEM_STATE_HAS_SOLUTION) != 0)
    {
        JDM_LEAVE_FUNCTION;
        return;
    }
    if ((p_state->problem_setup.load_state & JTA_PROBLEM_LOAD_STATE_HAS_ELEMENTS) &&
        (p_state->problem_setup.load_state & JTA_PROBLEM_LOAD_STATE_HAS_NATBC) &&
        (p_state->problem_setup.load_state & JTA_PROBLEM_LOAD_STATE_HAS_NUMBC))
    {
        jta_solution_free(&p_state->problem_solution);
        jta_result res = jta_solve_problem(
                &p_state->master_config.problem, &p_state->problem_setup, &p_state->problem_solution);
        if (res != JTA_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not solve problem, reason: %s", jta_result_to_str(res));
        }
        else
        {
            p_state->problem_state |= JTA_PROBLEM_STATE_HAS_SOLUTION;
            regenerate_meshes(p_state, 1, 0);
            res = jta_postprocess(&p_state->problem_setup, &p_state->problem_solution);
            if (res != JTA_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not post-process the solution, reason: %s", jta_result_to_str(res));
            }
        }
    }
    else
    {
        JDM_ERROR("Problem does not have defined elements, natural BCs, and/or numerical BCs");
    }
    JDM_LEAVE_FUNCTION;
}

static jrui_result top_menu_solve_replace(jta_config_problem* cfg, jrui_widget_base* body)
{
    char g_buffer_0[16];
    char g_buffer_1[16];
    char g_buffer_2[16];
    snprintf(g_buffer_0, sizeof(g_buffer_0), "%g", cfg->sim_and_sol.gravity[0]);
    snprintf(g_buffer_1, sizeof(g_buffer_1), "%g", cfg->sim_and_sol.gravity[1]);
    snprintf(g_buffer_2, sizeof(g_buffer_2), "%g", cfg->sim_and_sol.gravity[2]);
    //  gravity
    jrui_widget_create_info gravity_inputs[3] =
            {
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 0", .submit_callback = submit_gravity, .submit_param = (void*)0, .current_text = g_buffer_0}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 1", .submit_callback = submit_gravity, .submit_param = (void*)1, .current_text = g_buffer_1}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:gravity 2", .submit_callback = submit_gravity, .submit_param = (void*)2, .current_text = g_buffer_2}},
            };
    jrui_widget_create_info gravity_components[3] =
            {
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .pad_left = 2.0f, .pad_right = 1.0f, .pad_top = 2.0f, .pad_bottom = 2.0f, .child = gravity_inputs + 0}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .pad_left = 1.0f, .pad_right = 1.0f, .pad_top = 2.0f, .pad_bottom = 2.0f, .child = gravity_inputs + 1}},
                    {.pad = {.base_info.type = JRUI_WIDGET_TYPE_PAD, .pad_left = 1.0f, .pad_right = 2.0f, .pad_top = 2.0f, .pad_bottom = 2.0f, .child = gravity_inputs + 2}},
            };
    jrui_widget_create_info gravity_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Gravity:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(gravity_components) / sizeof(*gravity_components), .children = gravity_components}},
            };
    jrui_widget_create_info gravity_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(gravity_elements) / sizeof(*gravity_elements), .children = gravity_elements},
            };
    jrui_widget_create_info border_gravity =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &gravity_row},
            };

    char t_buffer_0[16];
    snprintf(t_buffer_0, sizeof(t_buffer_0), "%u", cfg->sim_and_sol.thrd_count);
    //  threads
    jrui_widget_create_info threads_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Thread count:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:threads", .submit_callback = submit_threads, .current_text = t_buffer_0}},
            };
    jrui_widget_create_info threads_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(threads_elements) / sizeof(*threads_elements), .children = threads_elements},
            };
    jrui_widget_create_info border_threads =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &threads_row},
            };

    char c_buffer_0[16];
    snprintf(c_buffer_0, sizeof(c_buffer_0), "%g", cfg->sim_and_sol.convergence_criterion);
    float pos_c = 1.0f + log10f(cfg->sim_and_sol.convergence_criterion) / 6.0f;
    if (pos_c < 0.0f)
    {
        pos_c = 0.0f;
    }
    else if (pos_c > 1.0f)
    {
        pos_c = 1.0f;
    }
    //  convergence
    jrui_widget_create_info convergence_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Max residual:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:convergence", .submit_callback = submit_convergence, .current_text = c_buffer_0}},
            };
    jrui_widget_create_info convergence_rows[] =
            {
                    { .row = { .base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(convergence_elements) /
                                                                                      sizeof(*convergence_elements), .children = convergence_elements }},
                    { .drag_h = {.base_info = {.type = JRUI_WIDGET_TYPE_DRAG_H, .label = "solve:drag convergence"}, .callback = submit_convergence_drag, .btn_ratio = 0.1f, .initial_pos = pos_c}},
            };
    jrui_widget_create_info convergence_col =
            {
                    .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = sizeof(convergence_rows) / sizeof(*convergence_rows), .children = convergence_rows},
            };
    jrui_widget_create_info border_convergence =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &convergence_col},
            };

    char i_buffer_0[16];
    snprintf(i_buffer_0, sizeof(i_buffer_0), "%u", cfg->sim_and_sol.max_iterations);
    //  iterations
    jrui_widget_create_info iterations_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Iteration count:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:iterations", .submit_callback = submit_iterations, .current_text = i_buffer_0}},
            };
    jrui_widget_create_info iterations_row =
            {
                    .row = {.base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(iterations_elements) / sizeof(*iterations_elements), .children = iterations_elements},
            };
    jrui_widget_create_info border_iterations =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &iterations_row},
            };

    char r_buffer_0[16];
    snprintf(r_buffer_0, sizeof(r_buffer_0), "%g", cfg->sim_and_sol.relaxation_factor);
    float pos_r = cfg->sim_and_sol.relaxation_factor;
    if (pos_r < 0.0f)
    {
        pos_r = 0.0f;
    }
    else if (pos_r > 1.0f)
    {
        pos_r = 1.0f;
    }
    //  relaxation
    jrui_widget_create_info relaxation_elements[2] =
            {
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text = "Relaxation factor:", .text_alignment_horizontal = JRUI_ALIGN_LEFT, .text_alignment_vertical = JRUI_ALIGN_CENTER}},
                    {.text_input = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_INPUT, .align_h_text = JRUI_ALIGN_RIGHT, .align_v_text = JRUI_ALIGN_CENTER, .align_v_hint = JRUI_ALIGN_CENTER, .align_h_hint = JRUI_ALIGN_LEFT, .base_info.label = "solve:relaxation", .submit_callback = submit_relaxation, .current_text = r_buffer_0}},
            };
    jrui_widget_create_info relaxation_rows[] =
            {
                    { .row = { .base_info.type = JRUI_WIDGET_TYPE_ROW, .child_count = sizeof(relaxation_elements) /
                                                                                      sizeof(*relaxation_elements), .children = relaxation_elements }},
                    { .drag_h = {.base_info = {.type = JRUI_WIDGET_TYPE_DRAG_H, .label = "solve:drag relaxation"}, .callback = submit_relaxation_drag, .btn_ratio = 0.1f, .initial_pos = pos_r}},
            };
    jrui_widget_create_info relaxation_col =
            {
                    .column = {.base_info.type = JRUI_WIDGET_TYPE_COLUMN, .child_count = sizeof(relaxation_rows) / sizeof(*relaxation_rows), .children = relaxation_rows},
            };
    jrui_widget_create_info border_relaxation =
            {
                    .border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &relaxation_col},
            };

    //  the doomsday button
    jrui_widget_create_info submit_stack_elements[2] =
            {
                    {.button = {.base_info.type = JRUI_WIDGET_TYPE_BUTTON, .btn_callback = submit_the_problem_to_solve}},
                    {.text_h = {.base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_vertical = JRUI_ALIGN_CENTER, .text_alignment_horizontal = JRUI_ALIGN_CENTER, .text = "Solve"}},
            };
    jrui_widget_create_info submit_stack = {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = sizeof(submit_stack_elements) / sizeof(*submit_stack_elements), .children = submit_stack_elements}};
    jrui_widget_create_info submit_container = {.containerf = {.base_info.type = JRUI_WIDGET_TYPE_CONTAINERF, .height = 1.0f, .width = 0.2f, .align_vertical = JRUI_ALIGN_CENTER, .align_horizontal = JRUI_ALIGN_CENTER, .child = &submit_stack}};
    jrui_widget_create_info border_submit = {.border = {.base_info.type = JRUI_WIDGET_TYPE_BORDER, .border_thickness = 1.0f, .child = &submit_container}};

    jrui_widget_create_info body_contents[] =
            {
            border_gravity, border_threads, border_convergence, border_iterations, border_relaxation, border_submit
            };
    jrui_widget_create_info replacement_body =
            {
            .fixed_column = {.base_info = {.type = JRUI_WIDGET_TYPE_FIXED_COLUMN,
//                                           .label = "body"
                                                   }, .child_count = sizeof(body_contents) / sizeof(*body_contents), .children = body_contents, .column_alignment = JRUI_ALIGN_TOP, .child_height = 64.0f}
            };

    jrui_widget_create_info body_constraint =
            {
                    .containerf = {.base_info = {.type = JRUI_WIDGET_TYPE_CONTAINERF, .label = "body"}, .align_vertical = JRUI_ALIGN_TOP, .align_horizontal = JRUI_ALIGN_LEFT, .width = 0.33f, .height = 1.0f, .child = &replacement_body}
            };
    
    return jrui_replace_widget(body, body_constraint);
}

static jrui_result top_menu_mesh_replace(jrui_widget_base* body)
{

    jrui_widget_create_info replacement_body =
            {
            .base_info = {.type = JRUI_WIDGET_TYPE_EMPTY, .label = "body"}
            };
    return jrui_replace_widget(body, replacement_body);
}


static void top_menu_toggle_on_callback(jrui_widget_base* widget, int pressed, void* param)
{
    const top_menu_button button = (top_menu_button)(uintptr_t)param;
    if (!pressed || button < TOP_MENU_PROBLEM || button >= TOP_MENU_COUNT)
    {
        return;
    }
    JDM_ENTER_FUNCTION;
    jrui_context* ctx = jrui_widget_get_context(widget);
    //  Set other button's states to zero
    for (top_menu_button btn = 0; btn < TOP_MENU_COUNT; ++btn)
    {
        if (button != btn)
        {
            jrui_update_toggle_button_state_by_label(ctx, TOP_MENU_LABELS[btn], 0);
        }
    }
    
    // Replace the body of the UI with what the actual menu entry wants
    jta_state* const state = jrui_context_get_user_param(ctx);
    jrui_widget_base* const body = jrui_get_by_label(ctx, "body");
    jrui_result replace_res;
    switch (button)
    {
    case TOP_MENU_PROBLEM:
        replace_res = top_menu_problem_replace(&state->master_config.problem, body);
        break;
    case TOP_MENU_DISPLAY:
        replace_res = top_menu_display_replace((int)(state->display_state & JTA_DISPLAY_UNDEFORMED), (int)(state->display_state & JTA_DISPLAY_DEFORMED), &state->master_config.display, body);
        break;
    case TOP_MENU_OUTPUT:
        replace_res = top_menu_output_replace(&state->master_config.output, body);
        break;
    case TOP_MENU_SOLVE:
        replace_res = top_menu_solve_replace(&state->master_config.problem, body);
        break;
    case TOP_MENU_MESH:
        replace_res = top_menu_mesh_replace(body);
        break;
    default:assert(0);
    }
    if (replace_res != JRUI_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not replace UI body widget, reason: %s (%s)", jrui_result_to_str(replace_res), jrui_result_message(replace_res));
    }
    JDM_LEAVE_FUNCTION;
}

static void close_callback(jrui_widget_base* widget, void* param) { (void)param; (void)widget; exit(EXIT_SUCCESS); }
static const jrui_widget_create_info TOP_MENU_BUTTONS[2 * (TOP_MENU_COUNT + 1)] =
        {
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:problem"}, 
                        .toggle_param = (void*)TOP_MENU_PROBLEM, 
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                    {
                        .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                        .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Problem"
                    }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:display"},
                        .toggle_param = (void*)TOP_MENU_DISPLAY,
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Display"
                        }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:output"},
                        .toggle_param = (void*)TOP_MENU_OUTPUT,
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Output"
                        }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:mesh"},
                        .toggle_param = (void*)TOP_MENU_MESH, 
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Mesh"
                        }
                },
                {.toggle_button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_TOGGLE_BUTTON, .label = "top menu:solve"},
                        .toggle_param = (void*)TOP_MENU_SOLVE, 
                        .toggle_callback = top_menu_toggle_on_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Solve"
                        }
                },
                {.button = 
                    {
                        .base_info = {.type = JRUI_WIDGET_TYPE_BUTTON, .label = "top menu:close"}, 
                        .btn_callback = close_callback
                    }
                },
                {.text_h =
                        {
                                .base_info.type = JRUI_WIDGET_TYPE_TEXT_H, .text_alignment_horizontal = JRUI_ALIGN_CENTER,
                                .text_alignment_vertical = JRUI_ALIGN_CENTER, .text = "Close"
                        }
                },
        };
static const jrui_widget_create_info TOP_MENU_BUTTON_STACKS[TOP_MENU_COUNT + 1] =
        {
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 0)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 1)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 2)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 3)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 4)}},
                {.stack = {.base_info.type = JRUI_WIDGET_TYPE_STACK, .child_count = 2, .children = TOP_MENU_BUTTONS + (ptrdiff_t)(2 * 5)}},
        };

const jrui_widget_create_info TOP_MENU_TREE =
        {
        .row = {.base_info = {.label = "top menu", .type = JRUI_WIDGET_TYPE_ROW}, .children = TOP_MENU_BUTTON_STACKS, .child_count = TOP_MENU_COUNT + 1},
        };
