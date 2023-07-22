#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "mem/aligned_jalloc.h"
#include <jmem/jmem.h>
#include <jdm.h>
#include <jio/iocfg.h>



#include "jfw/window.h"
#include "jfw/widget-base.h"
#include "jfw/error_system/error_codes.h"


#include "gfx/vk_state.h"
#include "gfx/drawing_3d.h"
#include "gfx/camera.h"
#include "gfx/bounding_box.h"
#include "ui.h"
#include "core/jtaelements.h"
#include "core/jtanaturalbcs.h"
#include "core/jtanumericalbcs.h"


static jfw_res widget_draw(jfw_widget* this)
{
    jta_draw_state* const draw_state = jfw_widget_get_user_pointer(this);
    vk_state* const state = draw_state->vulkan_state;
    bool draw_good = draw_frame(
            state, jfw_window_get_vk_resources(this->window), state->mesh_count, state->mesh_array,
            &draw_state->camera) == GFX_RESULT_SUCCESS;
    if (draw_good && draw_state->screenshot)
    {
        //  Save the screenshot


        draw_state->screenshot = 0;
    }
    return draw_good ? jfw_res_success : jfw_res_error;
}

static jfw_res widget_dtor(jfw_widget* this)
{
    jfw_window* wnd = this->window;
    void* state = jfw_window_get_usr_ptr(wnd);
    jfw_window_vk_resources* vk_res = jfw_window_get_vk_resources(wnd);
    vkDeviceWaitIdle(vk_res->device);
    vk_state_destroy(state, vk_res);
    return jfw_res_success;
}

static i32 jdm_error_hook_callback_function(const char* thread_name, u32 stack_trace_count, const char*const* stack_trace, jdm_message_level level, u32 line, const char* file, const char* function, const char* message, void* param)
{
    FILE* out;
    const char* level_str = jdm_message_level_str(level);
    if (level < JDM_MESSAGE_LEVEL_WARN)
    {
        out = stdout;
    }
    else
    {
        out = stderr;
    }
    fprintf(out, "%s:%d - %s [%s]: %s\n", file, line, function, level_str, message);
    fprintf(out, "Stacktrace:\n+->%s\n", function);
    for (uint32_t i = 0; stack_trace_count && i < stack_trace_count - 1; ++i)
    {
        fprintf(out, "|-%s\n", stack_trace[stack_trace_count - 1 - i]);
    }
    fprintf(out, "%s[%s]\n", stack_trace[0], thread_name);

    (void)param;

    return 0;
}

static void double_free_hook(jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    (void)param;
    JDM_ERROR("Double free detected by allocator %s (%p)", allocator->type, allocator);
    JDM_LEAVE_FUNCTION;
}

static void invalid_alloc(jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    (void)param;
    JDM_ERROR("Bad allocation detected by allocator %s (%p)", allocator->type, allocator);
    JDM_LEAVE_FUNCTION;
}

static void jmem_trap(uint32_t idx, void* param)
{
    (void)param;
    (void)idx;
    __builtin_trap();
}

static char* get_string_value_from_the_section(const jio_cfg_section* section, const char* const name)
{
    jio_cfg_value v;
    jio_result jio_res = jio_cfg_get_value_by_key(section, name, &v);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not get value of key \"%s\" from section \"%.*s\", reason: %s", name, (int)section->name.len, section->name.begin,
                  jio_result_to_str(jio_res));
        return NULL;
    }
    if (v.type != JIO_CFG_TYPE_STRING)
    {
        JDM_ERROR("Type of value of key \"%s\" of section \"%.*s\" was not \"String\", instead it was \"%s\"", name, (int)section->name.len, section->name.begin, "Unknown");
        return NULL;
    }
    char* str = lin_jalloc(G_LIN_JALLOCATOR, v.value.value_string.len + 1);
    if (!str)
    {
        JDM_ERROR("Could not allocate %zu bytes of memory to copy value of key \"%s\" from section \"%.*s\"", (size_t)v.value.value_string.len + 1, name, (int)section->name.len, section->name.begin);
        return NULL;
    }
    memcpy(str, v.value.value_string.begin, v.value.value_string.len);
    str[v.value.value_string.len] = 0;
    return str;
}

