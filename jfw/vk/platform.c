//
// Created by jan on 16.1.2023.
//

#include "platform.h"
#include "../error_system/error_stack.h"
#include "../window.h"
#include "../widget-base.h"
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

static const u64 REQUIRED_LAYERS_LENGTHS[] =
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
static const u64 REQUIRED_EXTENSIONS_LENGTHS[] =
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
static const u64 DEVICE_REQUIRED_LENGTHS[] =
        {
                sizeof(VK_KHR_SWAPCHAIN_EXTENSION_NAME) - 1,
        };
#define DEVICE_REQUIRED_COUNT (sizeof(DEVICE_REQUIRED_EXTENSIONS) / sizeof(*DEVICE_REQUIRED_EXTENSIONS))


static inline VkBool32 match_extension_name(const char* const extension_name, const u64 extension_name_len, const char* str_to_check)
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
    u64 v = strtoull(str_to_check, &end_ptr, 10);


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
    u32 cat_buffer_usage = 0;
    for (u32 i = 0; i < (sizeof(TYPE_FLAG_BIT_VALUES)/sizeof(*TYPE_FLAG_BIT_VALUES)); ++i)
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
        JFW_ERROR("Vulkan DBG utils ERROR (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
//        assert(0);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        JFW_WARN("Vulkan DBG utils warning (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        JFW_INFO("Vulkan DBG utils information (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
    }
//    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
//    {
//        JFW_INFO("Vulkan DBG utils diagnostic (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
//    }
//    else
//    {
//        JFW_ERROR("Vulkan DBG utils (unknown severity) (category/ies: %s): %s", category_buffer, pCallbackData->pMessage);
//    }


    //  VK_FALSE means that the call which caused this message to be issued is not interrupted as a
    //  result of just calling this function. If this is just some info then you do not want this to interrupt everything
    return VK_FALSE;
}

