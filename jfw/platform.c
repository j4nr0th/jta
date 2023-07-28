//
// Created by jan on 16.1.2023.
//

#include "platform.h"
#include "jdm.h"
#include "window.h"
#include <X11/extensions/Xrender.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xlocale.h>
#include <X11/Xutil.h>


static const char* REQUIRED_LAYERS_ARRAY[] =
        {
#ifndef NDEBUG
                //  DEBUG LAYERS
                "VK_LAYER_KHRONOS_validation",
#endif
                //  NON-DEBUG LAYERS

        };

static const uint64_t REQUIRED_LAYERS_LENGTHS[] =
        {
#ifndef NDEBUG
                //  DEBUG LAYERS
                sizeof("VK_LAYER_KHRONOS_validation") - 1,
#endif
                //  NON-DEBUG LAYERS

        };
#define REQUIRED_LAYERS_COUNT (sizeof(REQUIRED_LAYERS_ARRAY) / sizeof(*REQUIRED_LAYERS_ARRAY))

static const char* REQUIRED_EXTENSIONS_ARRAY[] =
        {
#ifndef NDEBUG
                //  DEBUG EXTENSIONS
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
                //  NON-DEBUG EXTENSIONS
                VK_KHR_SURFACE_EXTENSION_NAME,
//                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        };
static const uint64_t REQUIRED_EXTENSIONS_LENGTHS[] =
        {
#ifndef NDEBUG
                //  DEBUG EXTENSIONS
                sizeof(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) - 1,
                sizeof(VK_EXT_DEBUG_REPORT_EXTENSION_NAME) - 1,
#endif
                //  NON-DEBUG EXTENSIONS
                sizeof(VK_KHR_SURFACE_EXTENSION_NAME) - 1,
//                sizeof(VK_KHR_SWAPCHAIN_EXTENSION_NAME) - 1,
                sizeof(VK_KHR_XLIB_SURFACE_EXTENSION_NAME) - 1,
        };
#define REQUIRED_EXTENSIONS_COUNT (sizeof(REQUIRED_EXTENSIONS_ARRAY) / sizeof(*REQUIRED_EXTENSIONS_ARRAY))


static const char* const DEVICE_REQUIRED_EXTENSIONS[] =
        {
                VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
static const uint64_t DEVICE_REQUIRED_LENGTHS[] =
        {
                sizeof(VK_KHR_SWAPCHAIN_EXTENSION_NAME) - 1,
        };
#define DEVICE_REQUIRED_COUNT (sizeof(DEVICE_REQUIRED_EXTENSIONS) / sizeof(*DEVICE_REQUIRED_EXTENSIONS))


static inline VkBool32 match_extension_name(const char* const extension_name, const uint64_t extension_name_len, const char* str_to_check)
{
    if (strstr(str_to_check, extension_name) != str_to_check)
    {

        return 0;
    }
    str_to_check += extension_name_len;
    if (*str_to_check == 0)
    {

        return 1;
    }
    char* end_ptr;
    uint64_t v = strtoull(str_to_check, &end_ptr, 10);


    return (end_ptr != str_to_check && v != 0);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData)
{
    static const VkDebugUtilsMessageTypeFlagBitsEXT TYPE_FLAG_BIT_VALUES[] =
            {
                    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT,
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                    VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT,
            };
    static const char* const TYPE_STRINGS[] =
            {
                    "GENERAL",
                    "VALIDATION",
                    "PERFORMANCE",
                    "DEVICE_ADDRESS_BINDING",
            };
    char category_buffer[64];
    uint32_t cat_buffer_usage = 0;
    for (uint32_t i = 0; i < (sizeof(TYPE_FLAG_BIT_VALUES)/sizeof(*TYPE_FLAG_BIT_VALUES)); ++i)
    {
        if (messageType & TYPE_FLAG_BIT_VALUES[i])
        {
            cat_buffer_usage += snprintf(category_buffer, sizeof(category_buffer) - cat_buffer_usage, "%s ", TYPE_STRINGS[i]);
        }
    }
    if (category_buffer[cat_buffer_usage - 1] == ' ')
    {
        category_buffer[cat_buffer_usage - 1] = 0;
    }

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        JDM_ERROR("Vulkan DBG utils ERROR (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
//        assert(0);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        JDM_WARN("Vulkan DBG utils warning (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        JDM_INFO("Vulkan DBG utils information (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
    }
//    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
//    {
//        JDM_INFO("Vulkan DBG utils diagnostic (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
//    }
//    else
//    {
//        JDM_ERROR("Vulkan DBG utils (unknown severity) (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
//    }


    //  VK_FALSE means that the call which caused this message to be issued is not interrupted as a
    //  result of just calling this function. If this is just some info then you do not want this to interrupt everything
    return VK_FALSE;
}

static jfw_result jfw_vulkan_context_create(jfw_ctx* ctx, jfw_vulkan_context* this, const char* app_name, uint32_t app_version, VkAllocationCallbacks* alloc_callbacks)
{
    JDM_ENTER_FUNCTION;
    uint32_t mtx_created = 0;
    VkResult vk_res;
    jfw_result res;

    //  Create instance with appropriate layers and extensions
    {
        VkInstance instance;
        VkApplicationInfo app_info =
                {
                        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                        .apiVersion = VK_API_VERSION_1_0,
                        .engineVersion = 1,
                        .pApplicationName = app_name,
                        .pEngineName = "jfw_gfx",
                        .applicationVersion = app_version,
                };

        uint32_t count;
        vk_res = vkEnumerateInstanceLayerProperties(&count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Vulkan error with enumerating instance layer properties: %s", jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        VkLayerProperties* layer_properties;
        layer_properties = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, count * sizeof(*layer_properties));
        if (!layer_properties)
        {
            JDM_ERROR("Failed allocating memory for vulkan layer properties array");
            res = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkEnumerateInstanceLayerProperties(&count, layer_properties);
        if (vk_res != VK_SUCCESS)
        {
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, layer_properties);
            JDM_ERROR("Vulkan error with enumerating instance layer properties: %s", jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }

        uint32_t req_layers_found = 0;
        for (uint32_t i = 0; i < count && req_layers_found < REQUIRED_LAYERS_COUNT; ++i)
        {
            const char* name = layer_properties[i].layerName;
            for (uint32_t j = 0; j < REQUIRED_LAYERS_COUNT; ++j)
            {
                const char* lay_name = REQUIRED_LAYERS_ARRAY[j];
                const uint64_t lay_len = REQUIRED_LAYERS_LENGTHS[j];
                if (match_extension_name(lay_name, lay_len, name) != 0)
                {
                    req_layers_found += 1;
                    break;
                }
            }
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, layer_properties);

        if (req_layers_found != REQUIRED_LAYERS_COUNT)
        {
            JDM_ERROR("Could only find %u out of %lu required vulkan layers", req_layers_found, REQUIRED_LAYERS_COUNT);
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }

        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Vulkan error with enumerating instance extension properties: %s", jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }

        VkExtensionProperties* extension_properties;
        extension_properties = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, count * sizeof(*extension_properties));
        if (!extension_properties)
        {
            JDM_ERROR("Failed allocating memory for extension properties");
            res = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &count, extension_properties);
        if (vk_res != VK_SUCCESS)
        {
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, extension_properties);
            JDM_ERROR("Vulkan error with enumerating extension layer properties: %s", jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        uint32_t req_extensions_found = 0;
        for (uint32_t i = 0; i < count && req_extensions_found < REQUIRED_EXTENSIONS_COUNT; ++i)
        {
            const char* name = extension_properties[i].extensionName;
            for (uint32_t j = 0; j < REQUIRED_EXTENSIONS_COUNT; ++j)
            {
                const char* ext_name = REQUIRED_EXTENSIONS_ARRAY[j];
                const uint64_t ext_len = REQUIRED_EXTENSIONS_LENGTHS[j];
                if (match_extension_name(ext_name, ext_len, name) != 0)
                {
                    req_extensions_found += 1;
                    break;
                }
            }
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, extension_properties);

        if (req_extensions_found != REQUIRED_EXTENSIONS_COUNT)
        {
            JDM_ERROR("Could only find %u out of %lu required vulkan extensions", req_extensions_found, REQUIRED_EXTENSIONS_COUNT);
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }

        VkInstanceCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                        .enabledExtensionCount = REQUIRED_EXTENSIONS_COUNT,
                        .ppEnabledExtensionNames = REQUIRED_EXTENSIONS_ARRAY,
                        .enabledLayerCount = REQUIRED_LAYERS_COUNT,
                        .ppEnabledLayerNames = REQUIRED_LAYERS_ARRAY,
                        .pApplicationInfo = &app_info,
                };

        vk_res = vkCreateInstance(&create_info, alloc_callbacks, &instance);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating vulkan instance with desired extensions and layers, reason: %s",
                      jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        this->instance = instance;
        this->has_alloc = alloc_callbacks != NULL;
        if (alloc_callbacks)
        {
            this->vk_alloc_callback = *alloc_callbacks;
        }
    }

#ifndef NDEBUG
    //  Create the debug messenger when in debug build configuration
    {
        this->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT");
        if (!this->vkCreateDebugUtilsMessengerEXT)
        {
            res = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Failed fetching the address for procedure \"vkCreateDebugUtilsMessengerEXT\"");
            goto failed;
        }
        this->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (!this->vkDestroyDebugUtilsMessengerEXT)
        {
            res = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Failed fetching the address for procedure \"vkDestroyDebugUtilsMessengerEXT\"");
            goto failed;
        }
        VkDebugUtilsMessengerCreateInfoEXT create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,
                        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                        .pfnUserCallback = debug_callback,
                };

        VkDebugUtilsMessengerEXT dbg_messenger;
        vk_res = this->vkCreateDebugUtilsMessengerEXT(this->instance, &create_info, NULL, &dbg_messenger);
        if (dbg_messenger == VK_NULL_HANDLE)
        {
            res = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Failed creating VkDebugUtilsMessenger");
            goto failed;
        }
        this->dbg_messenger = dbg_messenger;
    }
