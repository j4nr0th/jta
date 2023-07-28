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
#include "jfw/jfw_error.h"


#include "gfx/vk_state.h"
#include "gfx/drawing_3d.h"
#include "gfx/camera.h"
#include "gfx/bounding_box.h"
#include "ui.h"
#include "core/jtaelements.h"
#include "core/jtanaturalbcs.h"
#include "core/jtanumericalbcs.h"
#include "config/config_loading.h"


static jfw_result wnd_draw(jfw_window* this)
{
    jta_draw_state* const draw_state = jfw_window_get_usr_ptr(this);
    vk_state* const state = draw_state->vulkan_state;
    bool draw_good = draw_frame(
            state, jfw_window_get_vk_resources(this),
            &draw_state->meshes, &draw_state->camera) == GFX_RESULT_SUCCESS;
    if (draw_good && draw_state->screenshot)
    {
        //  Save the screenshot


        draw_state->screenshot = 0;
    }
    return draw_good ? JFW_RESULT_SUCCESS : JFW_RESULT_ERROR;
}

static jfw_result wnd_dtor(jfw_window* this)
{
    jta_draw_state* const state = jfw_window_get_usr_ptr(this);
    jfw_window_vk_resources* vk_res = jfw_window_get_vk_resources(this);
    vkDeviceWaitIdle(vk_res->device);
    vk_state_destroy(state->vulkan_state, vk_res);
    return JFW_RESULT_SUCCESS;
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
//    ill_jallocator_set_debug_trap(G_JALLOCATOR, 6, jmem_trap, NULL);
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

    jta_timer_set(&main_timer);
    jta_config master_config;
    jta_result jta_res = jta_load_configuration(argv[1], &master_config);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load program configuration, reason: %s", jta_result_to_str(jta_res));
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Config parsing time: %g sec", dt);


    jta_problem_setup problem_setup;
    jta_timer_set(&main_timer);
    jta_res = jta_load_problem(&master_config.problem, &problem_setup);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load problem, reason: %s", jta_result_to_str(jta_res));
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Data loading time: %g sec", dt);
    //  Find the bounding box of the geometry
    vec4 geo_base;
    f32 geo_radius;
    gfx_find_bounding_sphere(&problem_setup.point_list, &geo_base, &geo_radius);




    jfw_ctx* jctx = NULL;
    jfw_window* jwnd = NULL;
    jta_timer_set(&main_timer);
    {
        const jfw_allocator_callbacks allocator_callbacks =
                {
                .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                .realloc = (void* (*)(void*, void*, uint64_t)) ill_jrealloc,
                .free = (void (*)(void*, void*)) ill_jfree,
                .state = G_JALLOCATOR,
                };
        jfw_result jfw_result = jfw_context_create(
                &jctx,
                NULL, &allocator_callbacks);
        if (jfw_result != JFW_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create jfw context, reason: %s", jfw_error_message(jfw_result));
            goto cleanup;
        }
        jfw_result = jfw_window_create(
                jctx, 1600, 900, "JANSYS - jta - 0.0.1", (jfw_color) { .a = 0xFF, .r = 0x80, .g = 0x80, .b = 0x80 },
                &jwnd, 0);
        if (jfw_result != JFW_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create window");
            goto cleanup;
        }
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

    jta_structure_meshes undeformed_meshes = {};
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Vulkan init time: %g sec", dt);

    jta_timer_set(&main_timer);
    gfx_res = mesh_init_truss(&undeformed_meshes.cylinders, 1 << 12, &vulkan_state, vk_res);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }

    gfx_res = mesh_init_sphere(&undeformed_meshes.spheres, 7, &vulkan_state, vk_res);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }

    gfx_res = mesh_init_cone(&undeformed_meshes.cones, 1 << 4, &vulkan_state, vk_res);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create cone mesh: %s", gfx_result_to_str(gfx_res));
    }

    gfx_res = jta_structure_meshes_generate_undeformed(&undeformed_meshes, &master_config.display, &problem_setup, &vulkan_state);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not generate undeformed mesh, reason: %s", gfx_result_to_str(gfx_res));
    }


    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Mesh generation time: %g sec", dt);




    JDM_TRACE("Total of %"PRIu64" triangles in the mesh\n", mesh_polygon_count(&undeformed_meshes.cylinders) + mesh_polygon_count(&undeformed_meshes.spheres) + mesh_polygon_count(&undeformed_meshes.cones));


    jwnd->functions.dtor_fn = wnd_dtor;
    jwnd->functions.draw = wnd_draw;
    jwnd->functions.mouse_button_press = truss_mouse_button_press;
    jwnd->functions.mouse_button_release = truss_mouse_button_release;
    jwnd->functions.mouse_motion = truss_mouse_motion;
    jwnd->functions.button_up = truss_key_press;
    jwnd->functions.mouse_button_double_press = truss_mouse_button_double_press;
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


    jta_solution solution = {};
    jta_draw_state draw_state =
            {
            .vulkan_state = &vulkan_state,
            .camera = camera,
            .original_camera = camera,
            .vulkan_resources = vk_res,
            .p_problem = &problem_setup,
            .p_solution = &solution,
            .config = &master_config,
            .meshes = undeformed_meshes,
            };


    jfw_window_set_usr_ptr(jwnd, &draw_state);
    vulkan_state.view = jta_camera_to_view_matrix(&camera);
    bool close = false;
    while ((JFW_RESULT_SUCCESS == jfw_context_wait_for_events(jctx)) && !close)
    {
        while (jfw_context_has_events(jctx) && !close)
        {
            close = (jfw_context_process_events(jctx) != JFW_RESULT_SUCCESS);
        }
        if (!close)
        {
            jfw_window_redraw(jctx, jwnd);
        }
    }

    jwnd = NULL;


    jta_solution_free(&solution);
cleanup:
    mesh_uninit(&draw_state.meshes.cylinders);
    mesh_uninit(&draw_state.meshes.spheres);
    mesh_uninit(&draw_state.meshes.cones);
    if (jctx)
    {
        jfw_context_destroy(jctx);
        jctx = NULL;
    }
    jta_free_problem(&problem_setup);
    jta_free_configuration(&master_config);
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
