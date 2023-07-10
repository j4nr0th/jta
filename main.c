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
#include "solver/jtaelements.h"
#include "solver/jtanaturalbcs.h"


static jfw_res widget_draw(jfw_widget* this)
{
    jta_draw_state* const draw_state = jfw_widget_get_user_pointer(this);
    vk_state* const state = draw_state->vulkan_state;
    return draw_frame(
            state, jfw_window_get_vk_resources(this->window), state->mesh_count, state->mesh_array, &draw_state->camera) == GFX_RESULT_SUCCESS ? jfw_res_success : jfw_res_error;
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

static void* VKAPI_PTR vk_aligned_allocation_fn(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
    void* ptr = aligned_jalloc((aligned_jallocator*) pUserData, alignment, size);
    if (alignment > 8)
    JDM_INFO("Called aligned_jalloc(%p, %zu, %zu) = %p\n", pUserData, alignment, size, ptr);
    return ptr;
}

static void* VKAPI_PTR vk_aligned_reallocation_fn(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
    void* ptr = aligned_jrealloc((aligned_jallocator*) pUserData, pOriginal, alignment, size);
    if (alignment > 8)
    JDM_INFO("Called aligned_rejalloc(%p, %p, %zu, %zu) = %p\n", pUserData, pOriginal, alignment, size, ptr);
    return ptr;
}

static void VKAPI_PTR vk_aligned_free_fn(void* pUserData, void* pMemory)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
//    JDM_INFO("Called aligned_jfree(%p, %p)", pUserData, pMemory);
    aligned_jfree(pUserData, pMemory);
}

static const char* vk_internal_allocation_type_to_str(VkInternalAllocationType allocation_type)
{
    switch (allocation_type)
    {
    case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE: return "VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE";
    default: return "UNKNOWN";
    }
}

static const char* vk_system_allocation_scope_to_str(VkSystemAllocationScope allocation_scope)
{
    switch (allocation_scope)
    {
    case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND: return "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND";
    case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT: return "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT";
    case VK_SYSTEM_ALLOCATION_SCOPE_CACHE: return "VK_SYSTEM_ALLOCATION_SCOPE_CACHE";
    case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE: return "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE";
    case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE: return "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE";
    default:return "UNKNOWN";
    }
}

static void VKAPI_PTR vk_internal_alloc_notif(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
    printf("Vulkan allocating %zu bytes of memory for type %s and scope %s", size, vk_internal_allocation_type_to_str(allocationType),
           vk_system_allocation_scope_to_str(allocationScope));
}

static void VKAPI_PTR vk_internal_free_notif(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
    printf("Vulkan freeing %zu bytes of memory for type %s and scope %s", size, vk_internal_allocation_type_to_str(allocationType),
           vk_system_allocation_scope_to_str(allocationScope));
}


static i32 jfw_error_hook_callback_function(const char* thread_name, u32 stack_trace_count, const char*const* stack_trace, jdm_message_level level, u32 line, const char* file, const char* function, const char* message, void* param)
{
    switch (level)
    {
    case JDM_MESSAGE_LEVEL_INFO:fprintf(stdout, "JDM_INFO (thread %s - %s): %s\n", thread_name, function, message);
        break;
    case JDM_MESSAGE_LEVEL_WARN:
        fprintf(stderr, "Warning issued from thread \"%s\", stack trace (%s", thread_name, stack_trace[0]);
        for (u32 i = 1; i < stack_trace_count; ++i)
        {
            fprintf(stderr, "-> %s", stack_trace[i]);
        }
        fprintf(stderr, ") in file \"%s\" on line \"%i\" (function \"%s\"): %s\n\n", file, line, function, message);
        break;

    case JDM_MESSAGE_LEVEL_ERR:
    default:
        fprintf(stderr, "Error occurred in thread \"%s\", stack trace (%s", thread_name, stack_trace[0]);
        for (u32 i = 1; i < stack_trace_count; ++i)
        {
            fprintf(stderr, "-> %s", stack_trace[i]);
        }
        fprintf(stderr, ") in file \"%s\" on line \"%i\" (function \"%s\"): %s\n\n", file, line, function, message);
        break;
    }
    (void)param;

    return 0;
}

static void double_free_hook(jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    JDM_ERROR("Double free detected by allocator %s (%p)", allocator->type, allocator);
    JDM_LEAVE_FUNCTION;
}

static void invalid_alloc(jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    JDM_ERROR("Bad allocation detected by allocator %s (%p)", allocator->type, allocator);
    JDM_LEAVE_FUNCTION;
}

static void jmem_trap(uint32_t idx, void* param)
{
    __builtin_trap();
}