#endif

    //  Enumerate physical devices
    {
        uint32_t count;
        vk_res = vkEnumeratePhysicalDevices(this->instance, &count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not enumerate instance physical devices, reason: %s", jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        if (!count)
        {
            JDM_ERROR("The instance had found %u available physical devices!", count);
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        VkPhysicalDevice* devices;
        devices = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, count * sizeof(*devices));
        if (!devices)
        {
            JDM_ERROR("Could not allocate memory for physical device list");
            res = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkEnumeratePhysicalDevices(this->instance, &count, devices);
        if (vk_res != VK_SUCCESS)
        {
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, devices);
            JDM_ERROR("Could not enumerate instance physical devices, reason: %s", jfw_vk_error_msg(vk_res));
            res = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        this->n_physical_devices = count;
        this->p_physical_devices = devices;

        for (uint32_t i = 0; i < count; ++i)
        {
            uint32_t n;
            vk_res = vkEnumerateDeviceExtensionProperties(devices[i], NULL, &n, NULL);
            if (vk_res == VK_SUCCESS && n > this->max_device_extensions)
            {
                this->max_device_extensions = n;
            }
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &n, NULL);
            if (n > this->max_device_queue_families)
            {
                this->max_device_queue_families = n;
            }
        }
    }


    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;

    failed:

    if (this->instance)
    {
        if (this->n_physical_devices)
        {
            assert(this->p_physical_devices != NULL);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, this->p_physical_devices);
        }
#ifndef NDEBUG
        if (this->dbg_messenger)
        {
            assert(this->vkDestroyDebugUtilsMessengerEXT);
            this->vkDestroyDebugUtilsMessengerEXT(this->instance, this->dbg_messenger, NULL);
        }
#endif
        vkDestroyInstance(this->instance, NULL);
        this->instance = VK_NULL_HANDLE;
    }
    JDM_LEAVE_FUNCTION;
    return res;
}

static jfw_result jfw_vulkan_context_destroy(jfw_ctx* ctx, jfw_vulkan_context* this)
{
    JDM_ENTER_FUNCTION;
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, this->p_physical_devices);
#ifndef NDEBUG
    this->vkDestroyDebugUtilsMessengerEXT(this->instance, this->dbg_messenger, NULL);
#endif
    vkDestroyInstance(this->instance, this->has_alloc ? &this->vk_alloc_callback : NULL);
    memset(this, 0, sizeof(*this));
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result score_physical_device(
        VkPhysicalDevice device, VkSurfaceKHR surface, int32_t* p_score, uint32_t n_prop_buffer,
        VkExtensionProperties* ext_buffer, int32_t* p_queue_gfx, int32_t* p_queue_prs, int32_t* p_queue_trs, uint32_t n_queue_buffer,
        VkQueueFamilyProperties* queue_buffer, VkSampleCountFlagBits* p_sample_flags)
{
    JDM_ENTER_FUNCTION;
    int32_t score = 0;
    uint32_t found_props = 0;
    uint32_t device_extensions = 0;
    VkResult vk_res = vkEnumerateDeviceExtensionProperties(device, NULL, &device_extensions, NULL);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not enumerate device extension properties for device, reason: %s", jfw_vk_error_msg(vk_res));
        return JFW_RESULT_VK_FAIL;
    }
    assert(device_extensions <= n_prop_buffer);
    vk_res = vkEnumerateDeviceExtensionProperties(device, NULL, &device_extensions, ext_buffer);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not enumerate device extension properties for device, reason: %s", jfw_vk_error_msg(vk_res));
        return JFW_RESULT_VK_FAIL;
    }
    for (uint32_t i = 0; i < device_extensions && found_props < DEVICE_REQUIRED_COUNT; ++i)
    {
        const char* ext_name = ext_buffer[i].extensionName;
        for (uint32_t j = 0; j < DEVICE_REQUIRED_COUNT; ++j)
        {
            const char* req_name = DEVICE_REQUIRED_EXTENSIONS[j];
            const uint32_t len = DEVICE_REQUIRED_LENGTHS[j];
            if (match_extension_name(req_name, len, ext_name) != 0)
            {
                found_props += 1;
                break;
            }
        }
    }

    if (found_props < DEVICE_REQUIRED_COUNT)
    {
        score = -1;
        goto end;
    }

    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, NULL);
    assert(queue_count <= n_queue_buffer);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queue_buffer);
    //  Find graphics display queue
    int32_t gfx = -1, prs = -1;
    //  Check for combined support
    for (uint32_t i = 0; i < queue_count; ++i)
    {
        const VkQueueFamilyProperties* const props = queue_buffer + i;
        VkBool32 surface_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surface_support);
        if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT && surface_support)
        {
            gfx = (int32_t)i;
            prs = (int32_t)i;
            break;
        }
    }
    //  Check for separate support
    for (uint32_t i = 0; i < queue_count && (gfx == -1 || prs == -1); ++i)
    {
        const VkQueueFamilyProperties* const props = queue_buffer + i;
        if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            gfx = (int32_t)i;
            if (prs != -1) break;
        }
        VkBool32 surface_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surface_support);
        if (surface_support)
        {
            prs = (int32_t)i;
            if (gfx != -1) break;
        }
    }

    if (gfx == -1 || prs == -1)
    {
        score = -1;
        goto end;
    }

    //  Try and find a dedicated transfer queue
    int32_t trs = -1;
    for (uint32_t i = 0; i < queue_count; ++i)
    {
        const VkQueueFamilyProperties* const props = queue_buffer + i;
        if (!(props->queueFlags & (VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT)) && props->queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            trs = (int32_t)i;
            break;
        }
    }
    if (trs == -1)
    {
        //  According to Vulkan specifications, any graphics queue must support transfer operations
        trs = gfx;
    }

    //  Check for surface support with relation to swapchain
    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, NULL);
    if (!count)
    {
        score = -1;
        goto end;
    }
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, NULL);
    if (!count)
    {
        score = -1;
        goto end;
    }

    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceFeatures(device, &features);
    vkGetPhysicalDeviceProperties(device, &props);



    score = (int32_t)(1 + (features.geometryShader ? 10 : 0) + (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) + props.limits.maxImageDimension2D);


    *p_sample_flags = props.limits.framebufferColorSampleCounts;
    *p_queue_gfx = gfx;
    *p_queue_prs = prs;
    *p_queue_trs = trs;
    end:
    *p_score = score;
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