int main(int argc, char* argv[argc])
{
    jta_timer main_timer;
    jta_timer_set(&main_timer);
    //  Create allocators
    //  Estimated to be 512 kB per small pool and 512 kB per med pool
    G_ALIGN_JALLOCATOR = aligned_jallocator_create(1, 1);
    if (!G_ALIGN_JALLOCATOR)
    {
        fputs("Could not create aligned allocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_LIN_JALLOCATOR = lin_jallocator_create(1 << 20);
    if (!G_LIN_JALLOCATOR)
    {
        fputs("Could not create linear allocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_JALLOCATOR = ill_jallocator_create(1 << 20, 1);
    if (!G_JALLOCATOR)
    {
        fputs("Could not create base jallocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_JALLOCATOR->double_free_callback = double_free_hook;
    G_JALLOCATOR->bad_alloc_callback = invalid_alloc;

    {
        jdm_allocator_callbacks jdm_callbacks =
                {
                .param = G_JALLOCATOR,
                .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                .free = (void (*)(void*, void*)) ill_jfree,
                };
        jdm_init_thread(
                "master",
#ifndef NEBUG
                JDM_MESSAGE_LEVEL_NONE,
#else
                JDM_MESSAGE_LEVEL_WARN,
#endif
                64,
                64,
                &jdm_callbacks);
    }
    JDM_ENTER_FUNCTION;
    jdm_set_hook(jdm_error_hook_callback_function, NULL);
    if (argc == 1)
    {
        printf("usage:\n    %s CFG_FILE\n", argv[0]);
        exit(EXIT_SUCCESS);
    }
    f64 dt = jta_timer_get(&main_timer);
    JDM_TRACE("Initialization time: %g sec", dt);

    //  Load up the configuration
    char* pts_file_name;
    char* mat_file_name;
    char* pro_file_name;
    char* elm_file_name;
    char* nat_file_name;
    char* num_file_name;

    jta_timer_set(&main_timer);
    {
        jio_memory_file cfg_file;
        jio_result jio_res = jio_memory_file_create(argv[1], &cfg_file, 0, 0, 0);
        if (jio_res != JIO_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not open configuration file \"%s\", reason: %s", argv[1], jio_result_to_str(jio_res));
        }

        jio_cfg_section* cfg_root;
        {
            jio_allocator_callbacks cfg_callbacks =
                    {
                    .param = G_JALLOCATOR,
                    .free = (void (*)(void*, void*)) ill_jfree,
                    .realloc = (void* (*)(void*, void*, uint64_t)) ill_jrealloc,
                    .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                    };
            jio_res = jio_cfg_parse(&cfg_file, &cfg_root, &cfg_callbacks);
            if (jio_res != JIO_RESULT_SUCCESS)
            {
                JDM_FATAL("Could not parse configuration file \"%s\", reason: %s", argv[1], jio_result_to_str(jio_res));
            }
        }


        //  Parse the config file
        {
            jio_cfg_section* setup_section;
            jio_res = jio_cfg_get_subsection(cfg_root, "problem setup", &setup_section);
            if (jio_res != JIO_RESULT_SUCCESS)
            {
                JDM_FATAL("Could not find the configuration file's \"problem setup\" section, reason: %s",
                          jio_result_to_str(jio_res));
            }


            {
                jio_cfg_section* input_section;
                jio_res = jio_cfg_get_subsection(setup_section, "input files", &input_section);
                if (jio_res != JIO_RESULT_SUCCESS)
                {
                    JDM_FATAL(
                            "Could not find the configuration file's \"problem setup.input files\" section, reason: %s",
                            jio_result_to_str(jio_res));
                }
                //  Get the points file
                pts_file_name = get_string_value_from_the_section(input_section, "points");
                if (!pts_file_name)
                {
                    JDM_FATAL("Could not load points");
                }
                //  Get the material file
                mat_file_name = get_string_value_from_the_section(input_section, "materials");
                if (!mat_file_name)
                {
                    JDM_FATAL("Could not load materials");
                }
                //  Get the profile file
                pro_file_name = get_string_value_from_the_section(input_section, "profiles");
                if (!pro_file_name)
                {
                    JDM_FATAL("Could not load profiles");
                }
                //  Get the element file
                elm_file_name = get_string_value_from_the_section(input_section, "elements");
                if (!elm_file_name)
                {
                    JDM_FATAL("Could not load elements");
                }
                //  Get the natural boundary condition file
                nat_file_name = get_string_value_from_the_section(input_section, "natural_bcs");
                if (!nat_file_name)
                {
                    JDM_FATAL("Could not load natural bcs");
                }
                //  Get the numerical boundary condition file
                num_file_name = get_string_value_from_the_section(input_section, "numerical_bcs");
                if (!num_file_name)
                {
                    JDM_FATAL("Could not load numerical bcs");
                }
            }

            {
                jio_cfg_section* parameter_section;
                jio_res = jio_cfg_get_subsection(setup_section, "parameters", &parameter_section);
                if (jio_res != JIO_RESULT_SUCCESS)
                {
                    JDM_FATAL(
                            "Could not find the configuration file's \"problem setup.parameters\" section, reason: %s",
                            jio_result_to_str(jio_res));
                }
            }

            {
                jio_cfg_section* output_section;
                jio_res = jio_cfg_get_subsection(setup_section, "output files", &output_section);
                if (jio_res != JIO_RESULT_SUCCESS)
                {
                    JDM_FATAL(
                            "Could not find the configuration file's \"problem setup.output files\" section, reason: %s",
                            jio_result_to_str(jio_res));
                }
            }
        }

        jio_cfg_section_destroy(cfg_root);
        jio_memory_file_destroy(&cfg_file);
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Config parsing time: %g sec", dt);

    jio_memory_file file_points, file_materials, file_profiles, file_elements, file_nat, file_num;
    jta_point_list point_list;
    jta_material_list material_list;
    jta_profile_list profile_list;
    jta_element_list elements;
    jta_natural_boundary_condition_list natural_boundary_conditions;
    jta_numerical_boundary_condition_list numerical_boundary_conditions;

    jta_timer_set(&main_timer);
    jio_result jio_res = jio_memory_file_create(pts_file_name, &file_points, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open point file \"%s\"", pts_file_name);
    }
    jio_res = jio_memory_file_create(mat_file_name, &file_materials, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open material file \"%s\"", mat_file_name);
    }
    jio_res = jio_memory_file_create(pro_file_name, &file_profiles, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open profile file \"%s\"", pro_file_name);
    }
    jio_res = jio_memory_file_create(elm_file_name, &file_elements, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open element file \"%s\"", elm_file_name);
    }
    jio_res = jio_memory_file_create(nat_file_name, &file_nat, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open element file \"%s\"", nat_file_name);
    }
    jio_res = jio_memory_file_create(num_file_name, &file_num, 0, 0, 0);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not open element file \"%s\"", num_file_name);
    }
    lin_jfree(G_LIN_JALLOCATOR, num_file_name);
    lin_jfree(G_LIN_JALLOCATOR, nat_file_name);
    lin_jfree(G_LIN_JALLOCATOR, elm_file_name);
    lin_jfree(G_LIN_JALLOCATOR, pro_file_name);
    lin_jfree(G_LIN_JALLOCATOR, mat_file_name);
    lin_jfree(G_LIN_JALLOCATOR, pts_file_name);

    jta_result jta_res = jta_load_points(&file_points, &point_list);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load points");
    }
    if (point_list.count < 2)
    {
        JDM_FATAL("At least two points should be defined");
    }
    jta_res = jta_load_materials(&file_materials, &material_list);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load materials");
    }
    if (material_list.count < 1)
    {
        JDM_FATAL("At least one material should be defined");
    }
    jta_res = jta_load_profiles(&file_profiles, &profile_list);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load profiles");
    }
    if (profile_list.count < 1)
    {
        JDM_FATAL("At least one profile should be defined");
    }

    jta_res = jta_load_elements(&file_elements, &point_list, &material_list, &profile_list, &elements);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load elements");
    }
    if (elements.count < 1)
    {
        JDM_FATAL("At least one profile should be defined");
    }

    jta_res = jta_load_natural_boundary_conditions(&file_nat, &point_list, &natural_boundary_conditions);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load natural boundary conditions");
    }

    jta_res = jta_load_numerical_boundary_conditions(&file_num, &point_list, &numerical_boundary_conditions);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load numerical boundary conditions");
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Data loading time: %g sec", dt);
    //  Find the bounding box of the geometry
    vec4 geo_base;
    f32 geo_radius;
    gfx_find_bounding_sphere(&point_list, &geo_base, &geo_radius);




    jfw_ctx* jctx = NULL;
    jfw_window* jwnd = NULL;
    jta_timer_set(&main_timer);

    jfw_res jfw_result = jfw_context_create(&jctx,
                                            NULL
                                           );
    if (!jfw_success(jfw_result))
    {
        JDM_ERROR("Could not create jfw context, reason: %s", jfw_error_message(jfw_result));
        goto cleanup;
    }
    jfw_result = jfw_window_create(
            jctx, 1600, 900, "JANSYS - jta - 0.0.1", (jfw_color) { .a = 0xFF, .r = 0x80, .g = 0x80, .b = 0x80 },
            &jwnd, 0);
    if (!jfw_success(jfw_result))
    {
        JDM_ERROR("Could not create window");
        goto cleanup;
    }
    jfw_window_show(jctx, jwnd);
    jfw_window_vk_resources* const vk_res = jfw_window_get_vk_resources(jwnd);
    jfw_vulkan_context* const vk_ctx = &jctx->vk_ctx;
    vk_state vulkan_state;
    gfx_result gfx_res = vk_state_create(&vulkan_state, vk_res);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create vulkan state");
        goto cleanup;
    }
    jta_mesh truss_mesh;
    jta_mesh sphere_mesh;
    jta_mesh cone_mesh;
    vulkan_state.point_list = &point_list;
    if ((gfx_res = mesh_init_truss(&truss_mesh, 1 << 12, &vulkan_state, vk_res)) != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }
    if ((gfx_res = mesh_init_sphere(&sphere_mesh, 7, &vulkan_state, vk_res)) != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }
    if ((gfx_res = mesh_init_cone(&cone_mesh, 1 << 4, &vulkan_state, vk_res)) != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create cone mesh: %s", gfx_result_to_str(gfx_res));
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Vulkan init time: %g sec", dt);


    jta_timer_set(&main_timer);
    //  This is the truss mesh :)
    f32 radius_factor = 1.0f;  //  This could be a config option
    for (u32 i = 0; i < elements.count; ++i)
    {
        if ((gfx_res = truss_mesh_add_between_pts(
                &truss_mesh, (jfw_color) { .r = 0xD0, .g = 0xD0, .b = 0xD0, .a = 0xFF },
                radius_factor * profile_list.equivalent_radius[elements.i_profile[i]],
                VEC4(point_list.p_x[elements.i_point0[i]], point_list.p_y[elements.i_point0[i]],
                     point_list.p_z[elements.i_point0[i]]), VEC4(point_list.p_x[elements.i_point1[i]],
                                                             point_list.p_y[elements.i_point1[i]],
                                                             point_list.p_z[elements.i_point1[i]]), 0.0f, &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add element %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }
    //  These are the joints
    uint32_t* bcs_per_point = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*bcs_per_point) * point_list.count);
    memset(bcs_per_point, 0, sizeof(*bcs_per_point) * point_list.count);
    for (u32 i = 0; i < numerical_boundary_conditions.count; ++i)
    {
        bcs_per_point[numerical_boundary_conditions.i_point[i]] +=
                ((numerical_boundary_conditions.type[i] & JTA_NUMERICAL_BC_TYPE_X) != 0) +
                ((numerical_boundary_conditions.type[i] & JTA_NUMERICAL_BC_TYPE_Y) != 0) +
                ((numerical_boundary_conditions.type[i] & JTA_NUMERICAL_BC_TYPE_Z) != 0);
    }
    //  These could be a config options
    const jfw_color point_colors[4] =
            {
                    {.r = 0x80, .g = 0x80, .b = 0x80, .a = 0xFF},   //  0 - just gray
                    {.r = 0xFF, .g = 0xFF, .b = 0x00, .a = 0xFF},   //  1 - yellow
                    {.r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0xFF},   //  2 - purple
                    {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF},   //  3 (or somehow more) - red
            };
    const f32 point_scales[4] =
            {
                1.0f,//  0 - just gray
                2.0f,//  1 - yellow
                2.0f,//  2 - purple
                2.0f,//  3 (or somehow more) - red
            };

    for (u32 i = 0; i < point_list.count; ++i)
    {
        jfw_color c;
        f32 r;
        switch (bcs_per_point[i])
        {
        case 0:
            c = point_colors[0];
            r = point_scales[0];
            break;
        case 1:
            c = point_colors[1];
            r = point_scales[1];
            break;
        case 2:
            c = point_colors[2];
            r = point_scales[2];
            break;
        default:
            JDM_WARN("Point \"%.*s\" has %u numerical boundary conditions applied to it", (int)point_list.label[i].len, point_list.label[i].begin, bcs_per_point[i]);
            [[fallthrough]];
        case 3:
            c = point_colors[3];
            r = point_scales[3];
            break;
        }

        if ((gfx_res = sphere_mesh_add(&sphere_mesh, c, r * point_list.max_radius[i], VEC4(point_list.p_x[i], point_list.p_y[i], point_list.p_z[i]), &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add node %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }
    lin_jfree(G_LIN_JALLOCATOR, bcs_per_point);
    //  These are the force vectors
    const f32 max_radius_scale = 0.3f;    //  This could be a config option
    const f32 arrow_cone_ratio = 0.5f;    //  This could be a config option
    const f32 max_length_scale = 0.3f;    //  This could be a config option
    for (u32 i = 0; i < natural_boundary_conditions.count; ++i)
    {
        vec4 base = VEC4(point_list.p_x[natural_boundary_conditions.i_point[i]],
                         point_list.p_y[natural_boundary_conditions.i_point[i]],
                         point_list.p_z[natural_boundary_conditions.i_point[i]]);
        float mag = hypotf(
                hypotf(natural_boundary_conditions.x[i], natural_boundary_conditions.y[i]),
                natural_boundary_conditions.z[i]);
        const f32 real_length = mag / natural_boundary_conditions.max_mag * elements.max_len * max_length_scale;
        vec4 direction = vec4_div_one(VEC4(natural_boundary_conditions.x[i], natural_boundary_conditions.y[i], natural_boundary_conditions.z[i]), mag);
        vec4 second = vec4_add(base, vec4_mul_one(direction, real_length * (1 - arrow_cone_ratio)));
        vec4 third = vec4_add(base, vec4_mul_one(direction, real_length));
        if ((gfx_res = cone_mesh_add_between_pts(&cone_mesh, (jfw_color){.b = 0xD0, .a = 0xFF}, 2 * max_radius_scale * profile_list.max_equivalent_radius,
                                                 second, third, &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add force %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
        if ((gfx_res = truss_mesh_add_between_pts(&truss_mesh, (jfw_color){.b = 0xD0, .a = 0xFF}, max_radius_scale * profile_list.max_equivalent_radius,
                                                 base, second, 0.0f, &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add force %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }

    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Mesh generation time: %g sec", dt);
    jta_mesh* meshes[] = {
            &truss_mesh,
            &sphere_mesh,
            &cone_mesh
    };

    vulkan_state.mesh_count = 3;
    vulkan_state.mesh_array = meshes + 0;


    printf("Total of %"PRIu64" triangles in the mesh\n", mesh_polygon_count(&truss_mesh) + mesh_polygon_count(&sphere_mesh) + mesh_polygon_count(&cone_mesh));

    jfw_window_set_usr_ptr(jwnd, &vulkan_state);
    jfw_widget* jwidget;
    jfw_result = jfw_widget_create_as_base(jwnd, 1600, 900, 0, 0, &jwidget);
    if (!jfw_success(jfw_result))
    {
        JDM_ERROR("Could not create window's base widget");
        goto cleanup;
    }
    jwidget->dtor_fn = widget_dtor;
    jwidget->draw_fn = widget_draw;
    jwidget->functions.mouse_button_press = truss_mouse_button_press;
    jwidget->functions.mouse_button_release = truss_mouse_button_release;
    jwidget->functions.mouse_motion = truss_mouse_motion;
    jwidget->functions.button_up = truss_key_press;
    jwidget->functions.mouse_button_double_press = truss_mouse_button_double_press;
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



    jta_draw_state draw_state =
            {
            .vulkan_state = &vulkan_state,
            .camera = camera,
            .original_camera = camera,
            .vulkan_resources = vk_res,
            };

    draw_state.problem.numerical_bcs = &numerical_boundary_conditions;
    draw_state.problem.natural_bcs = &natural_boundary_conditions;
    draw_state.problem.point_list = &point_list;
    draw_state.problem.element_list = &elements;
    draw_state.problem.materials = &material_list;
    draw_state.problem.profile_list = &profile_list;
    draw_state.problem.deformations = ill_jalloc(G_JALLOCATOR, sizeof(*draw_state.problem.deformations) * 3 * point_list.count);
    draw_state.problem.forces = ill_jalloc(G_JALLOCATOR, sizeof(*draw_state.problem.forces) * 3 * point_list.count);
    draw_state.problem.point_masses = ill_jalloc(G_JALLOCATOR, sizeof(*draw_state.problem.point_masses) * point_list.count);
    draw_state.problem.gravity = VEC4(0, 0, -9.81);

    if (!draw_state.problem.deformations)
    {
        JDM_FATAL("Could not allocate memory for deformation results");
    }
    if (!draw_state.problem.forces)
    {
        JDM_FATAL("Could not allocate memory for force values");
    }
    if (!draw_state.problem.point_masses)
    {
        JDM_FATAL("Could not allocate memory for point masses");
    }
    memset(draw_state.problem.deformations, 0, sizeof(*draw_state.problem.deformations) * point_list.count);
    memset(draw_state.problem.forces, 0, sizeof(*draw_state.problem.forces) * point_list.count);
    memset(draw_state.problem.point_masses, 0, sizeof(*draw_state.problem.point_masses) * point_list.count);

    jmtx_result jmtx_res = jmtx_matrix_crs_new(&draw_state.problem.stiffness_matrix, 3 * point_list.count, 3 * point_list.count, 36 * point_list.count, NULL);
    if (jmtx_res != JMTX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not allocate memory for problem stiffness matrix, reason: %s", jmtx_result_to_str(jmtx_res));
    }
    jfw_widget_set_user_pointer(jwidget, &draw_state);
    vulkan_state.view = jta_camera_to_view_matrix(&camera);

    i32 close = 0;
    while (jfw_success(jfw_context_wait_for_events(jctx)) && !close)
    {
        while (jfw_context_has_events(jctx) && !close)
        {
            close = !jfw_success(jfw_context_process_events(jctx));
        }
        if (!close)
        {
            jfw_window_redraw(jctx, jwnd);
        }
    }
//    vk_state_destroy(&vulkan_state, vk_res);
    jwnd = NULL;

    ill_jfree(G_JALLOCATOR, draw_state.problem.point_masses);
    ill_jfree(G_JALLOCATOR, draw_state.problem.forces);
    ill_jfree(G_JALLOCATOR, draw_state.problem.deformations);
    jmtx_matrix_crs_destroy(draw_state.problem.stiffness_matrix);

cleanup:
    mesh_uninit(&truss_mesh);
    mesh_uninit(&sphere_mesh);
    mesh_uninit(&cone_mesh);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.type);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.i_point);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.x);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.y);
    ill_jfree(G_JALLOCATOR, numerical_boundary_conditions.z);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.i_point);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.x);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.y);
    ill_jfree(G_JALLOCATOR, natural_boundary_conditions.z);
    ill_jfree(G_JALLOCATOR, elements.lengths);
    ill_jfree(G_JALLOCATOR, elements.labels);
    ill_jfree(G_JALLOCATOR, elements.i_material);
    ill_jfree(G_JALLOCATOR, elements.i_profile);
    ill_jfree(G_JALLOCATOR, elements.i_point0);
    ill_jfree(G_JALLOCATOR, elements.i_point1);
    ill_jfree(G_JALLOCATOR, profile_list.equivalent_radius);
    ill_jfree(G_JALLOCATOR, profile_list.area);
    ill_jfree(G_JALLOCATOR, profile_list.second_moment_of_area);
    ill_jfree(G_JALLOCATOR, profile_list.labels);
    ill_jfree(G_JALLOCATOR, material_list.labels);
    ill_jfree(G_JALLOCATOR, material_list.compressive_strength);
    ill_jfree(G_JALLOCATOR, material_list.tensile_strength);
    ill_jfree(G_JALLOCATOR, material_list.density);
    ill_jfree(G_JALLOCATOR, material_list.elastic_modulus);
    ill_jfree(G_JALLOCATOR, point_list.p_x);
    ill_jfree(G_JALLOCATOR, point_list.p_y);
    ill_jfree(G_JALLOCATOR, point_list.p_z);
    ill_jfree(G_JALLOCATOR, point_list.label);
    ill_jfree(G_JALLOCATOR, point_list.max_radius);
    jio_memory_file_destroy(&file_elements);
    jio_memory_file_destroy(&file_profiles);
    jio_memory_file_destroy(&file_materials);
    jio_memory_file_destroy(&file_points);
    if (jctx)
    {
        jfw_context_destroy(jctx);
        jctx = NULL;
    }
    JDM_LEAVE_FUNCTION;
    jdm_cleanup_thread();
    //  Clean up the allocators
    {
        jallocator* const allocator = G_JALLOCATOR;
        G_JALLOCATOR = NULL;
        uint64_t total_allocations, max_usage, total_allocated, biggest_allocation;
        ill_jallocator_statistics(allocator, &biggest_allocation, &total_allocated, &max_usage, &total_allocations);
        printf("G_JALLOCATOR statistics:\n\tBiggest allocation: %zu b (%g kB)\n\tTotal allocated memory: %g kB (%g MB)"
               "\n\tMax usage: %g kB (%g MB)\n\tTotal allocations: %zu\n", (size_t)biggest_allocation, (double)biggest_allocation / 1024.0, (double)total_allocated / 1024.0, (double)total_allocated / 1024.0 / 1024.0, (double)max_usage / 1024.0, (double )max_usage / 1024.0 / 1024.0, (size_t)total_allocations);
#ifndef NDEBUG
        uint_fast32_t leaked_block_indices[256];
        uint_fast32_t leaked_block_count = ill_jallocator_count_used_blocks(allocator, sizeof(leaked_block_indices) / sizeof(*leaked_block_indices), leaked_block_indices);
        for (u32 i = 0; i < leaked_block_count; ++i)
        {
            fprintf(stderr, "G_JALLOCATOR did not free block %"PRIuFAST32"\n", leaked_block_indices[i]);
        }
#endif
        ill_jallocator_destroy(allocator);
    }
    {
        jallocator* const allocator = G_LIN_JALLOCATOR;
        G_LIN_JALLOCATOR = NULL;
        lin_jallocator_destroy(allocator);
    }
    {
        printf("Total lifetime waste by aligned allocator: %zu\n", aligned_jallocator_lifetime_waste(G_ALIGN_JALLOCATOR));
        aligned_jallocator* const allocator = G_ALIGN_JALLOCATOR;
        G_ALIGN_JALLOCATOR = NULL;
        aligned_jallocator_destroy(allocator);
    }
    return 0;
}
