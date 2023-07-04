#include <stdio.h>
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "mem/aligned_jalloc.h"
#include <jmem/jmem.h>
#include <jdm.h>



#include "jfw/window.h"
#include "jfw/widget-base.h"
#include "jfw/error_system/error_codes.h"


#include "gfx/vk_state.h"
#include "gfx/drawing_3d.h"
#include "gfx/camera.h"
#include "ui.h"



static jfw_res widget_draw(jfw_widget* this)
{
    jtb_draw_state* const draw_state = jfw_widget_get_user_pointer(this);
    vk_state* const state = draw_state->vulkan_state;
    return draw_3d_scene(
            this->window, state, jfw_window_get_vk_resources(this->window), &state->buffer_vtx_geo,
            &state->buffer_vtx_mod, state->mesh, &draw_state->camera) == GFX_RESULT_SUCCESS ? jfw_res_success : jfw_res_error;
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
    //  Estimated to be 16 kB in size
    G_LIN_JALLOCATOR = lin_jallocator_create(1 << 16);
    if (!G_LIN_JALLOCATOR)
    {
        fputs("Could not create linear allocator\n", stderr);
        exit(EXIT_FAILURE);
    }
    G_JALLOCATOR = jallocator_create(1 << 20, 1);
    if (!G_JALLOCATOR)
    {
        fputs("Could not create base jallocator\n", stderr);
        exit(EXIT_FAILURE);
    }


    jdm_init_thread("master",
#ifndef NEBUG
                            JDM_MESSAGE_LEVEL_NONE,
#else
                            JDM_MESSAGE_LEVEL_WARN,
#endif
                          64,
                          64,
                          G_JALLOCATOR);
    JDM_ENTER_FUNCTION;
    jdm_set_hook(jfw_error_hook_callback_function, NULL);
    //  Important: aligned_jallocator works fine, but will not help with debugging of memory. As such, it's best to use
    //  it for Vulkan's use only, as this means that I won't fuck up the memory and then be unable to see where. Just
    //  stick with malloc family when linear allocator can't be used, since that means I can use Vallgrind's memcheck

    

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

    jfw_ctx* jctx = NULL;
    jfw_window* jwnd = NULL;
    jfw_res jfw_result = jfw_context_create(&jctx,
#ifndef NDEBUG
                                            nullptr
#else
                                            &VK_ALLOCATION_CALLBACKS
#endif
                                           );
    if (!jfw_success(jfw_result))
    {
        goto cleanup;
    }
    jfw_result = jfw_window_create(
            jctx, 1600, 900, "Jan's Truss Builder - 0.0.1", (jfw_color) { .a = 0xFF, .r = 0x80, .g = 0x80, .b = 0x80 },
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

    jtb_truss_mesh mesh;
    vulkan_state.mesh = &mesh;

    if ((gfx_res = truss_mesh_init(&mesh, 16)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create truss mesh: %s", gfx_result_to_str(gfx_res));
        goto cleanup;
    }
    if ((gfx_res = truss_mesh_add_between_pts(&mesh, (jfw_color){.r = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(0.1f, 0, 0), 0.0f)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not add a new truss to the mesh: %s", gfx_result_to_str(gfx_res));
        goto cleanup;
    }
    if ((gfx_res = truss_mesh_add_between_pts(&mesh, (jfw_color){.g = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(0, +0.1, 0), 0.0f)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not add a new truss to the mesh: %s", gfx_result_to_str(gfx_res));
        goto cleanup;
    }
    if ((gfx_res = truss_mesh_add_between_pts(&mesh, (jfw_color){.b = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(0, 0, 0.1f), 0.0f)) != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not add a new truss to the mesh: %s", gfx_result_to_str(gfx_res));
        goto cleanup;
    }
//    if (!jfw_success(res = truss_mesh_add_between_pts(&mesh, (jfw_color){.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF}, 0.001f, VEC4(0, 0, 0), VEC4(1, 1, 1), 0.0f)))
//    {
//        JDM_ERROR("Could not add a new truss to the mesh: %s", jfw_error_message(res));
//        goto cleanup;
//    }

    vk_buffer_allocation vtx_buffer_allocation_geometry, vtx_buffer_allocation_model, idx_buffer_allocation;
    i32 res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(jtb_vertex) * mesh.model.vtx_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_buffer_allocation_geometry);
    if (res_v < 0)
    {
        JDM_ERROR("Could not allocate geometry vertex buffer memory");
        goto cleanup;
    }
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &vtx_buffer_allocation_geometry, sizeof(jtb_vertex) * mesh.model.vtx_count, mesh.model.vtx_array);

    res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(*mesh.model.idx_array) * mesh.model.idx_count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &idx_buffer_allocation);
    if (res_v < 0)
    {
        JDM_ERROR("Could not allocate index buffer memory");
        goto cleanup;
    }
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &idx_buffer_allocation, sizeof(*mesh.model.idx_array) * mesh.model.idx_count, mesh.model.idx_array);

    res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(jtb_model_data) * mesh.count, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_buffer_allocation_model);
    if (res_v < 0)
    {
        JDM_ERROR("Could not allocate index buffer memory");
        goto cleanup;
    }
    jtb_model_data* model_data = lin_jalloc(G_LIN_JALLOCATOR, sizeof(*model_data) * mesh.count);
    for (u32 i = 0; i < mesh.count; ++i)
    {
        model_data[i] = jtb_truss_convert_model_data(mesh.model_matrices[i], mesh.normal_matrices[i], mesh.colors[i]);
    }
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &vtx_buffer_allocation_model, sizeof(jtb_model_data) * mesh.count, model_data);
    lin_jfree(G_LIN_JALLOCATOR, model_data);

    vkWaitForFences(vk_res->device, 1, &vulkan_state.fence_transfer_free, VK_TRUE, UINT64_MAX);

    vulkan_state.buffer_vtx_geo = vtx_buffer_allocation_geometry;
    vulkan_state.buffer_vtx_mod = vtx_buffer_allocation_model;
    vulkan_state.buffer_idx = idx_buffer_allocation;

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
    jtb_camera_3d camera;
    jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(0, 0, -1), VEC4(0, 1, 0), 1.0f, 1.0f);
//    jtb_camera_set(&camera, VEC4(0, 0, 0), VEC4(0, 0, +1), VEC4(0, 1, 0));
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
    JDM_LEAVE_FUNCTION;
    jdm_cleanup_thread();
    //  Clean up the allocators
    {
        jallocator* const allocator = G_JALLOCATOR;
        G_JALLOCATOR = NULL;
#ifndef NDEBUG
        uint_fast32_t leaked_block_indices[256];
        uint_fast32_t leaked_block_count = jallocator_count_used_blocks(allocator, sizeof(leaked_block_indices) / sizeof(*leaked_block_indices), leaked_block_indices);
        for (u32 i = 0; i < leaked_block_count; ++i)
        {
            fprintf(stderr, "G_JALLOCATOR did not free block %"PRIuFAST32"\n", leaked_block_indices[i]);
        }
#endif
        jallocator_destroy(allocator);
    }
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
    return 0;
}