jfw_result jfw_platform_create(
        jfw_ctx* ctx, jfw_platform* platform, uint32_t w, uint32_t h, size_t title_len, const char* title, uint32_t n_frames_in_filght,
        int32_t fixed, jfw_color color)
{
    JDM_ENTER_FUNCTION;
    jfw_result result = JFW_RESULT_SUCCESS;
    assert(ctx);
    assert(platform);
    memset(platform, 0, sizeof(*platform));
    Window wnd;
    //  Xorg window initialization
    {
        XSetWindowAttributes wa = { 0 };
        wa.event_mask = (ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask | KeyPressMask |
                         KeyReleaseMask
                         | StructureNotifyMask | VisibilityChangeMask);
        wa.background_pixmap = None;

        wnd = XCreateWindow(
                ctx->dpy, ctx->root_window, 0, 0, w, h, 0, DefaultDepth(ctx->dpy, DefaultScreen(ctx->dpy)), 0,
                DefaultVisual(ctx->dpy, DefaultScreen(ctx->dpy)), CWColormap | CWEventMask | CWBackPixmap, &wa);
        if (!wnd)
        {
            result = JFW_RESULT_PLATFORM_NO_WND;
            JDM_ERROR("Failed creating platform window handle");
            JDM_LEAVE_FUNCTION;
            return result;
        }
        XSelectInput(ctx->dpy, wnd, wa.event_mask);
        jfw_color c = {
                .r = 0x1D,
                .g = 0x1D,
                .b = 0x1D,
                .a = 0xFF
        };
        XSetWindowBackground(ctx->dpy, wnd, (c.r << 16) | (c.g << 8) | (c.b << 0));
        XSelectInput(ctx->dpy, wnd, wa.event_mask);
        XSizeHints size_hints =
                {
                        .flags = PMaxSize|PMinSize,
                        .min_width = (int) w,
                        .min_height = (int) h,
                        .max_width = (int) w,
                        .max_height = (int) h,
                        .width_inc = 0,
                        .height_inc = 0,
                };
        XSetWMNormalHints(ctx->dpy, wnd, &size_hints);
        if (ctx->del_atom != None)
        {
            Atom atom[] = { ctx->del_atom };
            XSetWMProtocols(ctx->dpy, wnd, atom, 1);
        }

        platform->ctx = ctx;
        platform->hwnd = wnd;

        XTextProperty tp =
                {
                        .format = 8,
                        .encoding = XA_STRING,
                        .nitems = title_len,
                        .value = (unsigned char*) title,
                };
        XSetWMName(ctx->dpy, wnd, &tp);
//        XClearWindow(ctx->dpy, wnd);
    }

    jfw_window_vk_resources* const res = &platform->vk_res;
    res->n_frames_in_flight = n_frames_in_filght;
    //  Vulkan's initialization
    //  Create surface for the window
    VkResult vk_res;
    const jfw_vulkan_context* context = &ctx->vk_ctx;
    {
        VkXlibSurfaceCreateInfoKHR vk_surface_info =
                {
                        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
                        .dpy = ctx->dpy,
                        .window = platform->hwnd,
                        .pNext = NULL,
                };
        VkSurfaceKHR surface;
        vk_res = vkCreateXlibSurfaceKHR(context->instance, &vk_surface_info, NULL, &surface);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not create vulkan surface from window, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->surface = surface;
    }

    //  Find a good physical device, which can support the surface's format
    uint32_t i_gfx_queue; uint32_t i_prs_queue; uint32_t i_trs_queue;
    VkSampleCountFlagBits samples;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    {
        int32_t best_score = -1;
        VkExtensionProperties* prop_buffer;
        prop_buffer = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, context->max_device_extensions * sizeof(*prop_buffer));
        if (!prop_buffer)
        {
            JDM_ERROR("Failed allocating memory for device extensions");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        VkQueueFamilyProperties* queue_buffer;
        queue_buffer = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, context->max_device_queue_families * sizeof(*queue_buffer));
        if (!queue_buffer)
        {
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, prop_buffer);
            JDM_ERROR("Failed allocating memory for device extensions");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        for (uint32_t i = 0; i < context->n_physical_devices; ++i)
        {
            int32_t score = -1; int32_t gfx = -1; int32_t prs = -1; int32_t trs = -1; VkSampleCountFlagBits s;
            result = score_physical_device(
                    context->p_physical_devices[i], res->surface, &score, context->max_device_extensions, prop_buffer,
                    &gfx, &prs, &trs, context->max_device_queue_families, queue_buffer, &s);
            if (result == JFW_RESULT_SUCCESS && score > best_score)
            {
                best_score = score;
                i_gfx_queue = gfx;
                i_prs_queue = prs;
                i_trs_queue = trs;
                samples = s;
                physical_device = context->p_physical_devices[i];
            }
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, prop_buffer);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, queue_buffer);
        if (best_score == -1)
        {
            JDM_ERROR("Could not find a physical device which would support the bare minimum features required for Vulkan");
            result = JFW_RESULT_VK_FAIL;
            goto failed;
        }
        assert(physical_device != VK_NULL_HANDLE);

        const float priority = 1.0f;
        uint32_t queue_indices[] =
                {
                i_gfx_queue, i_prs_queue, i_trs_queue
                };
#define n_queues (sizeof(queue_indices) / sizeof(*queue_indices))
        VkDeviceQueueCreateInfo queue_create_info[n_queues] = {};
        uint32_t unique_queues[n_queues];
        uint32_t n_unique_queues = 0;
        for (uint32_t i = 0, j; i < n_queues; ++i)
        {
            const uint32_t index = queue_indices[i];
            for (j = 0; j < n_unique_queues; ++j)
            {
                if (index == unique_queues[j]) break;
            }
            if (j == n_unique_queues)
            {
                assert(n_unique_queues <= n_queues);
                unique_queues[j] = index;
                queue_create_info[j] =
                        (VkDeviceQueueCreateInfo){
                        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                        .queueFamilyIndex = index,
                        .queueCount = 1,
                        .pQueuePriorities = &priority,
                        };
                n_unique_queues += 1;
            }
        }