int main(int argc, char* argv[argc])
{

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
    jdm_set_hook(jfw_error_hook_callback_function, NULL);

    if (argc == 1)
    {
        printf("usage:\n    %s CFG_FILE\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    //  Load up the configuration
    char* pts_file_name;
    char* mat_file_name;
    char* pro_file_name;
    char* elm_file_name;
    char* nat_file_name;

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
                {
                    jio_cfg_value v;
                    jio_res = jio_cfg_get_value_by_key(input_section, "points", &v);
                    if (jio_res != JIO_RESULT_SUCCESS)
                    {
                        JDM_FATAL("Could not get the element of section \"problem setup.input files\" with key \"points\", reason: %s", jio_result_to_str(jio_res));
                    }
                    if (v.type != JIO_CFG_TYPE_STRING)
                    {
                        JDM_FATAL("Element \"points\" of section \"problem setup.input files\" was not a string");
                    }
                    pts_file_name = lin_jalloc(G_LIN_JALLOCATOR ,v.value_string.len + 1);
                    if (!pts_file_name)
                    {
                        JDM_FATAL("Could not allocate %zu bytes for point file string", (size_t)v.value_string.len + 1);
                    }
                    memcpy(pts_file_name, v.value_string.begin, v.value_string.len);
                    pts_file_name[v.value_string.len] = 0;
                }
                //  Get the material file
                {
                    jio_cfg_value v;
                    jio_res = jio_cfg_get_value_by_key(input_section, "materials", &v);
                    if (jio_res != JIO_RESULT_SUCCESS)
                    {
                        JDM_FATAL("Could not get the element of section \"problem setup.input files\" with key \"materials\", reason: %s", jio_result_to_str(jio_res));
                    }
                    if (v.type != JIO_CFG_TYPE_STRING)
                    {
                        JDM_FATAL("Element \"materials\" of section \"problem setup.input files\" was not a string");
                    }
                    mat_file_name = lin_jalloc(G_LIN_JALLOCATOR ,v.value_string.len + 1);
                    if (!mat_file_name)
                    {
                        JDM_FATAL("Could not allocate %zu bytes for material file string", (size_t)v.value_string.len + 1);
                    }
                    memcpy(mat_file_name, v.value_string.begin, v.value_string.len);
                    mat_file_name[v.value_string.len] = 0;
                }
                //  Get the profile file
                {
                    jio_cfg_value v;
                    jio_res = jio_cfg_get_value_by_key(input_section, "profiles", &v);
                    if (jio_res != JIO_RESULT_SUCCESS)
                    {
                        JDM_FATAL("Could not get the element of section \"problem setup.input files\" with key \"profiles\", reason: %s", jio_result_to_str(jio_res));
                    }
                    if (v.type != JIO_CFG_TYPE_STRING)
                    {
                        JDM_FATAL("Element \"profiles\" of section \"problem setup.input files\" was not a string");
                    }
                    pro_file_name = lin_jalloc(G_LIN_JALLOCATOR ,v.value_string.len + 1);
                    if (!pro_file_name)
                    {
                        JDM_FATAL("Could not allocate %zu bytes for profile file string", (size_t)v.value_string.len + 1);
                    }
                    memcpy(pro_file_name, v.value_string.begin, v.value_string.len);
                    pro_file_name[v.value_string.len] = 0;
                }
                //  Get the element file
                {
                    jio_cfg_value v;
                    jio_res = jio_cfg_get_value_by_key(input_section, "elements", &v);
                    if (jio_res != JIO_RESULT_SUCCESS)
                    {
                        JDM_FATAL("Could not get the element of section \"problem setup.input files\" with key \"elements\", reason: %s", jio_result_to_str(jio_res));
                    }
                    if (v.type != JIO_CFG_TYPE_STRING)
                    {
                        JDM_FATAL("Element \"elements\" of section \"problem setup.input files\" was not a string");
                    }
                    elm_file_name = lin_jalloc(G_LIN_JALLOCATOR ,v.value_string.len + 1);
                    if (!elm_file_name)
                    {
                        JDM_FATAL("Could not allocate %zu bytes for element file string", (size_t)v.value_string.len + 1);
                    }
                    memcpy(elm_file_name, v.value_string.begin, v.value_string.len);
                    elm_file_name[v.value_string.len] = 0;
                }
                //  Get the natural boundary condition file
                {
                    jio_cfg_value v;
                    jio_res = jio_cfg_get_value_by_key(input_section, "natural_bcs", &v);
                    if (jio_res != JIO_RESULT_SUCCESS)
                    {
                        JDM_FATAL("Could not get the element of section \"problem setup.input files\" with key \"natural_bcs\", reason: %s", jio_result_to_str(jio_res));
                    }
                    if (v.type != JIO_CFG_TYPE_STRING)
                    {
                        JDM_FATAL("Element \"natural_bcs\" of section \"problem setup.input files\" was not a string");
                    }
                    nat_file_name = lin_jalloc(G_LIN_JALLOCATOR ,v.value_string.len + 1);
                    if (!nat_file_name)
                    {
                        JDM_FATAL("Could not allocate %zu bytes for natural bc file string", (size_t)v.value_string.len + 1);
                    }
                    memcpy(nat_file_name, v.value_string.begin, v.value_string.len);
                    nat_file_name[v.value_string.len] = 0;
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

//    ill_jallocator_set_debug_trap(G_JALLOCATOR, 35, jmem_trap, NULL);

    u32 n_materials;
    jio_memory_file file_points, file_materials, file_profiles, file_elements, file_nat;
    jta_point_list point_list;
    jta_material* materials;
    jta_profile_list profile_list;
    jta_element_list elements;
    jta_natural_boundary_condition_list natural_boundary_conditions;

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
    jta_res = jta_load_materials(&file_materials, &n_materials, &materials);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load materials");
    }
    if (n_materials < 1)
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

    jta_res = jta_load_elements(&file_elements, &point_list, n_materials, materials, &profile_list, &elements);
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

    //  Find the bounding box of the geometry
    vec4 geo_base;
    f32 geo_radius;
    gfx_find_bounding_sphere(&point_list, &geo_base, &geo_radius);



//#ifdef NDEBUG
//    VkAllocationCallbacks VK_ALLOCATION_CALLBACKS =
//            {
//            .pUserData = G_ALIGN_JALLOCATOR,
//            .pfnAllocation = vk_aligned_allocation_fn,
//            .pfnReallocation = vk_aligned_reallocation_fn,
//            .pfnFree = vk_aligned_free_fn,
//            .pfnInternalAllocation = vk_internal_alloc_notif,
//            .pfnInternalFree = vk_internal_free_notif,
//            };
//#endif

    jfw_ctx* jctx = NULL;
    jfw_window* jwnd = NULL;
    jfw_res jfw_result = jfw_context_create(&jctx,
//#ifndef NDEBUG
                                            NULL
//#else
//                                            &VK_ALLOCATION_CALLBACKS
//#endif
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
    VkResult vk_result;
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
    for (u32 i = 0; i < point_list.count; ++i)
    {
        if ((gfx_res = sphere_mesh_add(&sphere_mesh, (jfw_color){.r = 0x80, .g = 0x80, .b = 0x80, .a = 0xFF}, point_list.max_radius[i], VEC4(point_list.p_x[i], point_list.p_y[i], point_list.p_z[i]), &vulkan_state)) != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not add node %"PRIu32" to the mesh, reason: %s", i, gfx_result_to_str(gfx_res));
            goto cleanup;
        }
    }
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
    jta_camera_3d camera;
    jta_camera_set(
            &camera,                                    //  Camera
            geo_base,                                   //  View target
            geo_base,                                   //  Geometry center
            geo_radius,                                 //  Geometry radius
            VEC4(0, 0, -1),                             //  Down
            vec4_sub(geo_base, vec4_mul_one(VEC4(0, -1, 0), geo_radius)), //  Camera position
            4.0f,                                       //  Turn sensitivity
            1.0f                                        //  Move sensitivity
            );
#ifndef NDEBUG
    f32 min = +INFINITY, max = -INFINITY;
    for (u32 i = 0; i < point_list.count; ++i)
    {
        vec4 p = VEC4(point_list.p_x[i], point_list.p_y[i], point_list.p_z[i]);
        p = vec4_sub(p, camera.position);
        f32 d = vec4_dot(p, camera.uz);
        if (d < min)
        {
            min = d;
        }
        if (d > max)
        {
            max = d;
        }
    }
    f32 n, f;
    jta_camera_find_depth_planes(&camera, &n, &f);
//    assert(min >= n); assert(max <= f);
#endif
    jta_draw_state draw_state =
            {
            .vulkan_state = &vulkan_state,
            .camera = camera,
            .vulkan_resources = vk_res,
            };
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
            jfw_window_force_redraw(jctx, jwnd);
        }
    }
//    vk_state_destroy(&vulkan_state, vk_res);
    jwnd = NULL;

cleanup:
    mesh_uninit(&truss_mesh);
    mesh_uninit(&sphere_mesh);
    mesh_uninit(&cone_mesh);
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
    ill_jfree(G_JALLOCATOR, materials);
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
