#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include <jmem/jmem.h>
#include <jdm.h>


#include "jwin/source/jwin.h"


#include "gfx/drawing.h"
#include "gfx/camera.h"
#include "gfx/bounding_box.h"
#include "ui/jwin_handlers.h"
#include "core/jtaelements.h"
#include "core/jtanaturalbcs.h"
#include "core/jtanumericalbcs.h"
#include "config/config_loading.h"
#include "ui/ui_tree.h"

#include "jta_state.h"



static i32 jdm_error_hook_callback_function(
        const char* thread_name, u32 stack_trace_count, const char* const* stack_trace, jdm_message_level level,
        u32 line, const char* file, const char* function, const char* message, void* param)
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

    (void) param;

    return 0;
}

static void double_free_hook(ill_jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    (void) param;
    JDM_ERROR("Double free detected by allocator (%p)", allocator);
    JDM_LEAVE_FUNCTION;
}

static void invalid_alloc(ill_jallocator* allocator, void* param)
{
    JDM_ENTER_FUNCTION;
    (void) param;
    JDM_ERROR("Bad allocation detected by allocator (%p)", allocator);
    JDM_LEAVE_FUNCTION;
}

static void jmem_trap(uint32_t idx, void* param)
{
    (void)param;
    (void)idx;
    __builtin_trap();
}

static void jwin_err_report_fn(const char* msg, const char* file, int line, const char* function, void* state)
{
    (void) state;
    JDM_ERROR("JWIN error (%s:%d - %s): %s", file, line, function, msg);
}

static void jrui_error_callback(const char* msg, const char* function, const char* file, int line, void* state)
{
    (void) state;
    JDM_ERROR("JRUI error (%s:%d - %s): %s", file, line, function, msg);
}