#undef  n_queues
        VkPhysicalDeviceFeatures features = {};
        VkDeviceCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                .queueCreateInfoCount = n_unique_queues,
                .pQueueCreateInfos = queue_create_info,
                .pEnabledFeatures = &features,
                .ppEnabledExtensionNames = DEVICE_REQUIRED_EXTENSIONS,
                .enabledExtensionCount = DEVICE_REQUIRED_COUNT,
                };
        VkDevice device = VK_NULL_HANDLE;
        vk_res = vkCreateDevice(physical_device, &create_info, NULL, &device);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not create Vulkan logical device interface, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->device = device;
        res->i_gfx_queue = i_gfx_queue;
        res->i_prs_queue = i_prs_queue;
        res->i_trs_queue = i_trs_queue;
        res->sample_flags = samples;

        vkGetDeviceQueue(res->device, i_gfx_queue, 0, &res->queue_gfx);
        vkGetDeviceQueue(res->device, i_prs_queue, 0, &res->queue_present);
        vkGetDeviceQueue(res->device, i_trs_queue, 0, &res->queue_transfer);
    }

    //  Create the swapchain
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    {
        res->physical_device = physical_device;
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, res->surface, &surface_capabilities);
        uint32_t sc_img_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount && sc_img_count > surface_capabilities.maxImageCount)
        {
            sc_img_count = surface_capabilities.maxImageCount;
        }
        res->n_images = sc_img_count;
        uint64_t max_buffer_size;
        uint32_t sf_count, pm_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, res->surface, &sf_count, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, res->surface, &pm_count, NULL);
        max_buffer_size = sf_count * sizeof(VkSurfaceFormatKHR);
        if (max_buffer_size < pm_count * sizeof(VkPresentModeKHR))
        {
            max_buffer_size = pm_count * sizeof(VkPresentModeKHR);
        }
        void* ptr_buffer = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, max_buffer_size);
        if (!ptr_buffer)
        {
            JDM_ERROR("Could not allocate memory for the buffer used for presentation mode/surface formats");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        VkSurfaceFormatKHR* const formats_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, res->surface, &sf_count, formats_buffer);
        surface_format = formats_buffer[0];
        for (uint32_t i = 0; i < sf_count; ++i)
        {
            const VkSurfaceFormatKHR* const format = formats_buffer + i;
            if ((format->format == VK_FORMAT_R8G8B8A8_SRGB || format->format == VK_FORMAT_B8G8R8A8_SRGB) && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format = *format;
                break;
            }
        }
        VkPresentModeKHR* const present_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, res->surface, &pm_count, present_buffer);
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (uint32_t i = 0; i < sf_count; ++i)
        {
            const VkPresentModeKHR * const mode = present_buffer + i;
            if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = *mode;
                break;
            }
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ptr_buffer);
        VkExtent2D extent = surface_capabilities.minImageExtent;
        if (extent.width != ~0u)
        {
            assert(extent.height != ~0u);
            XWindowAttributes attribs;
            XGetWindowAttributes(ctx->dpy, wnd, &attribs);
            extent.width = attribs.width;
            extent.height = attribs.height;
            if (extent.width > surface_capabilities.maxImageExtent.width)
            {
                extent.width = surface_capabilities.maxImageExtent.width;
            }
            else if (extent.width < surface_capabilities.minImageExtent.width)
            {
                extent.width = surface_capabilities.minImageExtent.width;
            }
            if (extent.height > surface_capabilities.maxImageExtent.height)
            {
                extent.height = surface_capabilities.maxImageExtent.height;
            }
            else if (extent.height < surface_capabilities.minImageExtent.height)
            {
                extent.height = surface_capabilities.minImageExtent.height;
            }
        }
        uint32_t queue_indices[] =
                {
                    i_gfx_queue, i_prs_queue,
                };
        VkSwapchainCreateInfoKHR create_info =
                {
                .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .surface = res->surface,
                .minImageCount = sc_img_count,
                .imageFormat = surface_format.format,
                .imageColorSpace = surface_format.colorSpace,
                .imageArrayLayers = 1,
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode = present_mode,
                .clipped = VK_TRUE,
                .imageExtent = extent,
                };
        if (i_gfx_queue == i_prs_queue)
        {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = NULL;
        }
        else
        {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(*queue_indices);
            create_info.pQueueFamilyIndices = queue_indices;
        }

        VkSwapchainKHR sc;
        vk_res = vkCreateSwapchainKHR(res->device, &create_info, NULL, &sc);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not create a vulkan swapchain, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->swapchain = sc;
        res->extent = extent;
        res->surface_format = surface_format;
    }

    //  Create swapchain images
    {
        uint32_t n_sc_images;
        vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, NULL);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        VkImageView* views = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_sc_images * sizeof(*views));
        if (!views)
        {
            JDM_ERROR("Could not allocate memory for the views");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        {
            VkImage* images = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_sc_images * sizeof(*images));
            if (!images)
            {
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, views);
                JDM_ERROR("Could not allocate memory for swapchain images");
                result = JFW_RESULT_BAD_ALLOC;
                goto failed;
            }
            vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, images);
            if (vk_res != VK_SUCCESS)
            {
                result = JFW_RESULT_VK_FAIL;
                JDM_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, views);
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, images);
                goto failed;
            }

            VkImageViewCreateInfo create_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                            .viewType = VK_IMAGE_VIEW_TYPE_2D,
                            .format = surface_format.format,

                            .components.a = VK_COMPONENT_SWIZZLE_A,
                            .components.b = VK_COMPONENT_SWIZZLE_B,
                            .components.g = VK_COMPONENT_SWIZZLE_G,
                            .components.r = VK_COMPONENT_SWIZZLE_R,

                            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                            .subresourceRange.baseMipLevel = 0,
                            .subresourceRange.levelCount = 1,
                            .subresourceRange.baseArrayLayer = 0,
                            .subresourceRange.layerCount = 1,
                    };

            for (uint32_t i = 0; i < n_sc_images; ++i)
            {
                create_info.image = images[i];
                vk_res = vkCreateImageView(res->device, &create_info, NULL, views + i);
                if (vk_res != VK_SUCCESS)
                {
                    result = JFW_RESULT_VK_FAIL;
                    for (uint32_t j = 0; j < i; ++j)
                    {
                        vkDestroyImageView(res->device, views[j], NULL);
                    }
                    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, views);
                    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, images);
                    goto failed;
                }
            }
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, images);
        }
        res->views = views;
    }

    //  Now make the command pool (only the gfx queue needs it)
    {
        VkCommandPool cmd_pool;
        VkCommandPoolCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex = i_gfx_queue,
                };
        vk_res = vkCreateCommandPool(res->device, &create_info, NULL, &cmd_pool);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not create command pool for the gfx queue, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->cmd_pool = cmd_pool;
    }

    //  Now create cmd buffer for the gfx commands
    {
        VkCommandBuffer* cmd_buffers;
        VkSemaphore* sem_img;
        VkSemaphore* sem_prs;
        VkFence* fences;
        cmd_buffers = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_frames_in_filght * sizeof(*cmd_buffers));
        if (!cmd_buffers)
        {
            JDM_ERROR("Failed allocating memory for command buffers array");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        sem_img = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_frames_in_filght * sizeof(*sem_img));
        if (JFW_RESULT_SUCCESS !=(result))
        {
            JDM_ERROR("Failed allocating memory for image semaphore array");
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, cmd_buffers);
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        sem_prs = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_frames_in_filght * sizeof(*sem_prs));
        if (JFW_RESULT_SUCCESS !=(result))
        {
            JDM_ERROR("Failed allocating memory for present semaphore array");
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, cmd_buffers);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_img);
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        fences = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_frames_in_filght * sizeof(*fences));
        if (JFW_RESULT_SUCCESS !=(result))
        {
            JDM_ERROR("Failed allocating memory for fences array");
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, cmd_buffers);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_img);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_prs);
            goto failed;
        }
        VkCommandBufferAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandBufferCount = n_frames_in_filght + 1,
                .commandPool = res->cmd_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                };
        vk_res = vkAllocateCommandBuffers(res->device, &alloc_info, cmd_buffers);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not allocate command buffers for %u frames in flight + transfer", n_frames_in_filght);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, cmd_buffers);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_img);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_prs);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, fences);
            goto failed;
        }
        res->cmd_buffers = cmd_buffers;
        for (uint32_t i = 0; i < n_frames_in_filght; ++i)
        {
            VkSemaphoreCreateInfo sem_create_info =
                    {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    };
            vk_res = vkCreateSemaphore(res->device, &sem_create_info, NULL, sem_img + i);
            if (vk_res != VK_SUCCESS)
            {
                result = JFW_RESULT_VK_FAIL;
                JDM_ERROR("Failed creating %u semaphores, reason: %s", n_frames_in_filght, jfw_vk_error_msg(vk_res));
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(res->device, sem_img[j], NULL);
                }
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_img);
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_prs);
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, fences);
                goto failed;
            }
        }
        res->sem_img_available = sem_img;
        for (uint32_t i = 0; i < n_frames_in_filght; ++i)
        {
            VkSemaphoreCreateInfo sem_create_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    };
            vk_res = vkCreateSemaphore(res->device, &sem_create_info, NULL, sem_prs + i);
            if (vk_res != VK_SUCCESS)
            {
                result = JFW_RESULT_VK_FAIL;
                JDM_ERROR("Failed creating %u semaphores, reason: %s", n_frames_in_filght, jfw_vk_error_msg(vk_res));
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(res->device, sem_prs[j], NULL);
                }
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, sem_prs);
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, fences);
                goto failed;
            }
        }
        res->sem_present = sem_prs;
        for (uint32_t i = 0; i < n_frames_in_filght; ++i)
        {
            VkFenceCreateInfo create_info =
                    {
                    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                    };
            vk_res = vkCreateFence(res->device, &create_info, NULL, fences + i);
            if (vk_res != VK_SUCCESS)
            {
                result = JFW_RESULT_VK_FAIL;
                JDM_ERROR("Failed creating %u fences, reason: %s", n_frames_in_filght, jfw_vk_error_msg(vk_res));
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroyFence(res->device, fences[j], NULL);
                }
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, fences);
                goto failed;
            }
        }
        res->swap_fences = fences;
    }

    unsigned long pixel = ((unsigned long)color.a << (24)) | ((unsigned long)color.r << (16)) | ((unsigned long)color.g << (8)) | ((unsigned long)color.b << (0));
    XSetWindowBackground(ctx->dpy, wnd, pixel);
    JDM_LEAVE_FUNCTION;
    return result;



