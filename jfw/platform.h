//
// Created by jan on 16.1.2023.
//


#ifndef JFW_PLATFORM_H
#define JFW_PLATFORM_H
#include "jfw_common.h"
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
    uint32_t n_images;
    VkSwapchainKHR swapchain;
    VkImageView* views;
    uint32_t n_frames_in_flight;
    VkCommandPool cmd_pool;
    VkCommandBuffer* cmd_buffers;
    VkSemaphore* sem_img_available;
    VkSemaphore* sem_present;
    VkFence* swap_fences;
    VkPhysicalDevice physical_device;
    uint32_t i_prs_queue, i_gfx_queue, i_trs_queue;
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
    uint32_t max_device_extensions;
    uint32_t max_device_queue_families;
    VkAllocationCallbacks vk_alloc_callback;
    bool has_alloc;
};

struct jfw_context_struct
{
    uint32_t identifier;
    Display* dpy;
    uint32_t wnd_count;
    uint32_t wnd_capacity;
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
    unsigned int last_button_id;
    jfw_allocator_callbacks allocator_callbacks;
    Time last_button_time;
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

jfw_result jfw_context_create(
        jfw_ctx** p_ctx, VkAllocationCallbacks* vk_alloc_callbacks, const jfw_allocator_callbacks* allocator_callbacks);

jfw_result jfw_context_destroy(jfw_ctx* ctx);

jfw_result jfw_platform_create(
        jfw_ctx* ctx, jfw_platform* platform, uint32_t w, uint32_t h, size_t title_len, const char* title, uint32_t n_frames_in_filght,
        int32_t fixed, jfw_color color);

jfw_window_vk_resources* jfw_window_get_vk_resources(jfw_window* p_window);

jfw_result jfw_platform_destroy(jfw_platform* platform);

jfw_result jfw_context_add_window(jfw_ctx* ctx, jfw_window* p_window);

jfw_result jfw_context_remove_window(jfw_ctx* ctx, jfw_window* p_window);

jfw_result jfw_context_process_events(jfw_ctx* ctx);

int jfw_context_has_events(jfw_ctx* ctx);

jfw_result jfw_context_wait_for_events(jfw_ctx* ctx);

int jfw_context_status(jfw_ctx* ctx);

int jfw_context_set_status(jfw_ctx* ctx, int status);

jfw_result jfw_platform_show(jfw_ctx* ctx, jfw_platform* wnd);

jfw_result jfw_platform_hide(jfw_ctx* ctx, jfw_platform* wnd);

uint32_t jfw_context_window_count(jfw_ctx* ctx);

jfw_result jfw_platform_clear_window(jfw_ctx* ctx, jfw_platform* wnd);

jfw_result jfw_window_update_swapchain(jfw_window* p_window);

#endif //JFW_PLATFORM_H
