//
// Created by jan on 16.1.2023.
//


#ifndef JFW_PLATFORM_H
#define JFW_PLATFORM_H
#include "../jfw_common.h"
#ifdef linux
#include <X11/Xlib.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xlib.h>


typedef struct jfw_vulkan_context_struct jfw_vulkan_context;
typedef struct jfw_window_vk_resources_struct jfw_window_vk_resources;

struct jfw_window_vk_resources_struct
{
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue queue_gfx;
    VkQueue queue_present;
    VkQueue queue_transfer;
    VkExtent2D extent;
    VkSurfaceFormatKHR surface_format;
    u32 n_images;
    VkSwapchainKHR swapchain;
    VkImageView* views;
    u32 n_frames_in_flight;
    VkCommandPool cmd_pool;
    VkCommandBuffer* cmd_buffers;
    VkSemaphore* sem_img_available;
    VkSemaphore* sem_present;
    VkFence* swap_fences;
    VkPhysicalDevice physical_device;
    u32 i_prs_queue, i_gfx_queue, i_trs_queue;
    VkSampleCountFlagBits sample_flags;
};

struct jfw_vulkan_context_struct
{
    VkInstance instance;
    uint32_t n_physical_devices;
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT dbg_messenger;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif
    VkPhysicalDevice* p_physical_devices;
    u32 max_device_extensions;
    u32 max_device_queue_families;
    VkAllocationCallbacks alloc_callback;
    bool has_alloc;
};

struct jfw_context_struct
{
    u32 identifier;
    Display* dpy;
    u32 wnd_count;
    u32 wnd_capacity;
    jfw_window** wnd_array;
    Atom del_atom;
    Atom utf8_atom;
    int screen;
    int depth;
    Window root_window;
    XIC input_ctx;
    int status;
    int mouse_state;
    jfw_window* mouse_focus;
    jfw_window* kbd_focus;

    jfw_vulkan_context vk_ctx;
};

struct jfw_window_platform_struct
{
    jfw_ctx* ctx;
    Window hwnd;
    jfw_window_vk_resources vk_res;
};


#else
#error Not implimented
#endif

jfw_res jfw_context_create(jfw_ctx** p_ctx, VkAllocationCallbacks* alloc_callbacks);

jfw_res jfw_context_destroy(jfw_ctx* ctx);

jfw_res jfw_platform_create(
        jfw_ctx* ctx, jfw_platform* platform, u32 w, u32 h, size_t title_len, const char* title, u32 n_frames_in_filght,
        i32 fixed, jfw_color color);

jfw_window_vk_resources* jfw_window_get_vk_resources(jfw_window* p_window);

jfw_res jfw_platform_destroy(jfw_platform* platform);

jfw_res jfw_context_add_window(jfw_ctx* ctx, jfw_window* p_window);

jfw_res jfw_context_remove_window(jfw_ctx* ctx, jfw_window* p_window);

jfw_res jfw_context_process_events(jfw_ctx* ctx);

int jfw_context_has_events(jfw_ctx* ctx);

jfw_res jfw_context_wait_for_events(jfw_ctx* ctx);

int jfw_context_status(jfw_ctx* ctx);

int jfw_context_set_status(jfw_ctx* ctx, int status);

jfw_res jfw_platform_show(jfw_ctx* ctx, jfw_platform* wnd);

jfw_res jfw_platform_hide(jfw_ctx* ctx, jfw_platform* wnd);

jfw_res jfw_platform_draw_bind(jfw_ctx* ctx, jfw_platform* wnd);

jfw_res jfw_platform_unbind(jfw_ctx* ctx);

jfw_res jfw_platform_swap(jfw_ctx* ctx, jfw_platform* wnd);

u32 jfw_context_window_count(jfw_ctx* ctx);

jfw_res jfw_platform_clear_window(jfw_ctx* ctx, jfw_platform* wnd);

jfw_res jfw_window_update_swapchain(jfw_window* p_window);

#endif //JFW_PLATFORM_H