failed:
    if (res->swap_fences)
    {
        for (uint32_t j = 0; j < n_frames_in_filght; ++j)
        {
            vkDestroyFence(res->device, res->swap_fences[j], NULL);
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, res->swap_fences);
    }
    if (res->cmd_buffers)
    {
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, res->cmd_buffers);
    }
    if (res->sem_present)
    {
        for (uint32_t j = 0; j < n_frames_in_filght; ++j)
        {
            vkDestroySemaphore(res->device, res->sem_present[j], NULL);
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, res->sem_present);
    }
    if (res->sem_img_available)
    {
        for (uint32_t j = 0; j < n_frames_in_filght; ++j)
        {
            vkDestroySemaphore(res->device, res->sem_img_available[j], NULL);
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, res->sem_img_available);
    }
    if (res->cmd_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(res->device, res->cmd_pool, NULL);
    }
    if (res->views)
    {
        for (uint32_t i = 0; i < res->n_images; ++i)
        {
            vkDestroyImageView(res->device, res->views[i], NULL);
            res->views[i] = VK_NULL_HANDLE;
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, res->views);
    }
    if (res->swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(res->device, res->swapchain, NULL);
    }
    if (res->device != VK_NULL_HANDLE)
    {
        vkDestroyDevice(res->device, NULL);
    }
    if (res->surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(context->instance, res->surface, NULL);
    }
    XDestroyWindow(ctx->dpy, wnd);
    memset(platform, 0, sizeof(*platform));
    JDM_LEAVE_FUNCTION;
    return result;
}

jfw_result jfw_platform_destroy(jfw_platform* platform)
{
    JDM_ENTER_FUNCTION;
    jfw_ctx* ctx = platform->ctx;
    vkDeviceWaitIdle(platform->vk_res.device);
    for (uint32_t j = 0; j < platform->vk_res.n_frames_in_flight; ++j)
    {
        vkDestroyFence(platform->vk_res.device, platform->vk_res.swap_fences[j], NULL);
    }
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, platform->vk_res.swap_fences);
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, platform->vk_res.cmd_buffers);
    for (uint32_t j = 0; j < platform->vk_res.n_frames_in_flight; ++j)
    {
        vkDestroySemaphore(platform->vk_res.device, platform->vk_res.sem_present[j], NULL);
    }
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, platform->vk_res.sem_present);
    for (uint32_t j = 0; j < platform->vk_res.n_frames_in_flight; ++j)
    {
        vkDestroySemaphore(platform->vk_res.device, platform->vk_res.sem_img_available[j], NULL);
    }
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, platform->vk_res.sem_img_available);
    vkDestroyCommandPool(platform->vk_res.device, platform->vk_res.cmd_pool, NULL);
    for (uint32_t i = 0; i < platform->vk_res.n_images; ++i)
    {
        vkDestroyImageView(platform->vk_res.device, platform->vk_res.views[i], NULL);
        platform->vk_res.views[i] = VK_NULL_HANDLE;
    }
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, platform->vk_res.views);
    vkDestroySwapchainKHR(platform->vk_res.device, platform->vk_res.swapchain, NULL);
    vkDestroyDevice(platform->vk_res.device, NULL);
    vkDestroySurfaceKHR(ctx->vk_ctx.instance, platform->vk_res.surface, NULL);
    XDestroyWindow(ctx->dpy, platform->hwnd);
    memset(platform, 0, sizeof(*platform));
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

jfw_result jfw_context_add_window(jfw_ctx* ctx, jfw_window* p_window)
{
    JDM_ENTER_FUNCTION;
    jfw_result result = JFW_RESULT_SUCCESS;
    if (ctx->wnd_count == ctx->wnd_capacity)
    {
        const uint32_t new_capacity = ctx->wnd_capacity << 1;
        jfw_window** new_array = ctx->allocator_callbacks.realloc(ctx->allocator_callbacks.state, ctx->wnd_array, new_capacity * sizeof(p_window));
        if (!new_array)
        {
            JDM_ERROR("Failed reallocating memory for context's window handles");
            JDM_LEAVE_FUNCTION;
            return JFW_RESULT_BAD_REALLOC;
        }
        ctx->wnd_array = new_array;
        memset(ctx->wnd_array + ctx->wnd_count, 0, sizeof(p_window) * (new_capacity - ctx->wnd_capacity));
        ctx->wnd_capacity = new_capacity;
    }
    ctx->wnd_array[ctx->wnd_count++] = p_window;

    JDM_LEAVE_FUNCTION;
    return result;
}

jfw_result jfw_context_remove_window(jfw_ctx* ctx, jfw_window* p_window)
{
    JDM_ENTER_FUNCTION;
    for (uint32_t i = 0; i < ctx->wnd_count; ++i)
    {
        if (ctx->wnd_array[i] == p_window)
        {
            memmove(ctx->wnd_array + i, ctx->wnd_array + i + 1, sizeof(p_window) * (ctx->wnd_capacity - i - 1));
            ctx->wnd_array[--ctx->wnd_count] = 0;
            JDM_LEAVE_FUNCTION;
            return JFW_RESULT_SUCCESS;
        }
    }
    JDM_ERROR("Failed removing window %p from context since it was not found in its array", p_window);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_INVALID_WINDOW;
}

static const char* const funi_ptr = "What sound does a rubber aircraft make when you hit it?\n"
                                    "Boeing";

static void* def_alloc(void* state, uint64_t size)
{
    assert(funi_ptr == state);
    return malloc(size);
}

static void* def_realloc(void* state, void* ptr, uint64_t new_size)
{
    assert(funi_ptr == state);
    return realloc(ptr, new_size);
}

static void def_free(void* state, void* ptr)
{
    assert(state == funi_ptr);
    free(ptr);
}

static const jfw_allocator_callbacks DEFAULT_ALLOCATORS =
        {
            .state = (void*)funi_ptr,
            .alloc = def_alloc,
            .realloc = def_realloc,
            .free = def_free,
        };

