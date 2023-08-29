//
// Created by jan on 11.8.2023.
//

#ifndef JTA_VK_RESOURCES_H
#define JTA_VK_RESOURCES_H
#include "../common/common.h"
#include <jvm.h>
#include "frame_queue.h"
#include "gfxerr.h"
#include "../jwin/source/common.h"

const char* vk_result_to_str(VkResult res);
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

    jvm_image_allocation* depth_img;    //  Image used for the depth view. Since depth attachment does not need to be
                                        //  presented, there is no problem with overwriting it as soon as a render call
                                        //  is done using it.
    VkFormat depth_fmt;                 //  Format used for the depth buffer
    VkImageView depth_view;             //  Image view used to access the depth buffer
    int has_stencil;                    //  Zero, when the depth format does not support stencil

    jta_frame_job_queue** frame_queues; //  Queues, which execute all jobs submitted during previous frame before starting again
};
typedef struct jta_vulkan_swapchain_struct jta_vulkan_swapchain;

struct jta_vulkan_queue_struct
{
    VkQueue handle;
    uint32_t index;
    VkCommandPool transient_pool;
    VkFence fence;
};
typedef struct jta_vulkan_queue_struct jta_vulkan_queue;


/**
 * @brief Purpose of this struct is to hold data which is intrinsically linked with the window and must be created after
 */
struct jta_vulkan_window_context_struct
{
    jta_vulkan_context* ctx;
    VkPhysicalDevice physical_device;       //  physical device used by the program
    VkDevice device;                        //  handle to the interface to the physical device

    VkSurfaceKHR window_surface;            //  surface handle to the window's surface

    jta_vulkan_queue queue_graphics_data;   //  queue for issuing graphics commands
    jta_vulkan_queue queue_transfer_data;   //  queue for issuing transfer commands
    jta_vulkan_queue queue_present_data;    //  queue for issuing presentation commands

    VkPipelineLayout layout_mesh;           //  pipeline layout used to draw 3D mesh geometry with solid colors
    VkPipelineLayout layout_ui;             //  pipeline layout used to draw 2D ui overlay

    VkPipeline pipeline_mesh;               //  pipeline used to draw 3D mesh geometry with solid colors
    VkPipeline pipeline_cf;                 //  pipeline used to draw 3D mesh geometry with solid colors
    VkPipeline pipeline_ui;                 //  pipeline used to draw 2D ui overlay


    jvm_allocator* vulkan_allocator;        //  memory allocator for vulkan memory
    jwin_window* window;                    //  the window this was created for

    jta_vulkan_swapchain swapchain;         //  swapchain for the window

    VkRenderPass render_pass_mesh;          //  render pass for issuing the mesh drawing commands
    jta_vulkan_render_pass pass_mesh;       //  data associated with render pass (framebuffers)

    VkRenderPass render_pass_cf;            //  render pass for issuing 3d rendering commands after the mesh is rendered
    jta_vulkan_render_pass pass_cf;         //  data associated with render pass (framebuffers)

    VkRenderPass render_pass_ui;            //  render pass for issuing 2d rendering commands after the pass_cf is done
    jta_vulkan_render_pass pass_ui;         //  data associated with render pass (framebuffers)

    VkViewport viewport;                    //  viewport for drawing
    VkRect2D scissor;                       //  scissors for drawing

    jvm_buffer_allocation* transfer_buffer; //  allocation for the transfer buffer

    jta_frame_job_queue* current_queue;     // handle to the queue for the current frame, for jobs to execute after the current frame is done
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
        jvm_buffer_allocation* destination);

gfx_result jta_vulkan_queue_begin_transient(
        const jta_vulkan_window_context* ctx, const jta_vulkan_queue* queue,
        VkCommandBuffer* p_cmd_buffer);

gfx_result jta_vulkan_queue_end_transient(
        const jta_vulkan_window_context* ctx, const jta_vulkan_queue* queue, VkCommandBuffer cmd_buffer);

gfx_result jta_vulkan_context_enqueue_frame_job(const jta_vulkan_window_context* ctx, void(*job_callback)(void* job_param), void* job_param);

void jta_vulkan_context_enqueue_destroy_buffer(const jta_vulkan_window_context* ctx, jvm_buffer_allocation* buffer);

#endif //JTA_VK_RESOURCES_H
