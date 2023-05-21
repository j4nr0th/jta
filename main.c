#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <assert.h>

#include "mem/aligned_jalloc.h"
#include "mem/jalloc.h"
#include "mem/lin_jalloc.h"

#include "jfw/window.h"
#include "jfw/widget-base.h"
#include "jfw/error_system/error_codes.h"
#include "jfw/error_system/error_stack.h"


#include "vk_state.h"
#include "drawing_3d.h"
#include "camera.h"
#include "ui.h"


linear_jallocator* G_LIN_JALLOCATOR = NULL;
aligned_jallocator* G_ALIGN_JALLOCATOR = NULL;

static jfw_res widget_draw(jfw_widget* this)
{
    jtb_draw_state* const draw_state = jfw_widget_get_user_pointer(this);
    vk_state* const state = draw_state->vulkan_state;
    return draw_3d_scene(
            this->window, state, jfw_window_get_vk_resources(this->window), &state->buffer_vtx_geo,
            &state->buffer_vtx_mod, state->mesh);
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
    JFW_INFO("Called aligned_jalloc(%p, %zu, %zu) = %p\n", pUserData, alignment, size, ptr);
    return ptr;
}

static void* VKAPI_PTR vk_aligned_reallocation_fn(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
    void* ptr = aligned_jrealloc((aligned_jallocator*) pUserData, pOriginal, alignment, size);
    if (alignment > 8)
    JFW_INFO("Called aligned_rejalloc(%p, %p, %zu, %zu) = %p\n", pUserData, pOriginal, alignment, size, ptr);
    return ptr;
}