jfw_result jfw_context_create(
        jfw_ctx** p_ctx, VkAllocationCallbacks* vk_alloc_callbacks, const jfw_allocator_callbacks* allocator_callbacks)
{
    if (!allocator_callbacks)
    {
        allocator_callbacks = &DEFAULT_ALLOCATORS;
    }
    JDM_ENTER_FUNCTION;
    jfw_result result = JFW_RESULT_SUCCESS;
    jfw_ctx* ctx = allocator_callbacks->alloc(allocator_callbacks->state, sizeof(*ctx));
    if (!ctx)
    {
        JDM_ERROR("Failed allocating memory for context");
        JDM_LEAVE_FUNCTION;
        return JFW_RESULT_BAD_ALLOC;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->allocator_callbacks = *allocator_callbacks;
    ctx->wnd_array = allocator_callbacks->alloc(allocator_callbacks->state, 8 * sizeof(*ctx->wnd_array));
    if (!ctx->wnd_array)
    {
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx);
        JDM_ERROR("Failed allocating memory for context's window array");
        JDM_LEAVE_FUNCTION;
        return JFW_RESULT_BAD_ALLOC;
    }
    ctx->wnd_capacity = 8;

    Display* dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx->wnd_array);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx);
        JDM_ERROR("Failed opening XLib display");
        JDM_LEAVE_FUNCTION;
        return JFW_RESULT_CTX_NO_DPY;
    }
    ctx->dpy = dpy;
    ctx->screen = DefaultScreen(dpy);
    ctx->root_window = RootWindow(dpy, ctx->screen);
    char* atom_names[] =
            {
                    "WM_DELETE_WINDOW", //  delete atom
                    "UTF8_STRING",      //  UTF-8 string atome
            };
    Atom atoms[2];
    XInternAtoms(dpy, atom_names, 2, False, atoms);
    ctx->del_atom = atoms[0];
    ctx->utf8_atom = atoms[1];
    XSetLocaleModifiers("");
    XIM im = XOpenIM(dpy, NULL, NULL, NULL);
    if (!im)
    {
        XSetLocaleModifiers("@im=none");
        im = XOpenIM(dpy, NULL, NULL, NULL);
    }
    if (!im)
    {
        XCloseDisplay(dpy);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx->wnd_array);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx);
        JDM_ERROR("Failed opening input method for XLib");
        JDM_LEAVE_FUNCTION;
        return JFW_RESULT_CTX_NO_IM;
    }
    XIMStyle im_style = XIMStatusNone|XIMPreeditNone;
    ctx->input_ctx = XCreateIC(im, XNPreeditAttributes, XIMPreeditNone, XNStatusAttributes, XIMStatusNone, XNInputStyle, im_style, NULL);
    if (!ctx->input_ctx)
    {
        XCloseIM(im);
        XCloseDisplay(dpy);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx->wnd_array);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx);
        JDM_ERROR("Failed creating input context for XLib");
        JDM_LEAVE_FUNCTION;
        return JFW_RESULT_CTX_NO_IC;
    }
    XSetICFocus(ctx->input_ctx);

    result = jfw_vulkan_context_create(ctx, &ctx->vk_ctx, "jfw_context", 1, vk_alloc_callbacks);
    if (result != JFW_RESULT_SUCCESS)
    {
        XDestroyIC(ctx->input_ctx);
        XCloseIM(im);
        XCloseDisplay(dpy);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx->wnd_array);
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx);
        JDM_ERROR("Failed creating input context for XLib");
        JDM_LEAVE_FUNCTION;
        return result;
    }

    *p_ctx = ctx;
    JDM_LEAVE_FUNCTION;
    return result;
}

jfw_result jfw_context_destroy(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
    for (uint32_t i = ctx->wnd_count; i != 0; --i)
    {
        jfw_window_destroy(ctx, ctx->wnd_array[i - 1]);
    }
    XSetICFocus(NULL);
    XCloseDisplay(ctx->dpy);
    jfw_vulkan_context_destroy(ctx, &ctx->vk_ctx);
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx->wnd_array);
    ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ctx);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_mouse_button_press(jfw_ctx* ctx, XEvent* ptr, jfw_window* this)
{
    JDM_ENTER_FUNCTION;
    XButtonPressedEvent* const e = &ptr->xbutton;

    if (ctx->mouse_state == 0)
    {
        //  New press on a window
        XSetICFocus(ctx->input_ctx);
        XSetInputFocus(ctx->dpy, this->platform.hwnd, RevertToNone, CurrentTime);   //  This generates FocusIn event
    }
//    XGrabPointer(ctx->dpy, this->platform.hwnd, False, PointerMotionMask|EnterWindowMask|LeaveWindowMask|ButtonMotionMask,GrabModeAsync,GrabModeAsync, None, None, CurrentTime);
    ctx->mouse_state |= (1 << (e->button - Button1));

    int32_t x = e->x;
    int32_t y = e->y;


    jfw_result res = JFW_RESULT_SUCCESS;
    if (this->functions.mouse_button_double_press && ctx->last_button_id == e->button && (e->time - ctx->last_button_time) < 500)
    {
        res = this->functions.mouse_button_double_press(this, x, y, e->button, e->state);
        ctx->last_button_id = -1;
    }
    else
    {
        if (this->functions.mouse_button_press)
        {
            res = this->functions.mouse_button_press(this, x, y, e->button, e->state);
        }
        ctx->last_button_id = e->button;
    }
    ctx->last_button_time = e->time;

    JDM_LEAVE_FUNCTION;
    return res;
}