int main(int argc, char* argv[argc])
{
    jta_state program_state = {};
#ifndef NDEBUG
    memset(&program_state, 0xCC, sizeof(program_state));
#endif
    jta_timer main_timer;
    jta_timer_set(&main_timer);
    //  Create allocators

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
    jio_context* io_ctx;
    jio_context_create_info io_create_info =
            {

            };
    jio_result jio_res = jio_context_create(&io_create_info, &io_ctx);
    if (jio_res != JIO_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not create IO context, reason: %s", jio_result_to_str(jio_res));
    }

    jta_timer_set(&main_timer);
    jta_result jta_res = jta_load_configuration(io_ctx, argv[1], &program_state.master_config);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load program configuration, reason: %s", jta_result_to_str(jta_res));
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Config parsing time: %g sec", dt);


    jta_timer_set(&main_timer);
    jta_res = jta_load_problem(io_ctx, &program_state.master_config.problem, &program_state.problem_setup);
    if (jta_res != JTA_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not load problem, reason: %s", jta_result_to_str(jta_res));
    }
    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Data loading time: %g sec", dt);
    //  Find the bounding box of the geometry
    vec4 geo_base;
    f32 geo_radius;
    gfx_find_bounding_sphere(&program_state.problem_setup.point_list, &geo_base, &geo_radius);


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
        jwin_res = jwin_context_create(&ctx_info, &program_state.wnd_ctx);
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
        jwin_res = jwin_window_create(program_state.wnd_ctx, &win_info, &program_state.main_wnd);
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not create window, reason: %s", jwin_result_msg_str(jwin_res));
        }
    }
    gfx_res = jta_vulkan_window_context_create(program_state.main_wnd, vk_ctx, &wnd_ctx);
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

    gfx_res = jta_structure_meshes_generate_undeformed(
            &undeformed_meshes, &program_state.master_config.display, &program_state.problem_setup, wnd_ctx);
    if (gfx_res != GFX_RESULT_SUCCESS)
    {
        JDM_FATAL("Could not generate undeformed mesh, reason: %s", gfx_result_to_str(gfx_res));
    }


    dt = jta_timer_get(&main_timer);
    JDM_TRACE("Mesh generation time: %g sec", dt);


    JDM_TRACE("Total of %"PRIu64" triangles in the mesh\n",
              mesh_polygon_count(&undeformed_meshes.cylinders) + mesh_polygon_count(&undeformed_meshes.spheres) +
              mesh_polygon_count(&undeformed_meshes.cones));
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

    //  Initialize JRUI context
    {
        unsigned wnd_w, wnd_h;
        jwin_window_get_size(program_state.main_wnd, &wnd_w, &wnd_h);
        const jrui_allocator_callbacks allocator_callbacks =
                {
                .state = G_JALLOCATOR,
                .allocate = (void* (*)(void*, size_t)) ill_jalloc,
                .deallocate = (void (*)(void*, void*)) ill_jfree,
                .reallocate = (void* (*)(void*, void*, size_t)) ill_jrealloc,
                };
        const jrui_error_callbacks error_callbacks =
                {
                .report = jrui_error_callback,
                .param = NULL,
                };




        jrui_color_scheme color_scheme =
                {
                .text = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF},
                .background = {.r = 0x10, .g = 0x20, .b = 0x30, .a = 0xFF},
                .drag_bg = {.r = 0x50, .g = 0x50, .b = 0x50, .a = 0xFF},
                .button_up = {.r = 0x80, .g = 0x80, .b = 0x80, .a = 0xFF},
                .button_down = {.r = 0xC0, .g = 0xC0, .b = 0xC0, .a = 0xFF},
                .button_hover = {.r = 0xA0, .g = 0xA0, .b = 0xA0, .a = 0xFF},
                .button_toggled = {.r = 0xB0, .g = 0xB0, .b = 0xB0, .a = 0xFF},
                .border = {.r = 0xC0, .g = 0xD0, .b = 0xFF, .a = 0xFF},
                .text_input_fg_focused = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF},
                .text_input_bg_focused = {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF},
                .text_input_fg_unfocused = {.r = 0x80, .g = 0x80, .b = 0x80, .a = 0xFF},
                .text_input_bg_unfocused = {.r = 0x40, .g = 0x40, .b = 0x40, .a = 0xFF},
                };
        jta_ui_init(&program_state);    //  This must come before UI_ROOT is used
        jrui_context_create_info context_create_info =
                {
                    .width = wnd_w,
                    .height = wnd_h,
                    .font_type = JRUI_FONT_TYPE_FROM_FONTCONFIG,
                    .font_info.fc_info.fc_string = "Monospace:size=16",
                    .allocator_callbacks = &allocator_callbacks,
                    .error_callbacks = &error_callbacks,
                    .color_scheme = &color_scheme,
                    .root = UI_ROOT,
                };

        jrui_result jrui_res = jrui_context_create(context_create_info, &program_state.ui_state.ui_context);
        if (jrui_res != JRUI_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not initialize UI, reason: %s (%s)", jrui_result_to_str(jrui_res), jrui_result_message(jrui_res));
        }
        unsigned fnt_w, fnt_h;
        const unsigned char* p_tex;
        jrui_context_font_info(program_state.ui_state.ui_context, &fnt_w, &fnt_h, &p_tex);
        jta_texture_create_info tex_info =
                {
                    .format = VK_FORMAT_R8_UNORM,
                    .samples = VK_SAMPLE_COUNT_1_BIT,
                    .tiling = VK_IMAGE_TILING_OPTIMAL,
                };
        gfx_res = jta_texture_load(fnt_w, fnt_h, p_tex, wnd_ctx, tex_info, &program_state.ui_state.ui_font_texture);
        if (gfx_res != GFX_RESULT_SUCCESS)
        {
            JDM_FATAL("Could not load UI font texture, reason: %s", gfx_result_to_str(gfx_res));
        }
        jta_ui_bind_font_texture(wnd_ctx, program_state.ui_state.ui_font_texture);
        jrui_context_set_user_param(program_state.ui_state.ui_context, &program_state);
    }


    program_state.draw_state = (jta_draw_state)
            {
                    .vk_ctx = vk_ctx,
                    .wnd_ctx = wnd_ctx,
                    .camera = camera,
                    .original_camera = camera,
                    .meshes = undeformed_meshes,
                    .needs_redraw = 1,
            };

    jwin_window_show(program_state.main_wnd);
    for (unsigned i = 0; i < JTA_HANDLER_COUNT; ++i)
    {
        jwin_res = jwin_window_set_event_handler(
                program_state.main_wnd, JTA_HANDLER_ARRAY[i].type, JTA_HANDLER_ARRAY[i].callback, &program_state);
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("Failed setting the event handler %u, reason: %s (%s)", i,
                      jwin_result_to_str(jwin_res), jwin_result_msg_str(jwin_res));
        }
    }




    program_state.draw_state.view_matrix = jta_camera_to_view_matrix(&camera);
    program_state.ui_state.ui_vtx_buffer = 0;
    program_state.ui_state.ui_idx_buffer = 0;
    program_state.problem_solution = (jta_solution){};