static jfw_res jfw_vulkan_context_create(jfw_vulkan_context* this, const char* app_name, u32 app_version)
{
    JFW_ENTER_FUNCTION;
    u32 mtx_created = 0;
    VkResult vk_res;
    jfw_res res;

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

        u32 count;
        vk_res = vkEnumerateInstanceLayerProperties(&count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JFW_ERROR("Vulkan error with enumerating instance layer properties: %s", jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }
        VkLayerProperties* layer_properties;
        res = jfw_calloc(count, sizeof(*layer_properties), &layer_properties);
        if (!jfw_success(res))
        {
            JFW_ERROR("Failed allocating memory for vulkan layer properties array");
            goto failed;
        }
        vk_res = vkEnumerateInstanceLayerProperties(&count, layer_properties);
        if (vk_res != VK_SUCCESS)
        {
            jfw_free(&layer_properties);
            JFW_ERROR("Vulkan error with enumerating instance layer properties: %s", jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }

        u32 req_layers_found = 0;
        for (u32 i = 0; i < count && req_layers_found < REQUIRED_LAYERS_COUNT; ++i)
        {
            const char* name = layer_properties[i].layerName;
            for (u32 j = 0; j < REQUIRED_LAYERS_COUNT; ++j)
            {
                const char* lay_name = REQUIRED_LAYERS_ARRAY[j];
                const u64 lay_len = REQUIRED_LAYERS_LENGTHS[j];
                if (match_extension_name(lay_name, lay_len, name) != 0)
                {
                    req_layers_found += 1;
                    break;
                }
            }
        }
        jfw_free(&layer_properties);

        if (req_layers_found != REQUIRED_LAYERS_COUNT)
        {
            JFW_ERROR("Could only find %u out of %u required vulkan layers", req_layers_found, REQUIRED_LAYERS_COUNT);
            res = jfw_res_vk_fail;
            goto failed;
        }

        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JFW_ERROR("Vulkan error with enumerating instance extension properties: %s", jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }

        VkExtensionProperties* extension_properties;
        res = jfw_calloc(count, sizeof(*extension_properties), &extension_properties);
        if (!jfw_success(res))
        {
            JFW_ERROR("Failed allocating memory for extension properties");
            goto failed;
        }
        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &count, extension_properties);
        if (vk_res != VK_SUCCESS)
        {
            jfw_free(&extension_properties);
            JFW_ERROR("Vulkan error with enumerating extension layer properties: %s", jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }
        u32 req_extensions_found = 0;
        for (u32 i = 0; i < count && req_extensions_found < REQUIRED_EXTENSIONS_COUNT; ++i)
        {
            const char* name = extension_properties[i].extensionName;
            for (u32 j = 0; j < REQUIRED_EXTENSIONS_COUNT; ++j)
            {
                const char* ext_name = REQUIRED_EXTENSIONS_ARRAY[j];
                const u64 ext_len = REQUIRED_EXTENSIONS_LENGTHS[j];
                if (match_extension_name(ext_name, ext_len, name) != 0)
                {
                    req_extensions_found += 1;
                    break;
                }
            }
        }
        jfw_free(&extension_properties);

        if (req_extensions_found != REQUIRED_EXTENSIONS_COUNT)
        {
            JFW_ERROR("Could only find %u out of %u required vulkan extensions", req_extensions_found, REQUIRED_EXTENSIONS_COUNT);
            res = jfw_res_vk_fail;
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

        vk_res = vkCreateInstance(&create_info, NULL, &instance);
        if (vk_res != VK_SUCCESS)
        {
            JFW_ERROR("Failed creating vulkan instance with desired extensions and layers, reason: %s",
                      jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }
        this->instance = instance;
    }

#ifndef NDEBUG
    //  Create the debug messenger when in debug build configuration
    {
        this->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT");
        if (!this->vkCreateDebugUtilsMessengerEXT)
        {
            res = jfw_res_vk_fail;
            JFW_ERROR("Failed fetching the address for procedure \"vkCreateDebugUtilsMessengerEXT\"");
            goto failed;
        }
        this->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (!this->vkDestroyDebugUtilsMessengerEXT)
        {
            res = jfw_res_vk_fail;
            JFW_ERROR("Failed fetching the address for procedure \"vkDestroyDebugUtilsMessengerEXT\"");
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
            res = jfw_res_vk_fail;
            JFW_ERROR("Failed creating VkDebugUtilsMessenger");
            goto failed;
        }
        this->dbg_messenger = dbg_messenger;
    }
#endif

    //  Enumerate physical devices
    {
        u32 count;
        vk_res = vkEnumeratePhysicalDevices(this->instance, &count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JFW_ERROR("Could not enumerate instance physical devices, reason: %s", jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }
        if (!count)
        {
            JFW_ERROR("The instance had found %u available physical devices!", count);
            res = jfw_res_vk_fail;
            goto failed;
        }
        VkPhysicalDevice* devices;
        res = jfw_calloc(count, sizeof(*devices), &devices);
        if (res != jfw_res_success)
        {
            JFW_ERROR("Could not allocate memory for physical device list");
            goto failed;
        }
        vk_res = vkEnumeratePhysicalDevices(this->instance, &count, devices);
        if (vk_res != VK_SUCCESS)
        {
            jfw_free(&devices);
            JFW_ERROR("Could not enumerate instance physical devices, reason: %s", jfw_vk_error_msg(vk_res));
            res = jfw_res_vk_fail;
            goto failed;
        }
        this->n_physical_devices = count;
        this->p_physical_devices = devices;

        for (u32 i = 0; i < count; ++i)
        {
            u32 n;
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


    JFW_LEAVE_FUNCTION;
    return jfw_res_success;

    failed:

    if (this->instance)
    {
        if (this->n_physical_devices)
        {
            assert(this->p_physical_devices != NULL);
            jfw_free(&this->p_physical_devices);
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
    JFW_LEAVE_FUNCTION;
    return res;
}

static jfw_res jfw_vulkan_context_destroy(jfw_vulkan_context* this)
{
    JFW_ENTER_FUNCTION;
    jfw_free(&this->p_physical_devices);
#ifndef NDEBUG
    this->vkDestroyDebugUtilsMessengerEXT(this->instance, this->dbg_messenger, NULL);
#endif
    vkDestroyInstance(this->instance, NULL);
    memset(this, 0, sizeof(*this));
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res score_physical_device(
        VkPhysicalDevice device, VkSurfaceKHR surface, i32* p_score, u32 n_prop_buffer,
        VkExtensionProperties* ext_buffer, i32* p_queue_gfx, i32* p_queue_prs, i32* p_queue_trs, u32 n_queue_buffer,
        VkQueueFamilyProperties* queue_buffer, VkSampleCountFlagBits* p_sample_flags)
{
    JFW_ENTER_FUNCTION;
    i32 score = 0;
    u32 found_props = 0;
    u32 device_extensions = 0;
    VkResult vk_res = vkEnumerateDeviceExtensionProperties(device, NULL, &device_extensions, NULL);
    if (vk_res != VK_SUCCESS)
    {
        JFW_ERROR("Could not enumerate device extension properties for device, reason: %s", jfw_vk_error_msg(vk_res));
        return jfw_res_vk_fail;
    }
    assert(device_extensions <= n_prop_buffer);
    vk_res = vkEnumerateDeviceExtensionProperties(device, NULL, &device_extensions, ext_buffer);
    if (vk_res != VK_SUCCESS)
    {
        JFW_ERROR("Could not enumerate device extension properties for device, reason: %s", jfw_vk_error_msg(vk_res));
        return jfw_res_vk_fail;
    }
    for (u32 i = 0; i < device_extensions && found_props < DEVICE_REQUIRED_COUNT; ++i)
    {
        const char* ext_name = ext_buffer[i].extensionName;
        for (u32 j = 0; j < DEVICE_REQUIRED_COUNT; ++j)
        {
            const char* req_name = DEVICE_REQUIRED_EXTENSIONS[j];
            const u32 len = DEVICE_REQUIRED_LENGTHS[j];
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

    u32 queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, NULL);
    assert(queue_count <= n_queue_buffer);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queue_buffer);
    //  Find graphics display queue
    i32 gfx = -1, prs = -1;
    //  Check for combined support
    for (u32 i = 0; i < queue_count; ++i)
    {
        const VkQueueFamilyProperties* const props = queue_buffer + i;
        VkBool32 surface_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surface_support);
        if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT && surface_support)
        {
            gfx = (i32)i;
            prs = (i32)i;
            break;
        }
    }
    //  Check for separate support
    for (u32 i = 0; i < queue_count && (gfx == -1 || prs == -1); ++i)
    {
        const VkQueueFamilyProperties* const props = queue_buffer + i;
        if (props->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            gfx = (i32)i;
            if (prs != -1) break;
        }
        VkBool32 surface_support;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &surface_support);
        if (surface_support)
        {
            prs = (i32)i;
            if (gfx != -1) break;
        }
    }

    if (gfx == -1 || prs == -1)
    {
        score = -1;
        goto end;
    }

    //  Try and find a dedicated transfer queue
    i32 trs = -1;
    for (u32 i = 0; i < queue_count; ++i)
    {
        const VkQueueFamilyProperties* const props = queue_buffer + i;
        if (!(props->queueFlags & VK_QUEUE_GRAPHICS_BIT|VK_QUEUE_COMPUTE_BIT) && props->queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            trs = (i32)i;
            break;
        }
    }
    if (trs == -1)
    {
        //  According to Vulkan specifications, any graphics queue must support transfer operations
        trs = gfx;
    }

    //  Check for surface support with relation to swapchain
    u32 count;
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



    score = (i32)(1 + (features.geometryShader ? 10 : 0) + (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) + props.limits.maxImageDimension2D);


    *p_sample_flags = props.limits.framebufferColorSampleCounts;
    *p_queue_gfx = gfx;
    *p_queue_prs = prs;
    *p_queue_trs = trs;
    end:
    *p_score = score;
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_platform_create(
        jfw_ctx* ctx, jfw_platform* platform, u32 w, u32 h, size_t title_len, const char* title,
        u32 n_frames_in_filght)
{
    JFW_ENTER_FUNCTION;
    jfw_res result = jfw_res_success;
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
            result = jfw_res_platform_no_wnd;
            JFW_ERROR("Failed creating platform window handle");
            JFW_LEAVE_FUNCTION;
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
                        .min_width = (int) w,
                        .min_height = (int) h,
                        .max_width = (int) w,
                        .max_height = (int) h,
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
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not create vulkan surface from window, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->surface = surface;
    }

    //  Find a good physical device, which can support the surface's format
    u32 i_gfx_queue; u32 i_prs_queue; u32 i_trs_queue;
    VkSampleCountFlagBits samples;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    {
        i32 best_score = -1;
        VkExtensionProperties* prop_buffer;
        result = jfw_calloc(context->max_device_extensions, sizeof(*prop_buffer), &prop_buffer);
        if (result != jfw_res_success)
        {
            JFW_ERROR("Failed allocating memory for device extensions");
            goto failed;
        }
        VkQueueFamilyProperties* queue_buffer;
        jfw_calloc(context->max_device_queue_families, sizeof(*queue_buffer), &queue_buffer);
        if (result != jfw_res_success)
        {
            jfw_free(&prop_buffer);
            JFW_ERROR("Failed allocating memory for device extensions");
            goto failed;
        }
        for (u32 i = 0; i < context->n_physical_devices; ++i)
        {
            i32 score = -1; i32 gfx = -1; i32 prs = -1; i32 trs = -1; VkSampleCountFlagBits s;
            result = score_physical_device(
                    context->p_physical_devices[i], res->surface, &score, context->max_device_extensions, prop_buffer,
                    &gfx, &prs, &trs, context->max_device_queue_families, queue_buffer, &s);
            if (result == jfw_res_success && score > best_score)
            {
                best_score = score;
                i_gfx_queue = gfx;
                i_prs_queue = prs;
                i_trs_queue = trs;
                samples = s;
                physical_device = context->p_physical_devices[i];
            }
        }
        jfw_free(&prop_buffer);
        jfw_free(&queue_buffer);
        if (best_score == -1)
        {
            JFW_ERROR("Could not find a physical device which would support the bare minimum features required for Vulkan");
            result = jfw_res_vk_fail;
            goto failed;
        }
        assert(physical_device != VK_NULL_HANDLE);

        const f32 priority = 1.0f;
        u32 queue_indices[] =
                {
                i_gfx_queue, i_prs_queue, i_trs_queue
                };
#define n_queues (sizeof(queue_indices) / sizeof(*queue_indices))
        VkDeviceQueueCreateInfo queue_create_info[n_queues] = {};
        u32 unique_queues[n_queues];
        u32 n_unique_queues = 0;
        for (u32 i = 0, j; i < n_queues; ++i)
        {
            const u32 index = queue_indices[i];
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
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not create Vulkan logical device interface, reason: %s", jfw_vk_error_msg(vk_res));
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
        u32 sc_img_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount && sc_img_count > surface_capabilities.maxImageCount)
        {
            sc_img_count = surface_capabilities.maxImageCount;
        }
        res->n_images = sc_img_count;
        u64 max_buffer_size;
        u32 sf_count, pm_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, res->surface, &sf_count, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, res->surface, &pm_count, NULL);
        max_buffer_size = sf_count * sizeof(VkSurfaceFormatKHR);
        if (max_buffer_size < pm_count * sizeof(VkPresentModeKHR))
        {
            max_buffer_size = pm_count * sizeof(VkPresentModeKHR);
        }
        void* ptr_buffer;
        result = jfw_malloc(max_buffer_size, &ptr_buffer);
        if (!jfw_success(result))
        {
            JFW_ERROR("Could not allocate memory for the buffer used for presentation mode/surface formats");
            goto failed;
        }
        VkSurfaceFormatKHR* const formats_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, res->surface, &sf_count, formats_buffer);
        surface_format = formats_buffer[0];
        for (u32 i = 0; i < sf_count; ++i)
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
        for (u32 i = 0; i < sf_count; ++i)
        {
            const VkPresentModeKHR * const mode = present_buffer + i;
            if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = *mode;
                break;
            }
        }
        jfw_free(&ptr_buffer);
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
        u32 queue_indices[] =
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
                .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not create a vulkan swapchain, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->swapchain = sc;
        res->extent = extent;
        res->surface_format = surface_format;
    }

    //  Create swapchain images
    {
        u32 n_sc_images;
        vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, NULL);
        if (vk_res != VK_SUCCESS)
        {
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        VkImageView* views;
        result = jfw_calloc(n_sc_images, sizeof(*views), &views);
        if (!jfw_success(result))
        {
            JFW_ERROR("Could not allocate memory for the views");
            goto failed;
        }
        {
            VkImage* images;
            result = jfw_calloc(n_sc_images, sizeof(*images), &images);
            if (!jfw_success(result))
            {
                jfw_free(&views);
                JFW_ERROR("Could not allocate memory for swapchain images");
                goto failed;
            }
            vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, images);
            if (vk_res != VK_SUCCESS)
            {
                result = jfw_res_vk_fail;
                JFW_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
                jfw_free(&views);
                jfw_free(&images);
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

            for (u32 i = 0; i < n_sc_images; ++i)
            {
                create_info.image = images[i];
                vk_res = vkCreateImageView(res->device, &create_info, NULL, views + i);
                if (vk_res != VK_SUCCESS)
                {
                    result = jfw_res_vk_fail;
                    for (u32 j = 0; j < i; ++j)
                    {
                        vkDestroyImageView(res->device, views[j], NULL);
                    }
                    jfw_free(&views);
                    jfw_free(&images);
                    goto failed;
                }
            }
            jfw_free(&images);
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
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not create command pool for the gfx queue, reason: %s", jfw_vk_error_msg(vk_res));
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
        result = jfw_calloc(n_frames_in_filght, sizeof(*cmd_buffers), &cmd_buffers);
        if (!jfw_success(result))
        {
            JFW_ERROR("Failed allocating memory for command buffers array");
            goto failed;
        }
        result = jfw_calloc(n_frames_in_filght, sizeof(*sem_img), &sem_img);
        if (!jfw_success(result))
        {
            JFW_ERROR("Failed allocating memory for image semaphore array");
            jfw_free(&cmd_buffers);
            goto failed;
        }
        result = jfw_calloc(n_frames_in_filght, sizeof(*sem_prs), &sem_prs);
        if (!jfw_success(result))
        {
            JFW_ERROR("Failed allocating memory for present semaphore array");
            jfw_free(&cmd_buffers);
            jfw_free(&sem_img);
            goto failed;
        }
        result = jfw_calloc(n_frames_in_filght, sizeof(*fences), &fences);
        if (!jfw_success(result))
        {
            JFW_ERROR("Failed allocating memory for fences array");
            jfw_free(&cmd_buffers);
            jfw_free(&sem_img);
            jfw_free(&sem_prs);
            goto failed;
        }
        VkCommandBufferAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandBufferCount = n_frames_in_filght,
                .commandPool = res->cmd_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                };
        vk_res = vkAllocateCommandBuffers(res->device, &alloc_info, cmd_buffers);
        if (vk_res != VK_SUCCESS)
        {
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not allocate command buffers for %u frames in flight", n_frames_in_filght);
            jfw_free(&cmd_buffers);
            jfw_free(&sem_img);
            jfw_free(&sem_prs);
            jfw_free(&fences);
            goto failed;
        }
        res->cmd_buffers = cmd_buffers;
        for (u32 i = 0; i < n_frames_in_filght; ++i)
        {
            VkSemaphoreCreateInfo sem_create_info =
                    {
                    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    };
            vk_res = vkCreateSemaphore(res->device, &sem_create_info, NULL, sem_img + i);
            if (vk_res != VK_SUCCESS)
            {
                result = jfw_res_vk_fail;
                JFW_ERROR("Failed creating %u semaphores, reason: %s", n_frames_in_filght, jfw_vk_error_msg(vk_res));
                for (u32 j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(res->device, sem_img[j], NULL);
                }
                jfw_free(&sem_img);
                jfw_free(&sem_prs);
                jfw_free(&fences);
                goto failed;
            }
        }
        res->sem_img_available = sem_img;
        for (u32 i = 0; i < n_frames_in_filght; ++i)
        {
            VkSemaphoreCreateInfo sem_create_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    };
            vk_res = vkCreateSemaphore(res->device, &sem_create_info, NULL, sem_prs + i);
            if (vk_res != VK_SUCCESS)
            {
                result = jfw_res_vk_fail;
                JFW_ERROR("Failed creating %u semaphores, reason: %s", n_frames_in_filght, jfw_vk_error_msg(vk_res));
                for (u32 j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(res->device, sem_prs[j], NULL);
                }
                jfw_free(&sem_prs);
                jfw_free(&fences);
                goto failed;
            }
        }
        res->sem_present = sem_prs;
        for (u32 i = 0; i < n_frames_in_filght; ++i)
        {
            VkFenceCreateInfo create_info =
                    {
                    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                    };
            vk_res = vkCreateFence(res->device, &create_info, NULL, fences + i);
            if (vk_res != VK_SUCCESS)
            {
                result = jfw_res_vk_fail;
                JFW_ERROR("Failed creating %u fences, reason: %s", n_frames_in_filght, jfw_vk_error_msg(vk_res));
                for (u32 j = 0; j < i; ++j)
                {
                    vkDestroyFence(res->device, fences[j], NULL);
                }
                jfw_free(&fences);
                goto failed;
            }
        }
        res->swap_fences = fences;
    }


    JFW_LEAVE_FUNCTION;
    return result;



failed:
    if (res->swap_fences)
    {
        for (u32 j = 0; j < n_frames_in_filght; ++j)
        {
            vkDestroyFence(res->device, res->swap_fences[j], NULL);
        }
        jfw_free(&res->swap_fences);
    }
    if (res->cmd_buffers)
    {
        jfw_free(&res->cmd_buffers);
    }
    if (res->sem_present)
    {
        for (u32 j = 0; j < n_frames_in_filght; ++j)
        {
            vkDestroySemaphore(res->device, res->sem_present[j], NULL);
        }
        jfw_free(&res->sem_present);
    }
    if (res->sem_img_available)
    {
        for (u32 j = 0; j < n_frames_in_filght; ++j)
        {
            vkDestroySemaphore(res->device, res->sem_img_available[j], NULL);
        }
        jfw_free(&res->sem_img_available);
    }
    if (res->cmd_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(res->device, res->cmd_pool, NULL);
    }
    if (res->views)
    {
        for (u32 i = 0; i < res->n_images; ++i)
        {
            vkDestroyImageView(res->device, res->views[i], NULL);
            res->views[i] = VK_NULL_HANDLE;
        }
        jfw_free(&res->views);
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
    JFW_LEAVE_FUNCTION;
    return result;
}

jfw_res jfw_platform_destroy(jfw_platform* platform)
{
    JFW_ENTER_FUNCTION;
    jfw_ctx* ctx = platform->ctx;
    vkDeviceWaitIdle(platform->vk_res.device);
    for (u32 j = 0; j < platform->vk_res.n_frames_in_flight; ++j)
    {
        vkDestroyFence(platform->vk_res.device, platform->vk_res.swap_fences[j], NULL);
    }
    jfw_free(&platform->vk_res.swap_fences);
    jfw_free(&platform->vk_res.cmd_buffers);
    for (u32 j = 0; j < platform->vk_res.n_frames_in_flight; ++j)
    {
        vkDestroySemaphore(platform->vk_res.device, platform->vk_res.sem_present[j], NULL);
    }
    jfw_free(&platform->vk_res.sem_present);
    for (u32 j = 0; j < platform->vk_res.n_frames_in_flight; ++j)
    {
        vkDestroySemaphore(platform->vk_res.device, platform->vk_res.sem_img_available[j], NULL);
    }
    jfw_free(&platform->vk_res.sem_img_available);
    vkDestroyCommandPool(platform->vk_res.device, platform->vk_res.cmd_pool, NULL);
    for (u32 i = 0; i < platform->vk_res.n_images; ++i)
    {
        vkDestroyImageView(platform->vk_res.device, platform->vk_res.views[i], NULL);
        platform->vk_res.views[i] = VK_NULL_HANDLE;
    }
    jfw_free(&platform->vk_res.views);
    vkDestroySwapchainKHR(platform->vk_res.device, platform->vk_res.swapchain, NULL);
    vkDestroyDevice(platform->vk_res.device, NULL);
    vkDestroySurfaceKHR(ctx->vk_ctx.instance, platform->vk_res.surface, NULL);
    XDestroyWindow(ctx->dpy, platform->hwnd);
    memset(platform, 0, sizeof(*platform));
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_context_add_window(jfw_ctx* ctx, jfw_window* p_window)
{
    JFW_ENTER_FUNCTION;
    jfw_res result = jfw_res_success;
    if (ctx->wnd_count == ctx->wnd_capacity)
    {
        const u32 new_capacity = ctx->wnd_capacity << 1;
        if (!jfw_success(result = jfw_realloc(new_capacity * sizeof(p_window), &ctx->wnd_array)))
        {
            JFW_ERROR("Failed reallocating memory for context's window handles");
            JFW_LEAVE_FUNCTION;
            return result;
        }
        memset(ctx->wnd_array + ctx->wnd_count, 0, sizeof(p_window) * (new_capacity - ctx->wnd_capacity));
        ctx->wnd_capacity = new_capacity;
    }
    ctx->wnd_array[ctx->wnd_count++] = p_window;

    JFW_LEAVE_FUNCTION;
    return result;
}

jfw_res jfw_context_remove_window(jfw_ctx* ctx, jfw_window* p_window)
{
    JFW_ENTER_FUNCTION;
    for (u32 i = 0; i < ctx->wnd_count; ++i)
    {
        if (ctx->wnd_array[i] == p_window)
        {
            memmove(ctx->wnd_array + i, ctx->wnd_array + i + 1, sizeof(p_window) * (ctx->wnd_capacity - i - 1));
            ctx->wnd_array[--ctx->wnd_count] = 0;
            JFW_LEAVE_FUNCTION;
            return jfw_res_success;
        }
    }
    JFW_ERROR("Failed removing window %p from context since it was not found in its array", p_window);
    JFW_LEAVE_FUNCTION;
    return jfw_res_invalid_window;
}

jfw_res jfw_context_create(jfw_ctx** p_ctx)
{
    JFW_ENTER_FUNCTION;
    jfw_res result = jfw_res_success;
    jfw_ctx* ctx;
    if (!jfw_success(result = jfw_malloc(sizeof(*ctx), &ctx)))
    {
        JFW_ERROR("Failed allocating memory for context");
        JFW_LEAVE_FUNCTION;
        return result;
    }
    memset(ctx, 0, sizeof(*ctx));
    if (!jfw_success(result = jfw_calloc(sizeof(*ctx->wnd_array), 8, &ctx->wnd_array)))
    {
        jfw_free(&ctx);
        JFW_ERROR("Failed allocating memory for context's window array");
        JFW_LEAVE_FUNCTION;
        return result;
    }
    ctx->wnd_capacity = 8;

    Display* dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        jfw_free(&ctx->wnd_array);
        jfw_free(&ctx);
        JFW_ERROR("Failed opening XLib display");
        JFW_LEAVE_FUNCTION;
        return jfw_res_ctx_no_dpy;
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
        jfw_free(&ctx->wnd_array);
        jfw_free(&ctx);
        JFW_ERROR("Failed opening input method for XLib");
        JFW_LEAVE_FUNCTION;
        return jfw_res_ctx_no_im;
    }
    XIMStyle im_style = XIMStatusNone|XIMPreeditNone;
    ctx->input_ctx = XCreateIC(im, XNPreeditAttributes, XIMPreeditNone, XNStatusAttributes, XIMStatusNone, XNInputStyle, im_style, NULL);
    if (!ctx->input_ctx)
    {
        XCloseIM(im);
        XCloseDisplay(dpy);
        jfw_free(&ctx->wnd_array);
        jfw_free(&ctx);
        JFW_ERROR("Failed creating input context for XLib");
        JFW_LEAVE_FUNCTION;
        return jfw_res_ctx_no_ic;
    }
    XSetICFocus(ctx->input_ctx);

    result = jfw_vulkan_context_create(&ctx->vk_ctx, "jfw_context", 1);
    if (result != jfw_res_success)
    {
        XDestroyIC(ctx->input_ctx);
        XCloseIM(im);
        XCloseDisplay(dpy);
        jfw_free(&ctx->wnd_array);
        jfw_free(&ctx);
        JFW_ERROR("Failed creating input context for XLib");
        JFW_LEAVE_FUNCTION;
        return result;
    }

    *p_ctx = ctx;
    JFW_LEAVE_FUNCTION;
    return result;
}

jfw_res jfw_context_destroy(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
    for (u32 i = ctx->wnd_count; i != 0; --i)
    {
        jfw_window_destroy(ctx, ctx->wnd_array[i - 1]);
    }
    XSetICFocus(NULL);
    XCloseDisplay(ctx->dpy);
    jfw_vulkan_context_destroy(&ctx->vk_ctx);
    jfw_free(&ctx->wnd_array);
    jfw_free(&ctx);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_mouse_button_press(jfw_ctx* ctx, XEvent* ptr, jfw_window* this)
{
    JFW_ENTER_FUNCTION;
    XButtonPressedEvent* const e = &ptr->xbutton;

    if (ctx->mouse_state == 0)
    {
        //  New press on a window
        XSetICFocus(ctx->input_ctx);
        XSetInputFocus(ctx->dpy, this->platform.hwnd, RevertToNone, CurrentTime);   //  This generates FocusIn event
    }
//    XGrabPointer(ctx->dpy, this->platform.hwnd, False, PointerMotionMask|EnterWindowMask|LeaveWindowMask|ButtonMotionMask,GrabModeAsync,GrabModeAsync, None, None, CurrentTime);
    ctx->mouse_state |= (1 << (e->button - Button1));

    i32 x = e->x;
    i32 y = e->y;
    jfw_widget* widget = this->base;

check_children:
    x -= widget->x;
    y -= widget->y;
    for (u32 i = 0; i < widget->children_count; ++i)
    {
        jfw_widget * child = widget->children_array[i];
        if (x >= child->x && y >= child->y && x < child->x + child->width && y < child->y + child->height)
        {
            widget = child;
            goto check_children;
        }
    }
    if (widget->functions.mouse_button_press)
    {
        jfw_res res = widget->functions.mouse_button_press(widget, x, y, e->button, e->state);
        JFW_LEAVE_FUNCTION;
        return res;
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_mouse_button_release(jfw_ctx* ctx, XEvent* ptr, jfw_window* this)
{
    JFW_ENTER_FUNCTION;
    XButtonPressedEvent* const e = &ptr->xbutton;
    i32 x = e->x;
    i32 y = e->y;
    jfw_widget* widget = this->base;
    ctx->mouse_state &= ~(1 << (e->button - Button1));
    if (!ctx->mouse_state)
    {
        XUngrabPointer(ctx->dpy, 0);
    }
    check_children:
    x -= widget->x;
    y -= widget->y;
    for (u32 i = 0; i < widget->children_count; ++i)
    {
        jfw_widget * child = widget->children_array[i];
        if (x >= child->x && y >= child->y && x < child->x + child->width && y < child->y + child->height)
        {
            widget = child;
            goto check_children;
        }
    }
    if (widget->functions.mouse_button_release)
    {
        jfw_res res = widget->functions.mouse_button_release(widget, x, y, e->button, e->state);
        JFW_LEAVE_FUNCTION;
        return res;
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_client_message(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    XClientMessageEvent* const e = &ptr->xclient;
    if (e->data.l[0] == ctx->del_atom)
    {
        //  Request to close the window
        assert(e->window == wnd->platform.hwnd);
        if (wnd->hooks.on_close)
        {
            wnd->hooks.on_close(wnd);
        }
        else
        {
            jfw_res res = jfw_window_destroy(ctx, wnd);
            JFW_LEAVE_FUNCTION;
            return res;
        }
    }
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_focus_in(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    jfw_res res;
    if (wnd->keyboard_focus && wnd->keyboard_focus->functions.keyboard_focus_get && !jfw_success(res = wnd->keyboard_focus->functions.keyboard_focus_get(wnd->keyboard_focus, NULL)))
    {
        char w_buffer[64] = {0};
        jfw_widget_to_string(wnd->keyboard_focus, sizeof(w_buffer), w_buffer);
        JFW_ERROR("Failed handling giving focus to widget (%s) on focus gain", w_buffer);
        JFW_LEAVE_FUNCTION;
        return res;
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_focus_out(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    jfw_res res;
    if (wnd->keyboard_focus && wnd->keyboard_focus->functions.keyboard_focus_lose && !jfw_success(res = wnd->keyboard_focus->functions.keyboard_focus_lose(wnd->keyboard_focus, NULL)))
    {
        char w_buffer[64] = {0};
        jfw_widget_to_string(wnd->keyboard_focus, sizeof(w_buffer), w_buffer);
        JFW_ERROR("Failed handling taking focus from widget (%s) on focus loss", w_buffer);
        JFW_LEAVE_FUNCTION;
        return res;
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_kbd_mapping_change(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    XRefreshKeyboardMapping(&ptr->xmapping);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_keyboard_button_down(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    XKeyPressedEvent* const e = &ptr->xkey;
    char buffer[8];
    KeySym ks; Status stat;
    const int len = Xutf8LookupString(ctx->input_ctx, e, buffer, sizeof(buffer), &ks, &stat);
    if (wnd->keyboard_focus)
    {
        if (wnd->keyboard_focus->functions.button_down)
        {
            jfw_res res = wnd->keyboard_focus->functions.button_down(wnd->keyboard_focus, e->keycode);
            JFW_LEAVE_FUNCTION;
            return res;
        }
        if (wnd->keyboard_focus->functions.char_input && (stat == XLookupBoth || stat == XLookupChars))
        {
            jfw_res res = wnd->keyboard_focus->functions.char_input(wnd->keyboard_focus, buffer);
            JFW_LEAVE_FUNCTION;
            return res;
        }
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_keyboard_button_up(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    XKeyReleasedEvent* const e = &ptr->xkey;
    if (wnd->keyboard_focus && wnd->keyboard_focus->functions.button_up)
    {
        {
            jfw_res res = wnd->keyboard_focus->functions.button_up(wnd->keyboard_focus, e->keycode);
            JFW_LEAVE_FUNCTION;
            return res;
        }
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

static jfw_res handle_config_notify(jfw_ctx* ctx, XEvent* ptr, jfw_window* wnd)
{
    JFW_ENTER_FUNCTION;
    XConfigureEvent* const e = &ptr->xconfigure;
    jfw_res res = jfw_res_success;
    if (e->width != wnd->w || e->height != wnd->h)
    {
        if (wnd->base && wnd->base->functions.parent_resized)
        {
            res = wnd->base->functions.parent_resized(wnd->base, NULL, wnd->w, wnd->h, e->width, e->height);
            if (!jfw_success(res))
            {
                JFW_ERROR("Window base returned code \"%s\" when it's parent_resized function was called",
                          jfw_error_message(res));
            }
        }
        wnd->w = e->width; wnd->h = e->height;
    }

    JFW_LEAVE_FUNCTION;
    return res;
}

static jfw_res(*XEVENT_HANDLERS[LASTEvent])(jfw_ctx* ctx, XEvent* e, jfw_window* wnd) =
        {
        [ButtonPress] = handle_mouse_button_press,
        [ButtonRelease] = handle_mouse_button_release,
        [ClientMessage] = handle_client_message,
//        [FocusIn] = handle_focus_in,
//        [FocusOut] = handle_focus_out,
//        [KeyPress] = handle_keyboard_button_down,
//        [KeyRelease] = handle_keyboard_button_up,
        [MappingNotify] = handle_kbd_mapping_change,
        [ConfigureNotify] = handle_config_notify,
        };



jfw_res jfw_context_process_events(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
    if (!ctx->wnd_count)
    {
        JFW_LEAVE_FUNCTION;
        return jfw_res_ctx_no_windows;
    }
    XEvent e;
    if (XPending(ctx->dpy))
    {
        XNextEvent(ctx->dpy, &e);
        jfw_res (* const handler)(jfw_ctx* ctx, XEvent* e, jfw_window* wnd) = XEVENT_HANDLERS[e.type];
        if (handler)
        {
            if (e.type == MappingNotify)
            {
                handler(ctx, &e, NULL);
            }
            else for (u32 i = 0; i < ctx->wnd_count; ++i)
            {
                jfw_window* wnd = ctx->wnd_array[i];
                if (wnd->platform.hwnd == e.xany.window)
                {
                    jfw_res res = handler(ctx, &e, wnd);
                    JFW_LEAVE_FUNCTION;
                    return res;
                }
            }
        }
    }
//    XFlush(ctx->dpy);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

int jfw_context_status(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
    int status = ctx->status;
    JFW_LEAVE_FUNCTION;
    return status;
}

int jfw_context_set_status(jfw_ctx* ctx, int status)
{
    JFW_ENTER_FUNCTION;
    const int prev_status = ctx->status;
    ctx->status = status;
    JFW_LEAVE_FUNCTION;
    return prev_status;
}

static int map_predicate(Display* dpy, XEvent* e, XPointer ptr)
{
    const Window wnd = (Window)ptr;
    return dpy && e && e->type == MapNotify && e->xmap.window == wnd;

}

jfw_res jfw_platform_show(jfw_ctx* ctx, jfw_platform* wnd)
{
    JFW_ENTER_FUNCTION;
    XMapWindow(ctx->dpy, wnd->hwnd);
    XEvent unused;
    XPeekIfEvent(ctx->dpy, &unused, map_predicate, (XPointer)wnd->hwnd);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_platform_hide(jfw_ctx* ctx, jfw_platform* wnd)
{
    JFW_ENTER_FUNCTION;
    XUnmapWindow(ctx->dpy, wnd->hwnd);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_platform_draw_bind(jfw_ctx* ctx, jfw_platform* wnd)
{
    JFW_ENTER_FUNCTION;
    const int res = True;
//    const int res = glXMakeCurrent(ctx->dpy, wnd->glw, wnd->gl_ctx);
    JFW_LEAVE_FUNCTION;
    return res == True ? jfw_res_success : jfw_res_platform_no_bind;
}

jfw_res jfw_platform_unbind(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
//    glXMakeCurrent(ctx->dpy, None, None);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_res jfw_platform_swap(jfw_ctx* ctx, jfw_platform* wnd)
{
    JFW_ENTER_FUNCTION;
//    glXMakeCurrent(ctx->dpy, None, None);

//    glXMakeCurrent(ctx->dpy, wnd->glw, wnd->gl_ctx);
//    glXSwapBuffers(ctx->dpy, wnd->glw);
//    glFinish();
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

u32 jfw_context_window_count(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
    u32 count = ctx->wnd_count;
    JFW_LEAVE_FUNCTION;
    return count;
}

jfw_res jfw_context_wait_for_events(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
    XEvent unused;
    XPeekEvent(ctx->dpy, &unused);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

int jfw_context_has_events(jfw_ctx* ctx)
{
    JFW_ENTER_FUNCTION;
    int pending = XPending(ctx->dpy);
    JFW_LEAVE_FUNCTION;
    return pending;
}

jfw_res jfw_platform_clear_window(jfw_ctx* ctx, jfw_platform* wnd)
{
    JFW_ENTER_FUNCTION;
    XClearWindow(ctx->dpy, wnd->hwnd);
    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
}

jfw_window_vk_resources* jfw_window_get_vk_resources(jfw_window* p_window)
{
    return &p_window->platform.vk_res;
}

static inline jfw_res create_swapchain(jfw_ctx* ctx, jfw_platform* plt, jfw_window_vk_resources* res)
{
    jfw_res result;
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkResult vk_res;
    {
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(res->physical_device, res->surface, &surface_capabilities);
        u32 sc_img_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount && sc_img_count > surface_capabilities.maxImageCount)
        {
            sc_img_count = surface_capabilities.maxImageCount;
        }
        res->n_images = sc_img_count;
        u64 max_buffer_size;
        u32 sf_count, pm_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(res->physical_device, res->surface, &sf_count, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(res->physical_device, res->surface, &pm_count, NULL);
        max_buffer_size = sf_count * sizeof(VkSurfaceFormatKHR);
        if (max_buffer_size < pm_count * sizeof(VkPresentModeKHR))
        {
            max_buffer_size = pm_count * sizeof(VkPresentModeKHR);
        }
        void* ptr_buffer;
        result = jfw_malloc(max_buffer_size, &ptr_buffer);
        if (!jfw_success(result))
        {
            JFW_ERROR("Could not allocate memory for the buffer used for presentation mode/surface formats");
            goto failed;
        }
        VkSurfaceFormatKHR* const formats_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfaceFormatsKHR(res->physical_device, res->surface, &sf_count, formats_buffer);
        surface_format = formats_buffer[0];
        for (u32 i = 0; i < sf_count; ++i)
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
        for (u32 i = 0; i < sf_count; ++i)
        {
            const VkPresentModeKHR * const mode = present_buffer + i;
            if (*mode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                present_mode = *mode;
                break;
            }
        }
        jfw_free(&ptr_buffer);
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
        u32 queue_indices[] =
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
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not create a vulkan swapchain, reason: %s", jfw_vk_error_msg(vk_res));
            goto failed;
        }
        res->swapchain = sc;
        res->extent = extent;
        res->surface_format = surface_format;
    }
    return jfw_res_success;
    failed:
    return result;
}

static inline jfw_res create_swapchain_img_views(jfw_window_vk_resources* res)
{
    jfw_res result;
    VkResult vk_res;
    u32 n_sc_images;
    vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, NULL);
    if (vk_res != VK_SUCCESS)
    {
        result = jfw_res_vk_fail;
        JFW_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
        goto failed;
    }
    assert(n_sc_images == res->n_images);
    VkImageView* views = res->views;
//    result = jfw_calloc(n_sc_images, sizeof(*views), &views);
//    if (!jfw_success(result))
//    {
//        JFW_ERROR("Could not allocate memory for the views");
//        goto failed;
//    }
    {
        VkImage* images;
        result = jfw_calloc(n_sc_images, sizeof(*images), &images);
        if (!jfw_success(result))
        {
            jfw_free(&views);
            JFW_ERROR("Could not allocate memory for swapchain images");
            goto failed;
        }
        vk_res = vkGetSwapchainImagesKHR(res->device, res->swapchain, &n_sc_images, images);
        if (vk_res != VK_SUCCESS)
        {
            result = jfw_res_vk_fail;
            JFW_ERROR("Could not find the number of swapchain images, reason: %s", jfw_vk_error_msg(vk_res));
//            jfw_free(&views);
            jfw_free(&images);
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

        for (u32 i = 0; i < n_sc_images; ++i)
        {
            create_info.image = images[i];
            vk_res = vkCreateImageView(res->device, &create_info, NULL, views + i);
            if (vk_res != VK_SUCCESS)
            {
                result = jfw_res_vk_fail;
                for (u32 j = 0; j < i; ++j)
                {
                    vkDestroyImageView(res->device, views[j], NULL);
                }
                jfw_free(&images);
                goto failed;
            }
        }
        jfw_free(&images);
    }
//    res->views = views;
    return jfw_res_success;

failed:
    return result;
}

jfw_res jfw_window_update_swapchain(jfw_window* p_window)
{
    JFW_ENTER_FUNCTION;
    jfw_window_vk_resources* const this = &p_window->platform.vk_res;
    vkDeviceWaitIdle(this->device);

    //  Cleanup current swapchain
    {
        for (u32 i = 0; i < this->n_images; ++i)
        {
            vkDestroyImageView(this->device, this->views[i], NULL);
        }
        vkDestroySwapchainKHR(this->device, this->swapchain, NULL);
    }


    jfw_res result = create_swapchain(p_window->ctx, &p_window->platform, &p_window->platform.vk_res);
    if (!jfw_success(result))
    {
        JFW_ERROR("Could not recreate swapchain");
        goto failed;
    }

    //  Create swapchain images
    result = create_swapchain_img_views(&p_window->platform.vk_res);
    if (!jfw_success(result))
    {
        JFW_ERROR("Could not recreate image views");
        goto failed;
    }

    JFW_LEAVE_FUNCTION;
    return jfw_res_success;
    failed:
    JFW_LEAVE_FUNCTION;
    return result;
}