static void VKAPI_PTR vk_aligned_free_fn(void* pUserData, void* pMemory)
{
    assert(pUserData == G_ALIGN_JALLOCATOR);
//    JFW_INFO("Called aligned_jfree(%p, %p)", pUserData, pMemory);
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


static i32 jfw_error_hook_callback_function(const char* thread_name, u32 stack_trace_count, const char*const* stack_trace, jfw_error_level level, u32 line, const char* file, const char* function, const char* message, void* param)
{
    switch (level)
    {
    case jfw_error_level_info:fprintf(stdout, "JFW_INFO (thread %s - %s): %s\n", thread_name, function, message);
        break;
    case jfw_error_level_warn:
        fprintf(stderr, "Warning issued from thread \"%s\", stack trace (%s", thread_name, stack_trace[0]);
        for (u32 i = 1; i < stack_trace_count; ++i)
        {
            fprintf(stderr, "-> %s", stack_trace[i]);
        }
        fprintf(stderr, ") in file \"%s\" on line \"%i\" (function \"%s\"): %s\n\n", file, line, function, message);
        break;

    case jfw_error_level_err:
    case jfw_error_level_crit:
    case jfw_error_level_none:
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


int main(int argc, char* argv[argc])
{
    jfw_error_init_thread("master",
#ifndef NEBUG
                            jfw_error_level_none,
#else
                            jfw_error_level_warn,
#endif
                          64,
                          64
                          );
    JFW_ENTER_FUNCTION;
    jfw_error_set_hook(jfw_error_hook_callback_function, NULL);
    //  Important: aligned_jallocator works fine, but will not help with debugging of memory. As such, it's best to use
    //  it for Vulkan's use only, as this means that I won't fuck up the memory and then be unable to see where. Just
    //  stick with malloc family when linear allocator can't be used, since that means I can use Vallgrind's memcheck

    //  Create allocators
    //  Estimated to be 512 kB per small pool and 512 kB per med pool
    G_ALIGN_JALLOCATOR = aligned_jallocator_create(1, 1);
    if (!G_ALIGN_JALLOCATOR)
    {
        fputs("Could not create aligned allocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    //  Estimated to be 16 kB in size
    G_LIN_JALLOCATOR = lin_jallocator_create(1 << 16);
    if (!G_LIN_JALLOCATOR)
    {
        fputs("Could not create linear allocator\n", stderr);
        exit(EXIT_FAILURE);
    }

#ifdef NDEBUG
    VkAllocationCallbacks VK_ALLOCATION_CALLBACKS =
            {
            .pUserData = G_ALIGN_JALLOCATOR,
            .pfnAllocation = vk_aligned_allocation_fn,
            .pfnReallocation = vk_aligned_reallocation_fn,
            .pfnFree = vk_aligned_free_fn,
            .pfnInternalAllocation = vk_internal_alloc_notif,
            .pfnInternalFree = vk_internal_free_notif,
            };
#endif

//    mtx4 matrix = mtx4_view_look_at(VEC4(0, 0, 0), VEC4(1, 0, 0), 0.0f);
//    matrix = mtx4_view_matrix(VEC4(0, -1, 0), VEC4(0, 1, 0), 0.0f);
//    printf("% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n% 4.4f % 4.4f % 4.4f % 4.4f\n", matrix.col0.s0, matrix.col1.s0, matrix.col2.s0, matrix.col3.s0, matrix.col0.s1, matrix.col1.s1, matrix.col2.s1, matrix.col3.s1, matrix.col0.s2, matrix.col1.s2, matrix.col2.s2, matrix.col3.s2, matrix.col0.s3, matrix.col1.s3, matrix.col2.s3, matrix.col3.s3);
//    exit(0);
//    vec4 res_vec = mtx4_vector_mul(matrix, VEC4(1, 0, 0));
//    printf("Res: [%g, %g, %g]\n", res_vec.x, res_vec.y, res_vec.z);
//    res_vec = mtx4_vector_mul(matrix, VEC4(0, 1, 0));
//    printf("Res: [%g, %g, %g]\n", res_vec.x, res_vec.y, res_vec.z);
//    res_vec = mtx4_vector_mul(matrix, VEC4(0, 0, 1));
//    printf("Res: [%g, %g, %g]\n", res_vec.x, res_vec.y, res_vec.z);
//
//    matrix = mtx4_view_look_at(VEC4(0, 1, 0), VEC4(0, 2, 0), M_PI_2);
//    res_vec = mtx4_vector_mul(matrix, VEC4(1, 0, 0));
//    printf("Res: [%g, %g, %g]\n", res_vec.x, res_vec.y, res_vec.z);
//    res_vec = mtx4_vector_mul(matrix, VEC4(0, 1, 0));
//    printf("Res: [%g, %g, %g]\n", res_vec.x, res_vec.y, res_vec.z);
//    res_vec = mtx4_vector_mul(matrix, VEC4(0, 0, 1));
//    printf("Res: [%g, %g, %g]\n", res_vec.x, res_vec.y, res_vec.z);
//    exit(0);
    jfw_ctx* jctx = NULL;
    jfw_window* jwnd = NULL;
    jfw_res res = jfw_context_create(&jctx,
#ifndef NDEBUG
                                     nullptr
#else
                                            &VK_ALLOCATION_CALLBACKS
#endif
                                    );
    if (!jfw_success(res))
    {
        goto cleanup;
    }
    res = jfw_window_create(
            jctx, 1600, 900, "Jan's Truss Builder - 0.0.1", (jfw_color) { .a = 0xFF, .r = 0x80, .g = 0x80, .b = 0x80 },
            &jwnd, 0);
    if (!jfw_success(res))
    {
        JFW_ERROR("Could not create window");
        goto cleanup;
    }
    jfw_window_show(jctx, jwnd);
    VkResult vk_result;
    jfw_window_vk_resources* const vk_res = jfw_window_get_vk_resources(jwnd);
    jfw_vulkan_context* const vk_ctx = &jctx->vk_ctx;
    vk_state vulkan_state;
    res = vk_state_create(&vulkan_state, vk_res);
    if (!jfw_success(res))
    {
        JFW_ERROR("Could not create vulkan state");
        goto cleanup;
    }

    jtb_truss_mesh mesh;
    vulkan_state.mesh = &mesh;

    if (!jfw_success(res = truss_mesh_init(&mesh, 16)))
    {
        JFW_ERROR("Could not create truss mesh: %s", jfw_error_message(res));
        goto cleanup;
    }
    if (!jfw_success(res = truss_mesh_add_between_pts(&mesh, (jfw_color){.r = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(0.1f, 0, 0), 0.0f)))
    {
        JFW_ERROR("Could not add a new truss to the mesh: %s", jfw_error_message(res));
        goto cleanup;
    }
    if (!jfw_success(res = truss_mesh_add_between_pts(&mesh, (jfw_color){.g = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(0, +0.1, 0), 0.0f)))
    {
        JFW_ERROR("Could not add a new truss to the mesh: %s", jfw_error_message(res));
        goto cleanup;
    }
    if (!jfw_success(res = truss_mesh_add_between_pts(&mesh, (jfw_color){.b = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(0, 0, 0.1f), 0.0f)))
    {
        JFW_ERROR("Could not add a new truss to the mesh: %s", jfw_error_message(res));
        goto cleanup;
    }
//    if (!jfw_success(res = truss_mesh_add_between_pts(&mesh, (jfw_color){.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(1, 1, 1), 0.0f)))
//    {
//        JFW_ERROR("Could not add a new truss to the mesh: %s", jfw_error_message(res));
//        goto cleanup;
//    }

    vk_buffer_allocation vtx_buffer_allocation_geometry, vtx_buffer_allocation_model, idx_buffer_allocation;
    i32 res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(jtb_vertex) * mesh.model.vtx_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_buffer_allocation_geometry);
    if (res_v < 0)
    {
        JFW_ERROR("Could not allocate geometry vertex buffer memory");
        goto cleanup;
    }
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &vtx_buffer_allocation_geometry, sizeof(jtb_vertex) * mesh.model.vtx_count, mesh.model.vtx_array);

    res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(*mesh.model.idx_array) * mesh.model.idx_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &idx_buffer_allocation);
    if (res_v < 0)
    {
        JFW_ERROR("Could not allocate index buffer memory");
        goto cleanup;
    }
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &idx_buffer_allocation, sizeof(*mesh.model.idx_array) * mesh.model.idx_count, mesh.model.idx_array);

    res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(jtb_model_data) * mesh.count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_buffer_allocation_model);
    if (res_v < 0)
    {
        JFW_ERROR("Could not allocate index buffer memory");
        goto cleanup;
    }
    jtb_model_data* model_data = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*model_data) * mesh.count);
    for (u32 i = 0; i < mesh.count; ++i)
    {
        model_data[i] = jtb_truss_convert_model_data(mesh.model_matrices[i], mesh.colors[i]);
    }
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &vtx_buffer_allocation_model, sizeof(jtb_model_data) * mesh.count, model_data);
    lin_jfree(G_LIN_JALLOCATOR, model_data);

    vkWaitForFences(vk_res->device, 1, &vulkan_state.fence_transfer_free, VK_TRUE, UINT64_MAX);

    vulkan_state.buffer_vtx_geo = vtx_buffer_allocation_geometry;
    vulkan_state.buffer_vtx_mod = vtx_buffer_allocation_model;
    vulkan_state.buffer_idx = idx_buffer_allocation;

    jfw_window_set_usr_ptr(jwnd, &vulkan_state);
    jfw_widget* jwidget;
    res = jfw_widget_create_as_base(jwnd, 1600, 900, 0, 0, &jwidget);
    if (!jfw_success(res))
    {
        JFW_ERROR("Could not create window's base widget");
        goto cleanup;
    }
    jwidget->dtor_fn = widget_dtor;
    jwidget->draw_fn = widget_draw;
    jwidget->functions.mouse_button_press = truss_mouse_button_press;
    jwidget->functions.mouse_button_release = truss_mouse_button_release;
    jwidget->functions.mouse_motion = truss_mouse_motion;
    jtb_camera_3d camera;
    jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(0, 0, -1), VEC4(0, 1, 0));
    jtb_draw_state draw_state =
            {
            .vulkan_state = &vulkan_state,
            .camera = camera,
            .vulkan_resources = vk_res,
            };
    jfw_widget_set_user_pointer(jwidget, &draw_state);
    vulkan_state.view = jtb_camera_to_view_matrix(&camera);


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
    if (jctx)
    {
        jfw_context_destroy(jctx);
        jctx = NULL;
    }
    //  Clean up the allocators
    {
        linear_jallocator* const allocator = G_LIN_JALLOCATOR;
        G_LIN_JALLOCATOR = NULL;
        lin_jallocator_destroy(allocator);
    }
    {
        printf("Total lifetime waste by aligned allocator: %zu\n", aligned_jallocator_lifetime_waste(G_ALIGN_JALLOCATOR));
        aligned_jallocator* const allocator = G_ALIGN_JALLOCATOR;
        G_ALIGN_JALLOCATOR = NULL;
        aligned_jallocator_destroy(allocator);
    }
    JFW_LEAVE_FUNCTION;
    return 0;
}