//    while ((jwin_res = jwin_context_wait_for_events(jctx)) == JWIN_RESULT_SUCCESS)
    for (;;)
    {
        jwin_res = jwin_context_wait_for_events(program_state.wnd_ctx);
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("jwin_context_wait_for_events returned: %s (%s)", jwin_result_to_str(jwin_res),
                      jwin_result_msg_str(jwin_res));
        }
        jwin_res = jwin_context_handle_events(program_state.wnd_ctx);
        if (jwin_res == JWIN_RESULT_SHOULD_CLOSE)
        {
            break;
        }
        if (jwin_res != JWIN_RESULT_SUCCESS)
        {
            JDM_FATAL("jwin_context_handle_events returned: %s (%s)", jwin_result_to_str(jwin_res),
                      jwin_result_msg_str(jwin_res));
        }
        int ui_redraw = 0;
        (void) jrui_context_build(program_state.ui_state.ui_context, &ui_redraw);
        if (ui_redraw)
        {
            jrui_vertex* vertices;
            uint16_t* indices;
            size_t vtx_count, idx_count;
            jrui_receive_vertex_data(program_state.ui_state.ui_context, &vtx_count, &vertices);
            jrui_receive_index_data(program_state.ui_state.ui_context, &idx_count, &indices);
            //  Must make sure that drawing commands have finished before touching these
            if (program_state.ui_state.ui_vtx_buffer && jvm_buffer_allocation_get_size(program_state.ui_state.ui_vtx_buffer) < sizeof(*vertices) * vtx_count)
            {
                jta_vulkan_context_enqueue_destroy_buffer(wnd_ctx, program_state.ui_state.ui_vtx_buffer);
                program_state.ui_state.ui_vtx_buffer = NULL;
            }
            if (!program_state.ui_state.ui_vtx_buffer)
            {
                VkBufferCreateInfo vtx_create_info =
                        {
                                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                .size = sizeof(*vertices) * vtx_count
                        };

                VkResult vk_res = jvm_buffer_create(
                        wnd_ctx->vulkan_allocator, &vtx_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0,
                        &program_state.ui_state.ui_vtx_buffer);
                if (vk_res != VK_SUCCESS)
                {
                    JDM_FATAL("Could not create a new UI VTX buffer, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
                }
            }
            if (program_state.ui_state.ui_idx_buffer && jvm_buffer_allocation_get_size(program_state.ui_state.ui_idx_buffer) < sizeof(*indices) * idx_count)
            {
                jta_vulkan_context_enqueue_destroy_buffer(wnd_ctx, program_state.ui_state.ui_idx_buffer);
                program_state.ui_state.ui_idx_buffer = NULL;
            }
            if (!program_state.ui_state.ui_idx_buffer)
            {
                VkBufferCreateInfo idx_create_info =
                        {
                                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                .size = sizeof(*indices) * idx_count
                        };
                VkResult vk_res = jvm_buffer_create(wnd_ctx->vulkan_allocator, &idx_create_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, &program_state.ui_state.ui_idx_buffer);
                if (vk_res != VK_SUCCESS)
                {
                    JDM_FATAL("Could not create a new UI IDX buffer, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
                }
            }
            
            jta_vulkan_memory_to_buffer(wnd_ctx, 0, sizeof(*vertices) * vtx_count, vertices, 0, program_state.ui_state.ui_vtx_buffer);
            jta_vulkan_memory_to_buffer(wnd_ctx, 0, sizeof(*indices) * idx_count, indices, 0, program_state.ui_state.ui_idx_buffer);
        }
        if (program_state.draw_state.needs_redraw || ui_redraw)
        {
            jta_draw_frame(wnd_ctx, &program_state.ui_state, program_state.draw_state.view_matrix, &program_state.draw_state.meshes, &program_state.draw_state.camera);
            program_state.draw_state.needs_redraw = 0;
        }
    }


    jrui_context_destroy(program_state.ui_state.ui_context);
    jta_solution_free(&program_state.problem_solution);
    if (program_state.wnd_ctx)
    {
        jwin_context_destroy(program_state.wnd_ctx);
    }
    jta_free_problem(&program_state.problem_setup);
    jta_free_configuration(&program_state.master_config);
    jio_context_destroy(io_ctx);
    JDM_LEAVE_FUNCTION;
    jdm_cleanup_thread();
    //  Clean up the allocators
    {
        ill_jallocator* const allocator = G_JALLOCATOR;
        G_JALLOCATOR = NULL;
        uint64_t total_allocations, max_usage, total_allocated, biggest_allocation;
        ill_jallocator_statistics(allocator, &biggest_allocation, &total_allocated, &max_usage, &total_allocations);
        printf(
                "G_JALLOCATOR statistics:\n\tBiggest allocation: %zu b (%g kB)\n\tTotal allocated memory: %g kB (%g MB)"
                "\n\tMax usage: %g kB (%g MB)\n\tTotal allocations: %zu\n", (size_t) biggest_allocation,
                (double) biggest_allocation / 1024.0, (double) total_allocated / 1024.0,
                (double) total_allocated / 1024.0 / 1024.0, (double) max_usage / 1024.0,
                (double) max_usage / 1024.0 / 1024.0, (size_t) total_allocations);
#ifndef NDEBUG
        uint_fast32_t leaked_block_indices[256];
        uint_fast32_t leaked_block_count = ill_jallocator_count_used_blocks(
                allocator, sizeof(leaked_block_indices) / sizeof(*leaked_block_indices), leaked_block_indices);
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
    return 0;
}