static jfw_result handle_mouse_button_release(jfw_ctx* ctx, XEvent* ptr, jfw_window* this)
{
    JDM_ENTER_FUNCTION;
    XButtonPressedEvent* const e = &ptr->xbutton;
    int32_t x = e->x;
    int32_t y = e->y;
    ctx->mouse_state &= ~(1 << (e->button - Button1));
    if (!ctx->mouse_state)
    {
        XUngrabPointer(ctx->dpy, 0);
    }

    if (this->functions.mouse_button_release)
    {
        jfw_result res = this->functions.mouse_button_release(this, x, y, e->button, e->state);
        JDM_LEAVE_FUNCTION;
        return res;
    }

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_motion_notify(jfw_ctx* ctx, XEvent* ptr, jfw_window* this)
{
    JDM_ENTER_FUNCTION;
    XMotionEvent* const e = &ptr->xmotion;
    int32_t x = e->x;
    int32_t y = e->y;

    if (this->functions.mouse_motion)
    {
        jfw_result res = this->functions.mouse_motion(this, x, y, e->state);
        JDM_LEAVE_FUNCTION;
        return res;
    }

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_client_message(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XClientMessageEvent* const e = &ptr->xclient;
    if (e->data.l[0] == ctx->del_atom)
    {
        //  Request to close the window
        assert(e->window == wnd->platform.hwnd);
        if (wnd->functions.on_close)
        {
            wnd->functions.on_close(wnd);
        }
        else
        {
            jfw_result res = jfw_window_destroy(ctx, wnd);
            JDM_LEAVE_FUNCTION;
            return res;
        }
    }
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_focus_in(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_result res;
    if (wnd->functions.keyboard_focus_get)
    {
        wnd->functions.keyboard_focus_get(wnd);
    }

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_focus_out(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    jfw_result res;
    if (wnd->functions.keyboard_focus_lose)
    {
        wnd->functions.keyboard_focus_lose(wnd);
    }

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_kbd_mapping_change(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XRefreshKeyboardMapping(&ptr->xmapping);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_keyboard_button_down(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XKeyPressedEvent* const e = &ptr->xkey;
    char buffer[8];
    KeySym ks; Status stat;
    const int len = Xutf8LookupString(ctx->input_ctx, e, buffer, sizeof(buffer), &ks, &stat);

    if (wnd->functions.button_down && (stat == XLookupBoth || stat == XLookupKeySym))
    {
        jfw_result res = wnd->functions.button_down(wnd, ks);
        JDM_LEAVE_FUNCTION;
        return res;
    }
    if (wnd->functions.char_input && (stat == XLookupBoth || stat == XLookupChars))
    {
        jfw_result res = wnd->functions.char_input(wnd, buffer);
        JDM_LEAVE_FUNCTION;
        return res;
    }


    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_keyboard_button_up(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XKeyReleasedEvent* const e = &ptr->xkey;
    if (wnd->functions.button_up)
    {
        KeySym ks = XLookupKeysym(e, 0);
        if (ks != NoSymbol)
        {

            jfw_result res = wnd->functions.button_up(wnd, ks);
            JDM_LEAVE_FUNCTION;
            return res;
        }
    }

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

static jfw_result handle_config_notify(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XConfigureEvent* const e = &ptr->xconfigure;
    jfw_result res = JFW_RESULT_SUCCESS;
    if ((uint32_t)e->width != wnd->w || (uint32_t)e->height != wnd->h)
    {
        if (wnd->functions.on_resize)
        {
            res = wnd->functions.on_resize(wnd, wnd->w, wnd->h, e->width, e->height);
        }
        wnd->w = e->width; wnd->h = e->height;
    }

    JDM_LEAVE_FUNCTION;
    return res;
}

static jfw_result handle_map_notify(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XMapEvent* const e = &ptr->xmap;
    jfw_result res = JFW_RESULT_SUCCESS;
    wnd->redraw += 1;

    JDM_LEAVE_FUNCTION;
    return res;
}

static jfw_result handle_visibility_notify(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JDM_ENTER_FUNCTION;
    XVisibilityEvent* const e = &ptr->xvisibility;
    jfw_result res = JFW_RESULT_SUCCESS;
    wnd->redraw += 1;

    JDM_LEAVE_FUNCTION;
    return res;
}

static jfw_result(*XEVENT_HANDLERS[LASTEvent])(jfw_ctx* ctx, XEvent* e, jfw_window* wnd) =
        {
        [ButtonPress] = handle_mouse_button_press,
        [ButtonRelease] = handle_mouse_button_release,
        [ClientMessage] = handle_client_message,
        [MotionNotify] = handle_motion_notify,
        [FocusIn] = handle_focus_in,
        [FocusOut] = handle_focus_out,
        [KeyPress] = handle_keyboard_button_down,
        [KeyRelease] = handle_keyboard_button_up,
        [MappingNotify] = handle_kbd_mapping_change,
        [ConfigureNotify] = handle_config_notify,
        [MapNotify] = handle_map_notify,
        [VisibilityNotify] = handle_visibility_notify,
        };

static const char* const XEVENT_NAMES[LASTEvent] =
        {
        [KeyPress] = "KeyPress",
        [KeyRelease] = "KeyRelease",
        [ButtonPress] = "ButtonPress",
        [ButtonRelease] = "ButtonRelease",
        [MotionNotify] = "MotionNotify",
        [EnterNotify] = "EnterNotify",
        [LeaveNotify] = "LeaveNotify",
        [FocusIn] = "FocusIn",
        [FocusOut] = "FocusOut",
        [KeymapNotify] = "KeymapNotify",
        [Expose] = "Expose",
        [GraphicsExpose] = "GraphicsExpose",
        [NoExpose] = "NoExpose",
        [VisibilityNotify] = "VisibilityNotify",
        [CreateNotify] = "CreateNotify",
        [DestroyNotify] = "DestroyNotify",
        [UnmapNotify] = "UnmapNotify",
        [MapNotify] = "MapNotify",
        [MapRequest] = "MapRequest",
        [ReparentNotify] = "ReparentNotify",
        [ConfigureNotify] = "ConfigureNotify",
        [ConfigureRequest] = "ConfigureRequest",
        [GravityNotify] = "GravityNotify",
        [ResizeRequest] = "ResizeRequest",
        [CirculateNotify] = "CirculateNotify",
        [CirculateRequest] = "CirculateRequest",
        [PropertyNotify] = "PropertyNotify",
        [SelectionClear] = "SelectionClear",
        [SelectionRequest] = "SelectionRequest",
        [SelectionNotify] = "SelectionNotify",
        [ColormapNotify] = "ColormapNotify",
        [ClientMessage] = "ClientMessage",
        [MappingNotify] = "MappingNotify",
        [GenericEvent] = "GenericEvent",
        };

jfw_result jfw_context_process_events(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
    if (!ctx->wnd_count)
    {
        JDM_LEAVE_FUNCTION;
        return JFW_RESULT_CTX_NO_WINDOWS;
    }
    XEvent e;
    if (XPending(ctx->dpy))
    {
        XNextEvent(ctx->dpy, &e);
//        printf("Got event of type %s\n", XEVENT_NAMES[e.type]);
        jfw_result (* const handler)(jfw_ctx* ctx, XEvent* e, jfw_window* wnd) = XEVENT_HANDLERS[e.type];
        if (handler)
        {
            if (e.type == MappingNotify)
            {
                handler(ctx, &e, NULL);
            }
            else for (uint32_t i = 0; i < ctx->wnd_count; ++i)
            {
                jfw_window* wnd = ctx->wnd_array[i];
                if (wnd->platform.hwnd == e.xany.window)
                {
                    jfw_result res = handler(ctx, &e, wnd);
                    JDM_LEAVE_FUNCTION;
                    return res;
                }
            }
        }
    }
//    XFlush(ctx->dpy);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

int jfw_context_status(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
    int status = ctx->status;
    JDM_LEAVE_FUNCTION;
    return status;
}

int jfw_context_set_status(jfw_ctx* ctx, int status)
{
    JDM_ENTER_FUNCTION;
    const int prev_status = ctx->status;
    ctx->status = status;
    JDM_LEAVE_FUNCTION;
    return prev_status;
}

static int map_predicate(Display* dpy, XEvent* e, XPointer ptr)
{
    const Window wnd = (Window)ptr;
    return dpy && e && e->type == MapNotify && e->xmap.window == wnd;

}

jfw_result jfw_platform_show(jfw_ctx* ctx, jfw_platform* wnd)
{
    JDM_ENTER_FUNCTION;
    XMapWindow(ctx->dpy, wnd->hwnd);
    XEvent unused;
    XPeekIfEvent(ctx->dpy, &unused, map_predicate, (XPointer)wnd->hwnd);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

jfw_result jfw_platform_hide(jfw_ctx* ctx, jfw_platform* wnd)
{
    JDM_ENTER_FUNCTION;
    XUnmapWindow(ctx->dpy, wnd->hwnd);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

jfw_result jfw_platform_draw_bind(jfw_ctx* ctx, jfw_platform* wnd)
{
    JDM_ENTER_FUNCTION;
    const int res = True;
//    const int res = glXMakeCurrent(ctx->dpy, wnd->glw, wnd->gl_ctx);
    JDM_LEAVE_FUNCTION;
    return res == True ? JFW_RESULT_SUCCESS : JFW_RESULT_PLATFORM_NO_BIND;
}

jfw_result jfw_platform_unbind(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
//    glXMakeCurrent(ctx->dpy, None, None);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

uint32_t jfw_context_window_count(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
    uint32_t count = ctx->wnd_count;
    JDM_LEAVE_FUNCTION;
    return count;
}

jfw_result jfw_context_wait_for_events(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
    XEvent unused;
    XPeekEvent(ctx->dpy, &unused);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

int jfw_context_has_events(jfw_ctx* ctx)
{
    JDM_ENTER_FUNCTION;
    int pending = XPending(ctx->dpy);
    JDM_LEAVE_FUNCTION;
    return pending;
}

jfw_result jfw_platform_clear_window(jfw_ctx* ctx, jfw_platform* wnd)
{
    JDM_ENTER_FUNCTION;
    XClearWindow(ctx->dpy, wnd->hwnd);
    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
}

jfw_window_vk_resources* jfw_window_get_vk_resources(jfw_window* p_window)
{
    return &p_window->platform.vk_res;
}

static inline jfw_result create_swapchain(jfw_ctx* ctx, jfw_platform* plt, jfw_window_vk_resources* res)
{
    jfw_result result;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkResult vk_res;
    {
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(res->physical_device, res->surface, &surface_capabilities);
        uint32_t sc_img_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount && sc_img_count > surface_capabilities.maxImageCount)
        {
            sc_img_count = surface_capabilities.maxImageCount;
        }
        res->n_images = sc_img_count;
        uint64_t max_buffer_size;
        uint32_t sf_count, pm_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(res->physical_device, res->surface, &sf_count, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(res->physical_device, res->surface, &pm_count, NULL);
        max_buffer_size = sf_count * sizeof(VkSurfaceFormatKHR);
        if (max_buffer_size < pm_count * sizeof(VkPresentModeKHR))
        {
            max_buffer_size = pm_count * sizeof(VkPresentModeKHR);
        }
        void* ptr_buffer = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, max_buffer_size);
        if (!ptr_buffer)
        {
            JDM_ERROR("Could not allocate memory for the buffer used for presentation mode/surface formats");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        VkSurfaceFormatKHR* const formats_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfaceFormatsKHR(res->physical_device, res->surface, &sf_count, formats_buffer);
        surface_format = formats_buffer[0];
        for (uint32_t i = 0; i < sf_count; ++i)
        {
            const VkSurfaceFormatKHR* const format = formats_buffer + i;
            if ((format->format == VK_FORMAT_R8G8B8A8_SRGB || format->format == VK_FORMAT_B8G8R8A8_SRGB) && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                surface_format = *format;
                break;
            }
        }
        VkPresentModeKHR* const present_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfacePresentModesKHR(res->physical_device, res->surface, &pm_count, present_buffer);
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
        for (uint32_t i = 0; i < sf_count; ++i)
        {
            const VkPresentModeKHR * const mode = present_buffer + i;
            if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = *mode;
                break;
            }
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, ptr_buffer);
        VkExtent2D extent = surface_capabilities.minImageExtent;
        if (extent.width != ~0u)
        {
            assert(extent.height != ~0u);
            XWindowAttributes attribs;
            XGetWindowAttributes(ctx->dpy, plt->hwnd, &attribs);
            extent.width = attribs.width;
            extent.height = attribs.height;
            if (extent.width > surface_capabilities.maxImageExtent.width)
            {
                extent.width = surface_capabilities.maxImageExtent.width;
            }
            else if (extent.width < surface_capabilities.minImageExtent.width)
            {
                extent.width = surface_capabilities.minImageExtent.width;
            }
            if (extent.height > surface_capabilities.maxImageExtent.height)
            {
                extent.height = surface_capabilities.maxImageExtent.height;
            }
            else if (extent.height < surface_capabilities.minImageExtent.height)
            {
                extent.height = surface_capabilities.minImageExtent.height;
            }
        }
        uint32_t queue_indices[] =
                {
                        res->i_gfx_queue, res->i_prs_queue, res->i_trs_queue
                };
        VkSwapchainCreateInfoKHR create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                        .surface = res->surface,
                        .minImageCount = sc_img_count,
                        .imageFormat = surface_format.format,
                        .imageColorSpace = surface_format.colorSpace,
                        .imageArrayLayers = 1,
                        .imageExtent = extent,
                        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
                        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                        .presentMode = present_mode,
                        .clipped = VK_TRUE,
                };
        if (res->i_gfx_queue == res->i_prs_queue && res->i_gfx_queue == res->i_trs_queue)
        {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices = NULL;
        }
        else
        {
            create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = sizeof(queue_indices) / sizeof(*queue_indices);
            create_info.pQueueFamilyIndices = queue_indices;
        }

        VkSwapchainKHR sc;
        vk_res = vkCreateSwapchainKHR(res->device, &create_info, NULL, &sc);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not create a vulkan swapchain, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->swapchain = sc;
        res->extent = extent;
        res->surface_format = surface_format;
    }
    return JFW_RESULT_SUCCESS;
    failed:
    return result;
}

static inline jfw_result create_swapchain_img_views(jfw_ctx* ctx, jfw_window_vk_resources* res)
{
    jfw_result result;
    VkResult vk_res;
    uint32_t n_sc_images;
    vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, NULL);
    if (vk_res != VK_SUCCESS)
    {
        result = JFW_RESULT_VK_FAIL;
        JDM_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
        goto failed;
    }
    assert(n_sc_images == res->n_images);
    VkImageView* views = res->views;
//    result = jfw_calloc(n_sc_images, sizeof(*views), &views);
//    if (JFW_RESULT_SUCCESS !=(result))
//    {
//        JDM_ERROR("Could not allocate memory for the views");
//        goto failed;
//    }
    {
        VkImage* images = ctx->allocator_callbacks.alloc(ctx->allocator_callbacks.state, n_sc_images * sizeof(*images));
        if (!images)
        {
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, views);
            JDM_ERROR("Could not allocate memory for swapchain images");
            result = JFW_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, images);
        if (vk_res != VK_SUCCESS)
        {
            result = JFW_RESULT_VK_FAIL;
            JDM_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
//            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, views);
            ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, images);
            goto failed;
        }

        VkImageViewCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = res->surface_format.format,

                        .components.a = VK_COMPONENT_SWIZZLE_A,
                        .components.b = VK_COMPONENT_SWIZZLE_B,
                        .components.g = VK_COMPONENT_SWIZZLE_G,
                        .components.r = VK_COMPONENT_SWIZZLE_R,

                        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .subresourceRange.baseMipLevel = 0,
                        .subresourceRange.levelCount = 1,
                        .subresourceRange.baseArrayLayer = 0,
                        .subresourceRange.layerCount = 1,
                };

        for (uint32_t i = 0; i < n_sc_images; ++i)
        {
            create_info.image = images[i];
            vk_res = vkCreateImageView(res->device, &create_info, NULL, views + i);
            if (vk_res != VK_SUCCESS)
            {
                result = JFW_RESULT_VK_FAIL;
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroyImageView(res->device, views[j], NULL);
                }
                ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, images);
                goto failed;
            }
        }
        ctx->allocator_callbacks.free(ctx->allocator_callbacks.state, images);
    }
//    res->views = views;
    return JFW_RESULT_SUCCESS;

failed:
    return result;
}

jfw_result jfw_window_update_swapchain(jfw_window* p_window)
{
    JDM_ENTER_FUNCTION;
    jfw_window_vk_resources* const this = &p_window->platform.vk_res;
    vkDeviceWaitIdle(this->device);

    //  Cleanup current swapchain
    {
        for (uint32_t i = 0; i < this->n_images; ++i)
        {
            vkDestroyImageView(this->device, this->views[i], NULL);
        }
        vkDestroySwapchainKHR(this->device, this->swapchain, NULL);
    }


    jfw_result result = create_swapchain(p_window->ctx, &p_window->platform, &p_window->platform.vk_res);
    if (JFW_RESULT_SUCCESS !=(result))
    {
        JDM_ERROR("Could not recreate swapchain");
        goto failed;
    }

    //  Create swapchain images
    result = create_swapchain_img_views(p_window->ctx, &p_window->platform.vk_res);
    if (JFW_RESULT_SUCCESS !=(result))
    {
        JDM_ERROR("Could not recreate image views");
        goto failed;
    }

    JDM_LEAVE_FUNCTION;
    return JFW_RESULT_SUCCESS;
    failed:
    JDM_LEAVE_FUNCTION;
    return result;
}
