#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "mem/aligned_jalloc.h"
#include <jmem/jmem.h>
#include <jdm.h>
#include <jio/iocfg.h>


#include "jwin/source/jwin.h"


#include "gfx/drawing_3d.h"
#include "gfx/camera.h"
#include "gfx/bounding_box.h"
#include "jwin_handlers.h"
#include "core/jtaelements.h"
#include "core/jtanaturalbcs.h"
#include "core/jtanumericalbcs.h"
#include "config/config_loading.h"



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

static void double_free_hook(ill_jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    (void)param;
    JDM_ERROR("Double free detected by allocator (%p)", allocator);
    JDM_LEAVE_FUNCTION;
}

static void invalid_alloc(ill_jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    (void)param;
    JDM_ERROR("Bad allocation detected by allocator (%p)", allocator);
    JDM_LEAVE_FUNCTION;
}

//static void jmem_trap(uint32_t idx, void* param)
//{
//    (void)param;
//    (void)idx;
//    __builtin_trap();
//}

static void jwin_err_report_fn(const char* msg, const char* file, int line, const char* function, void* state)
{
    (void) state;
    JDM_ERROR("JWIN error (%s:%d - %s): %s", file, line, function, msg);
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
    ill_jallocator_set_bad_alloc_callback(G_JALLOCATOR, invalid_alloc, NULL);
    ill_jallocator_set_double_free_callback(G_JALLOCATOR, double_free_hook, NULL);
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




    jwin_context* jctx = NULL;
    jwin_window* jwnd = NULL;
    jta_vulkan_window_context* wnd_ctx = NULL;
    jta_vulkan_context* vk_ctx = NULL;
    jta_timer_set(&main_timer);
    gfx_result gfx_res = jta_vulkan_context_create(NULL, "jta", 1, &vk_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not initialize Vulkan, reason: %s", gfx_result_to_str(gfx_res));
    }
    jwin_result jwin_res;
    {
        const jwin_allocator_callbacks allocator_callbacks =
                {
                .alloc = (void* (*)(void*, uint64_t)) ill_jalloc,
                .realloc = (void* (*)(void*, void*, uint64_t)) ill_jrealloc,
                .free = (void (*)(void*, void*)) ill_jfree,
                .state = G_JALLOCATOR,
                };
        const jwin_error_callbacks error_callbacks =
                {
                .report = jwin_err_report_fn,
                .state = NULL,
                };
        const jwin_context_create_info ctx_info =
                {
                .allocator_callbacks = &allocator_callbacks,
                .error_callbacks = &error_callbacks,
                };
        jwin_res = jwin_context_create(&ctx_info, &jctx);
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not create jwin context, reason: %s", jwin_result_msg_str(jwin_res));
        }
        const jwin_window_create_info win_info =
                {
                .fixed_size = 1,
                .width = 1600,
                .height = 900,
                .title = "JANSYS - jta - 0.0.1",
                };
        jwin_res = jwin_window_create(jctx, &win_info, &jwnd);
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not create window, reason: %s", jwin_result_msg_str(jwin_res));
        }
    }
    jwin_window_show(jwnd);
    gfx_res = jta_vulkan_window_context_create(jwnd, vk_ctx, &wnd_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create window-dependant Vulkan resources, reason: %s", gfx_result_to_str(gfx_res));
    }

    jta_structure_meshes undeformed_meshes = {};
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Vulkan init time: %g sec", dt);

    jta_timer_set(&main_timer);
    gfx_res = mesh_init_truss(&undeformed_meshes.cylinders, 1 << 12, wnd_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }

    gfx_res = mesh_init_sphere(&undeformed_meshes.spheres, 7, wnd_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
    }

    gfx_res = mesh_init_cone(&undeformed_meshes.cones, 1 << 4, wnd_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create cone mesh: %s", gfx_result_to_str(gfx_res));
    }

    gfx_res = jta_structure_meshes_generate_undeformed(&undeformed_meshes, &master_config.display, &problem_setup, wnd_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not generate undeformed mesh, reason: %s", gfx_result_to_str(gfx_res));
    }


    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Mesh generation time: %g sec", dt);




    JDM_TRACE("Total of %"PRIu64" triangles in the mesh\n", mesh_polygon_count(&undeformed_meshes.cylinders) + mesh_polygon_count(&undeformed_meshes.spheres) + mesh_polygon_count(&undeformed_meshes.cones));
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
                    .vk_ctx = vk_ctx,
                    .wnd_ctx = wnd_ctx,
                    .camera = camera,
                    .original_camera = camera,
                    .p_problem = &problem_setup,
                    .p_solution = &solution,
                    .config = &master_config,
                    .meshes = undeformed_meshes,
                    .needs_redraw = 1,
            };
    for (unsigned i = 0; i < JTA_HANDLER_COUNT; ++i)
    {
        jwin_res = jwin_window_set_event_handler(jwnd, JTA_HANDLER_ARRAY[i].type, JTA_HANDLER_ARRAY[i].callback, &draw_state);
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("Failed setting the event handler %u, reason: %s (%s)", i,
                      jwin_result_to_str(jwin_res), jwin_result_msg_str(jwin_res));
        }
    }

//    jwnd->functions.dtor_fn = wnd_dtor;
//    jwnd->functions.draw = wnd_draw;
//    jwnd->functions.mouse_button_press = truss_mouse_button_press;
//    jwnd->functions.mouse_button_release = truss_mouse_button_release;
//    jwnd->functions.mouse_motion = truss_mouse_motion;
//    jwnd->functions.button_up = truss_key_press;
//    jwnd->functions.mouse_button_double_press = truss_mouse_button_double_press;



    draw_state.view_matrix = jta_camera_to_view_matrix(&camera);
//    while ((jwin_res = jwin_context_wait_for_events(jctx)) == JWIN_RESULT_SUCCESS)
    for (;;)
    {
        jwin_res = jwin_context_handle_events(jctx);
        if (jwin_res == JWIN_RESULT_SHOULD_CLOSE)
        {
            break;
        }
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("jwin_context_handle_events returned: %s (%s)", jwin_result_to_str(jwin_res), jwin_result_msg_str(jwin_res));
        }
        if (draw_state.needs_redraw)
        {
            jta_draw_frame(wnd_ctx, draw_state.view_matrix, &draw_state.meshes, &draw_state.camera);
            draw_state.needs_redraw = 0;
        }
    }
    if (jwin_res != JWIN_RESULT_SHOULD_CLOSE)
    {
        JDM_FATAL("jwin_context_wait_for_events returned: %s (%s)", jwin_result_to_str(jwin_res), jwin_result_msg_str(jwin_res));
    }

    jwnd = NULL;


    jta_solution_free(&solution);
    mesh_uninit(&draw_state.meshes.cylinders);
    mesh_uninit(&draw_state.meshes.spheres);
    mesh_uninit(&draw_state.meshes.cones);
    if (jctx)
    {
        jwin_context_destroy(jctx);
        jctx = NULL;
    }
    jta_free_problem(&problem_setup);
    jta_free_configuration(&master_config);
    JDM_LEAVE_FUNCTION;
    jdm_cleanup_thread();
    //  Clean up the allocators
    {
        ill_jallocator* const allocator = G_JALLOCATOR;
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
        lin_jallocator* const allocator = G_LIN_JALLOCATOR;
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
