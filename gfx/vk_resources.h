//
// Created by jan on 11.8.2023.
//

#ifndef JTA_VK_RESOURCES_H
#define JTA_VK_RESOURCES_H
#include "../common/common.h"
#include "../mem/vk_mem_allocator.h"
#include "gfxerr.h"
#include "../jwin/source/common.h"

const char* vk_result_to_str(VkResult res);

const char* vk_result_message(VkResult res);

/**
 * @brief Purpose of this struct is to hold data which is window independent and can be created before the window
 */
struct jta_vulkan_context_struct
{
    const VkAllocationCallbacks* allocator; //  memory allocation callbacks
    VkInstance instance;                    //  instance handle

    //  Function pointers for extensions
#ifndef NDEBUG
    VkDebugUtilsMessengerEXT dbg_messenger;
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif
};
typedef struct jta_vulkan_context_struct jta_vulkan_context;

struct jta_vulkan_render_pass_struct
{
    VkRenderPass render_pass;
    VkPipeline pipeline;
    VkFramebuffer* framebuffers;
    unsigned framebuffer_count;
};
typedef struct jta_vulkan_render_pass_struct jta_vulkan_render_pass;

/**
 * @brief Purpose of this struct is to hold data related to an individual window's swapchain
 */
struct jta_vulkan_swapchain_struct
{
    VkExtent2D window_extent;           //  The extent of the window's surface
    VkSwapchainKHR swapchain;           //  Handle to the swapchin
    VkSurfaceFormatKHR window_format;   //  Format of the swapchain
    VkCommandPool cmd_pool;             //  Command pool used for command buffers used by the swapchain

    uint32_t image_count;               //  Number of images in the swapchain
    VkImageView* image_views;           //  Array of image views to the images in the swapchain

    uint32_t frames_in_flight;          //  Number of frames that can be drawn before one is presented
    uint32_t current_img;               //  Index of the current image
    uint32_t last_img;                  //  Index of last image in use
    VkCommandBuffer* cmd_buffers;       //  Command buffer array, one per frame in flight
    VkSemaphore* sem_present;           //  Semaphores used to synchronize present calls
    VkSemaphore* sem_available;         //  Semaphores used to synchronize image available calls
    VkFence* fen_swap;                  //  Fences used to synchronize swapchain's swap

    VkImage depth_img;                  //  Image used for the depth view. Since depth attachment does not need to be
                                        //  presented, there is no problem with overwriting it as soon as a render call
                                        //  is done using it.
    VkDeviceMemory depth_mem;           //  Memory backing the depth_img
    VkFormat depth_fmt;                 //  Format used for the depth buffer
    VkImageView depth_view;             //  Image view used to access the depth buffer
    int has_stencil;                    //  Zero, when the depth format does not support stencil

};
typedef struct jta_vulkan_swapchain_struct jta_vulkan_swapchain;


/**
 * @brief Purpose of this struct is to hold data which is intrinsically linked with the window and must be created after
 */
struct jta_vulkan_window_context_struct
{
    jta_vulkan_context* ctx;
    VkPhysicalDevice physical_device;       //  physical device used by the program
    VkDevice device;                        //  handle to the interface to the physical device

    VkSurfaceKHR window_surface;            //  surface handle to the window's surface

    uint32_t i_queue_gfx;                   //  device's graphical queue index
    VkQueue queue_gfx;                      //  device's graphical queue handle
    uint32_t i_queue_present;               //  index of the device's queue used for presentation
    VkQueue queue_present;                  //  handle of the device's queue used for presentation
    uint32_t i_queue_transfer;              //  index of the device's queue used for memory transfer operations
    VkQueue queue_transfer;                 //  handle of the device's queue used for memory transfer operations

    VkPipelineLayout layout_mesh;           //  pipeline layout used to draw 3D mesh geometry with solid colors

    VkPipeline pipeline_mesh;               //  pipeline layout used to draw 3D mesh geometry with solid colors
    VkPipeline pipeline_cf;                 //  pipeline layout used to draw 3D mesh geometry with solid colors

    vk_buffer_allocator* buffer_allocator;  //  memory allocator for vulkan memory
    jwin_window* window;                    //  the window this was created for

    jta_vulkan_swapchain swapchain;         //  swapchain for the window

    VkRenderPass render_pass_mesh;          //  render pass for issuing the mesh drawing commands
    jta_vulkan_render_pass pass_mesh;       //  data associated with render pass (framebuffers)

    VkRenderPass render_pass_cf;            //  render pass for issuing 3d rendering commands after the mesh is rendered
    jta_vulkan_render_pass pass_cf;         //  data associated with render pass (framebuffers)

    VkViewport viewport;                    //  viewport for drawing
    VkRect2D scissor;                       //  scissors for drawing

    VkCommandPool transfer_pool;            //  command pool used to create command buffers for transfer operations
    vk_buffer_allocation transfer_buffer;   //  allocation for the transfer buffer
    VkFence transfer_buffer_fence;          //  fence to synchronize the transfer buffer
};
typedef struct jta_vulkan_window_context_struct jta_vulkan_window_context;

gfx_result
jta_vulkan_context_create(
        const VkAllocationCallbacks* allocator, const char* app_name, uint32_t app_version, jta_vulkan_context** p_out);

void jta_vulkan_context_destroy(jta_vulkan_context* ctx);

gfx_result jta_vulkan_window_context_create(jwin_window* win, jta_vulkan_context* ctx, jta_vulkan_window_context** p_out);

void jta_vulkan_window_context_destroy(jta_vulkan_window_context* ctx);

gfx_result jta_vulkan_begin_draw(jta_vulkan_window_context* ctx, VkCommandBuffer* p_buffer, unsigned* p_img);

gfx_result jta_vulkan_end_draw(jta_vulkan_window_context* ctx, VkCommandBuffer cmd_buffer);

gfx_result jta_vulkan_memory_to_buffer(
        jta_vulkan_window_context* ctx, uint64_t offset, uint64_t size, const void* ptr, uint64_t destination_offset,
        const vk_buffer_allocation* destination);

#endif //JTA_VK_RESOURCES_H
