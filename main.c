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


linear_jallocator* G_LIN_JALLOCATOR = NULL;
aligned_jallocator* G_ALIGN_JALLOCATOR = NULL;

static jfw_res widget_draw(jfw_widget* this)
{
    return draw_3d_scene(this->window, this->window->user_ptr, jfw_window_get_vk_resources(this->window));
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
        JFW_ERROR("Could not create window");
        goto cleanup;
    }
    VkResult vk_result;
    jfw_window_vk_resources* const vk_res = jfw_window_get_vk_resources(jwnd);
    jfw_vulkan_context* const vk_ctx = &jctx->vk_ctx;
    vk_state vulkan_state;
    jfw_result = vk_state_create(&vulkan_state, vk_res);
    if (!jfw_success(jfw_result))
    {
        JFW_ERROR("Could not create vulkan state");
        goto cleanup;
    }

    jtb_vtx VERTEX_DATA[8] =
            {
                    {.pos = {0.0f, 0.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0x00}},
                    {.pos = {1.0f, 0.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0x00}},
                    {.pos = {1.0f, 1.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0x00}},
                    {.pos = {0.0f, 1.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0x00}},

                    {.pos = {0.0f, 0.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0x00}},
                    {.pos = {1.0f, 1.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0x00}},
                    {.pos = {0.0f, 1.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0x00}},
                    {.pos = {1.0f, 0.0f, 0.0f}, .color = {.r = 0xFF, .g = 0x00, .b = 0xFF, .a = 0x00}},
            };
    u32 INDEX_DATA[12] =
            {
                0, 1, 2,
                1, 3, 2,

                4, 5, 6,
                5, 7, 6,
            };
    vk_buffer_allocation vtx_buffer_allocation, idx_buffer_allocation;
    i32 res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(VERTEX_DATA), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &vtx_buffer_allocation);
    if (res_v < 0)
    {
        JFW_ERROR("Could not allocate vertex buffer memory");
        goto cleanup;
    }
    res_v = vk_buffer_allocate(vulkan_state.buffer_allocator, 1, sizeof(INDEX_DATA), VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &idx_buffer_allocation);
    if (res_v < 0)
    {
        JFW_ERROR("Could not allocate index buffer memory");
        goto cleanup;
    }

    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &vtx_buffer_allocation, sizeof(VERTEX_DATA), VERTEX_DATA);
    vk_transfer_memory_to_buffer(vk_res, &vulkan_state, &idx_buffer_allocation, sizeof(INDEX_DATA), INDEX_DATA);

    vkWaitForFences(vk_res->device, 1, &vulkan_state.fence_transfer_free, VK_TRUE, UINT64_MAX);

    vulkan_state.buffer_vtx = vtx_buffer_allocation;
    vulkan_state.buffer_idx = idx_buffer_allocation;

    jfw_window_set_usr_ptr(jwnd, &vulkan_state);
    jfw_widget* jwidget;
    jfw_result = jfw_widget_create_as_base(jwnd, 1600, 900, 0, 0, &jwidget);
    if (!jfw_success(jfw_result))
    {
        JFW_ERROR("Could not create window's base widget");
        goto cleanup;
    }
    jwidget->dtor_fn = widget_dtor;
    jwidget->draw_fn = widget_draw;

    printf("Hello, World!\n");
    i32 close = 0;
    jfw_window_show(jctx, jwnd);
    while (jfw_success(jfw_context_wait_for_events(jctx)) && !close)
    {
        while (jfw_context_has_events(jctx) && !close)
        {
            close = !jfw_success(jfw_context_process_events(jctx));
        }
        if (!close)
        {
            jfw_window_force_redraw(jctx, jwnd);
            printf("Hello mom!\n");
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
