//
// Created by jan on 11.8.2023.
//

#include "vk_resources.h"
#include <jdm.h>
#include <ctype.h>
#include <inttypes.h>
#include "../jwin/source/jwin_vk.h"
#include "mesh.h"
#include <jrui.h>

//  Compiled Vulkan shaders
#include "vtx_shader3d.vert.h"
#include "frg_shader3d.frag.h"
#include "vtx_shader_ui.vert.h"
#include "frg_shader_ui.frag.h"

static const char* const REQUIRED_LAYERS_NAMES[] =
        {
#ifndef NDEBUG
        //  Debug only layers
                "VK_LAYER_KHRONOS_validation",
#endif
        //  Non-debug layers
        };
static const size_t REQUIRED_LAYERS_LENGTHS[] =
        {
#ifndef NDEBUG
                //  Debug only layers
                sizeof("VK_LAYER_KHRONOS_validation") - 1,
#endif
                //  Non-debug layers
        };
static const uint32_t REQUIRED_LAYERS_COUNT = sizeof(REQUIRED_LAYERS_NAMES) / sizeof(*REQUIRED_LAYERS_NAMES);


VkBool32 match_extension_name(const char* extension_name, const uint64_t len, const char* str_to_match)
{
    //  If beginning of the name is not the same, they do not match
    if (strstr(extension_name, str_to_match) == NULL)
    {
        return 0;
    }
    //  If this holds, the two strings are identical
    if (extension_name[len] == 0)
    {
        return 1;
    }
    
    //  For the case where the extension name may be followed by a number, allow it, if the desired extension did not
    //  specify one
    return (isdigit(extension_name[len]) && !isdigit(str_to_match[len - 1]));
}

#ifndef NDEBUG

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData)
{
    (void) pUserData;
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
#endif

static const char* const DEVICE_REQUIRED_NAMES[] =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
static size_t DEVICE_REQUIRED_LENGTHS[] =
        {
            sizeof(VK_KHR_SWAPCHAIN_EXTENSION_NAME) - 1,
        };
static size_t DEVICE_REQUIRED_COUNT = sizeof(DEVICE_REQUIRED_NAMES) / sizeof(*DEVICE_REQUIRED_NAMES);


static gfx_result score_physical_device(
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
        JDM_ERROR("Could not enumerate device extension properties for device, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        return GFX_RESULT_BAD_VK_CALL;
    }
    (void)n_prop_buffer;
    (void)n_queue_buffer;
    assert(device_extensions <= n_prop_buffer);
    vk_res = vkEnumerateDeviceExtensionProperties(device, NULL, &device_extensions, ext_buffer);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not enumerate device extension properties for device, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        return GFX_RESULT_BAD_VK_CALL;
    }
    for (uint32_t i = 0; i < device_extensions && found_props < DEVICE_REQUIRED_COUNT; ++i)
    {
        const char* ext_name = ext_buffer[i].extensionName;
        for (uint32_t j = 0; j < DEVICE_REQUIRED_COUNT; ++j)
        {
            const char* req_name = DEVICE_REQUIRED_NAMES[j];
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
    return GFX_RESULT_SUCCESS;
}

gfx_result
jta_vulkan_context_create(
        const VkAllocationCallbacks* allocator, const char* app_name, uint32_t app_version, jta_vulkan_context** p_out)
{
    //  Create vulkan instance
    JDM_ENTER_FUNCTION;
    VkResult vk_res;
    gfx_result res;

    jta_vulkan_context* this = ill_alloc(G_ALLOCATOR, sizeof(*this));
    if (!this)
    {
        JDM_ERROR("Failed allocating memory for vulkan context");
        res = GFX_RESULT_BAD_ALLOC;
        goto failed;
    }

    //  Create instance with appropriate layers and extensions
    {
        VkInstance instance;
        VkApplicationInfo app_info =
                {
                        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                        .apiVersion = VK_API_VERSION_1_0,
                        .engineVersion = 1,
                        .pApplicationName = app_name,
                        .pEngineName = "jta-engine",
                        .applicationVersion = app_version,
                };

        uint32_t count;
        vk_res = vkEnumerateInstanceLayerProperties(&count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Vulkan error with enumerating instance layer properties: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
        VkLayerProperties* layer_properties;
        layer_properties = lin_alloc(G_LIN_ALLOCATOR, count * sizeof(*layer_properties));
        if (!layer_properties)
        {
            JDM_ERROR("Failed allocating memory for vulkan layer properties array");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkEnumerateInstanceLayerProperties(&count, layer_properties);
        if (vk_res != VK_SUCCESS)
        {
            lin_jfree(G_LIN_ALLOCATOR, layer_properties);
            JDM_ERROR("Vulkan error with enumerating instance layer properties: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        uint32_t req_layers_found = 0;
        for (uint32_t i = 0; i < count && req_layers_found < REQUIRED_LAYERS_COUNT; ++i)
        {
            const char* name = layer_properties[i].layerName;
            for (uint32_t j = 0; j < REQUIRED_LAYERS_COUNT; ++j)
            {
                const char* lay_name = REQUIRED_LAYERS_NAMES[j];
                const uint64_t lay_len = REQUIRED_LAYERS_LENGTHS[j];
                if (match_extension_name(lay_name, lay_len, name) != 0)
                {
                    req_layers_found += 1;
                    break;
                }
            }
        }
        lin_jfree(G_LIN_ALLOCATOR, layer_properties);

        if (req_layers_found != REQUIRED_LAYERS_COUNT)
        {
            JDM_ERROR("Could only find %u out of %"PRIu32" required vulkan layers", req_layers_found, REQUIRED_LAYERS_COUNT);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Vulkan error with enumerating instance extension properties: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        VkExtensionProperties* extension_properties;
        extension_properties = lin_alloc(G_LIN_ALLOCATOR, count * sizeof(*extension_properties));
        if (!extension_properties)
        {
            JDM_ERROR("Failed allocating memory for extension properties");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkEnumerateInstanceExtensionProperties(NULL, &count, extension_properties);
        if (vk_res != VK_SUCCESS)
        {
            lin_jfree(G_LIN_ALLOCATOR, extension_properties);
            JDM_ERROR("Vulkan error with enumerating extension layer properties: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        const char* const* jwin_required_extension_names;
        unsigned jwin_required_extension_count;
        jwin_required_vk_extensions(&jwin_required_extension_count, &jwin_required_extension_names);
        unsigned required_extension_count = jwin_required_extension_count;
#ifndef NDEBUG
        //  Add debug layers
        required_extension_count += 2;
#endif
        const char** const required_extension_names = lin_alloc(G_LIN_ALLOCATOR, sizeof(const char*) * required_extension_count);
        if (!required_extension_names)
        {
            lin_jfree(G_LIN_ALLOCATOR, extension_properties);
            JDM_ERROR("Failed allocating memory for required extension array");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        size_t* const required_extension_lengths = lin_alloc(G_LIN_ALLOCATOR, sizeof(size_t) * required_extension_count);
        if (!required_extension_lengths)
        {
            lin_jfree(G_LIN_ALLOCATOR, required_extension_names);
            lin_jfree(G_LIN_ALLOCATOR, extension_properties);
            JDM_ERROR("Failed allocating memory for required extension array");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }

        //  Fill in the names array
        {
            uint32_t i;
            for (i = 0; i < jwin_required_extension_count; ++i)
            {
                required_extension_names[i] = jwin_required_extension_names[i];
            }
#ifndef NDEBUG
            required_extension_names[i++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
            required_extension_names[i++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
#endif
            assert(i == required_extension_count);
        }

        for (uint32_t i = 0; i < required_extension_count; ++i)
        {
            required_extension_lengths[i] = strlen(required_extension_names[i]);
        }

        uint32_t req_extensions_found = 0;
        for (uint32_t i = 0; i < count && req_extensions_found < required_extension_count; ++i)
        {
            const char* name = extension_properties[i].extensionName;
            for (uint32_t j = 0; j < required_extension_count; ++j)
            {
                const char* ext_name = required_extension_names[j];
                const uint64_t ext_len = required_extension_lengths[j];
                if (match_extension_name(ext_name, ext_len, name) != 0)
                {
                    req_extensions_found += 1;
                    break;
                }
            }
        }

        if (req_extensions_found != required_extension_count)
        {
            JDM_ERROR("Could only find %u out of %u required vulkan extensions", req_extensions_found, required_extension_count);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        VkInstanceCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                        .enabledExtensionCount = required_extension_count,
                        .ppEnabledExtensionNames = required_extension_names,
                        .enabledLayerCount = REQUIRED_LAYERS_COUNT,
                        .ppEnabledLayerNames = REQUIRED_LAYERS_NAMES,
                        .pApplicationInfo = &app_info,
                };

        vk_res = vkCreateInstance(&create_info, allocator, &instance);
        lin_jfree(G_LIN_ALLOCATOR, required_extension_lengths);
        lin_jfree(G_LIN_ALLOCATOR, required_extension_names);
        lin_jfree(G_LIN_ALLOCATOR, extension_properties);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating vulkan instance with desired extensions and layers, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
        this->instance = instance;
        this->allocator = allocator;
    }

#ifndef NDEBUG
    //  Create the debug messenger when in debug build configuration
    {
        this->vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkCreateDebugUtilsMessengerEXT");
        if (!this->vkCreateDebugUtilsMessengerEXT)
        {
            res = GFX_RESULT_BAD_VK_CALL;
            JDM_ERROR("Failed fetching the address for procedure \"vkCreateDebugUtilsMessengerEXT\"");
            goto failed;
        }
        this->vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (!this->vkDestroyDebugUtilsMessengerEXT)
        {
            res = GFX_RESULT_BAD_VK_CALL;
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
            res = GFX_RESULT_BAD_VK_CALL;
            JDM_ERROR("Failed creating VkDebugUtilsMessenger, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            goto failed;
        }
        this->dbg_messenger = dbg_messenger;
    }
#endif
    
    *p_out = this;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

    failed:

    if (this && this->instance)
    {
        
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
    ill_jfree(G_ALLOCATOR, this);
    JDM_LEAVE_FUNCTION;
    return res;
}

const char* vk_result_to_str(VkResult res)
{
    switch (res)
    {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_EVENT_SET: return "VK_EVENT_SET";
    case VK_EVENT_RESET: return "VK_EVENT_RESET";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR: return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR: return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_KHR: return "VK_ERROR_NOT_PERMITTED_KHR";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR: return "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT: return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
    case VK_RESULT_MAX_ENUM: return "VK_RESULT_MAX_ENUM";
    default:return "Unknown";
    }
}

void jta_vulkan_context_destroy(jta_vulkan_context* ctx)
{
#ifndef NDEBUG
    ctx->vkDestroyDebugUtilsMessengerEXT(ctx->instance, ctx->dbg_messenger, NULL);
#endif
    vkDestroyInstance(ctx->instance, ctx->allocator);
    ill_jfree(G_ALLOCATOR, ctx);
}

static gfx_result create_swapchain(
        jwin_window* win, VkPhysicalDevice physical_device, VkSurfaceKHR surface,
        VkDevice device, jta_vulkan_swapchain* this, uint32_t i_gfx_queue, uint32_t i_prs_queue,
        uint32_t frames_in_flight, jvm_allocator* allocator)
{
    JDM_ENTER_FUNCTION;
    gfx_result res = GFX_RESULT_SUCCESS;
    //  Create the window surface
    void* const base = lin_allocator_save_state(G_LIN_ALLOCATOR);

    VkResult vk_res;
    uint32_t n_images;

    //  Create the swapchain
    VkSurfaceFormatKHR surface_format;
    VkPresentModeKHR present_mode;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D extent;
    VkImageView* views = NULL;
    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    jvm_image_allocation* depth_img = NULL;
    VkImageView depth_view = VK_NULL_HANDLE;
    VkFormat depth_format;
    VkCommandBuffer* cmd_buffers = NULL;
    VkSemaphore* sem_present = VK_NULL_HANDLE;
    VkSemaphore* sem_available = VK_NULL_HANDLE;
    VkFence* fen_swap = VK_NULL_HANDLE;
    jta_frame_job_queue** frame_queues = NULL;

    {
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);
        uint32_t sc_img_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount && sc_img_count > surface_capabilities.maxImageCount)
        {
            sc_img_count = surface_capabilities.maxImageCount;
        }
        n_images = sc_img_count;
        uint64_t max_buffer_size;
        uint32_t sf_count, pm_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &sf_count, NULL);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &pm_count, NULL);
        max_buffer_size = sf_count * sizeof(VkSurfaceFormatKHR);
        if (max_buffer_size < pm_count * sizeof(VkPresentModeKHR))
        {
            max_buffer_size = pm_count * sizeof(VkPresentModeKHR);
        }
        void* ptr_buffer = lin_alloc(G_LIN_ALLOCATOR, max_buffer_size);
        if (!ptr_buffer)
        {
            JDM_ERROR("Could not allocate memory for the buffer used for presentation mode/surface formats");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        VkSurfaceFormatKHR* const formats_buffer = ptr_buffer;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &sf_count, formats_buffer);
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &pm_count, present_buffer);
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
        lin_jfree(G_LIN_ALLOCATOR, ptr_buffer);
        extent = surface_capabilities.minImageExtent;
        if (extent.width != ~0u)
        {
            assert(extent.height != ~0u);
            unsigned wnd_w, wnd_h;
            jwin_window_get_size(win, &wnd_w, &wnd_h);
            extent.width = wnd_w;
            extent.height = wnd_h;
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
                        .surface = surface,
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

        vk_res = vkCreateSwapchainKHR(device, &create_info, NULL, &swapchain);
        if (vk_res != VK_SUCCESS)
        {
            res = GFX_RESULT_BAD_VK_CALL;
            JDM_ERROR("Could not create a vulkan swapchain, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            goto failed;
        }
    }

    //  Create swapchain images
    {
        vk_res = vkGetSwapchainImagesKHR(device, swapchain, &n_images, NULL);
        if (vk_res != VK_SUCCESS)
        {
            res = GFX_RESULT_BAD_VK_CALL;
            JDM_ERROR("Could not find the number of swapchain images, reason: %s", vk_result_to_str(vk_res));
            goto failed;
        }
        views = ill_alloc(G_ALLOCATOR, n_images * sizeof(*views));
        if (!views)
        {
            JDM_ERROR("Could not allocate memory for the views");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        {
            VkImage* images = lin_alloc(G_LIN_ALLOCATOR, n_images * sizeof(VkImage));
            if (!images)
            {
                JDM_ERROR("Could not allocate memory for swapchain images");
                res = GFX_RESULT_BAD_ALLOC;
                goto failed;
            }
            vk_res = vkGetSwapchainImagesKHR(device, swapchain, &n_images, images);
            if (vk_res != VK_SUCCESS)
            {
                res = GFX_RESULT_BAD_VK_CALL;
                JDM_ERROR("Could not find the number of swapchain images, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
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

            for (uint32_t i = 0; i < n_images; ++i)
            {
                create_info.image = images[i];
                vk_res = vkCreateImageView(device, &create_info, NULL, views + i);
                if (vk_res != VK_SUCCESS)
                {
                    res = GFX_RESULT_BAD_VK_CALL;
                    for (uint32_t j = 0; j < i; ++j)
                    {
                        vkDestroyImageView(device, views[j], NULL);
                    }
                    goto failed;
                }
            }
            lin_jfree(G_LIN_ALLOCATOR, images);
        }
    }

    //  Create the command pool for the command buffers
    {
        VkCommandPoolCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                        .queueFamilyIndex = i_gfx_queue,
                };
        vk_res = vkCreateCommandPool(device, &create_info, NULL, &cmd_pool);
        if (vk_res != VK_SUCCESS)
        {
            res = GFX_RESULT_BAD_VK_CALL;
            JDM_ERROR("Could not create command pool for the gfx queue, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            goto failed;
        }
    }

    i32 has_stencil;
    //  Create the depth buffer
    {

        const VkFormat possible_depth_formats[3] = {VK_FORMAT_D32_SFLOAT_S8_UINT , VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT};
        const u32 n_possible_depth_formats = sizeof(possible_depth_formats) / sizeof(*possible_depth_formats);
        VkFormatProperties props;
        u32 i;
        for (i = 0; i < n_possible_depth_formats; ++i)
        {
            vkGetPhysicalDeviceFormatProperties(physical_device, possible_depth_formats[i], &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                break;
            }
        }
        if (i == n_possible_depth_formats)
        {
            JDM_ERROR("Could not find a good image format for the depth buffer");
            res = GFX_RESULT_NO_IMG_FMT;
            goto failed;
        }
        depth_format = possible_depth_formats[i];
        has_stencil = (depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT);
        VkImageCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                        .imageType = VK_IMAGE_TYPE_2D,
                        .extent = { .width = extent.width, .height = extent.height, .depth = 1 },
                        .mipLevels = 1,
                        .arrayLayers = 1,
                        .format = depth_format,
                        .tiling = VK_IMAGE_TILING_OPTIMAL,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                };
        vk_res = jvm_image_create(allocator, &create_info, 0, 0, 0, &depth_img);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create depth buffer image, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_DEPBUF_IMG;
            goto failed;
        }

        VkImageViewCreateInfo view_info =
                {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format = depth_format,
                        .image = jvm_image_allocation_get_image(depth_img),

                        .components.a = VK_COMPONENT_SWIZZLE_A,
                        .components.r = VK_COMPONENT_SWIZZLE_R,
                        .components.g = VK_COMPONENT_SWIZZLE_G,
                        .components.b = VK_COMPONENT_SWIZZLE_B,

                        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | (has_stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0),
                        .subresourceRange.baseMipLevel = 0,
                        .subresourceRange.levelCount = 1,
                        .subresourceRange.baseArrayLayer = 0,
                        .subresourceRange.layerCount = 1,
                };
        vk_res = vkCreateImageView(device, &view_info, NULL, &depth_view);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create image view for the depth buffer: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_IMG_VIEW;
            goto failed;
        }
    }

    //  Create command buffers
    {
        cmd_buffers = ill_alloc(G_ALLOCATOR, sizeof(VkCommandBuffer) * frames_in_flight);
        if (!cmd_buffers)
        {
            JDM_ERROR("Could not allocate memory for command buffer array");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        VkCommandBufferAllocateInfo allocate_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandBufferCount = frames_in_flight,
                .commandPool = cmd_pool,
                };
        vk_res = vkAllocateCommandBuffers(device, &allocate_info, cmd_buffers);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not allocate command buffers, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
    }

    //  Create frame objects (semaphores and fences)
    {
        sem_available = ill_alloc(G_ALLOCATOR, sizeof(VkSemaphore) * frames_in_flight);
        if (!sem_available)
        {
            JDM_ERROR("Could not allocate memory for semaphores");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        sem_present = ill_alloc(G_ALLOCATOR, sizeof(VkSemaphore) * frames_in_flight);
        if (!sem_present)
        {
            ill_jfree(G_ALLOCATOR, sem_available);
            JDM_ERROR("Could not allocate memory for semaphores");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            VkSemaphoreCreateInfo sem_create_info =
                    {
                            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                    };
            vk_res = vkCreateSemaphore(device, &sem_create_info, NULL, sem_available + i);
            if (vk_res != VK_SUCCESS)
            {
                res = GFX_RESULT_BAD_VK_CALL;
                JDM_ERROR("Failed creating %u semaphore pairs, reason: %s(%d)", frames_in_flight, vk_result_to_str(vk_res), vk_res);
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(device, sem_available[j], NULL);
                    vkDestroySemaphore(device, sem_present[j], NULL);
                }
                ill_jfree(G_ALLOCATOR, sem_available);
                ill_jfree(G_ALLOCATOR, sem_present);
                sem_available = NULL;
                sem_present = NULL;
                goto failed;
            }
            vk_res = vkCreateSemaphore(device, &sem_create_info, NULL, sem_present + i);
            if (vk_res != VK_SUCCESS)
            {
                vkDestroySemaphore(device, sem_available[i], NULL);
                res = GFX_RESULT_BAD_VK_CALL;
                JDM_ERROR("Failed creating %u semaphore pairs, reason: %s(%d)", frames_in_flight, vk_result_to_str(vk_res), vk_res);
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroySemaphore(device, sem_available[j], NULL);
                    vkDestroySemaphore(device, sem_present[j], NULL);
                }
                ill_jfree(G_ALLOCATOR, sem_available);
                ill_jfree(G_ALLOCATOR, sem_present);
                sem_available = NULL;
                sem_present = NULL;
                goto failed;
            }
        }

        fen_swap = ill_alloc(G_ALLOCATOR, sizeof(VkFence) * frames_in_flight);
        if (!fen_swap)
        {
            JDM_ERROR("Could not allocate memory for fence array");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        const VkFenceCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                };
        for (uint32_t i = 0; i < frames_in_flight; ++i)
        {
            vk_res = vkCreateFence(device, &create_info, NULL, fen_swap + i);
            if (vk_res != VK_SUCCESS)
            {
                JDM_ERROR("Could not create %u fences, reason: %s(%d)", frames_in_flight, vk_result_to_str(vk_res), vk_res);
                for (uint32_t j = 0; j < i; ++j)
                {
                    vkDestroyFence(device, fen_swap[i], NULL);
                }
                ill_jfree(G_ALLOCATOR, fen_swap);
                fen_swap = NULL;
                res = GFX_RESULT_BAD_VK_CALL;
                goto failed;
            }
        }
    }

    //  Create frame job queues
    {
        frame_queues = ill_alloc(G_ALLOCATOR, sizeof(*frame_queues) * frames_in_flight);
        if (!frame_queues)
        {
            JDM_ERROR("Could not allocate memory for frame job queues");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        for (unsigned i = 0; i < frames_in_flight; ++i)
        {
            res = jta_frame_job_queue_create(32, frame_queues + i);
            if (res != GFX_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not create frame queue at index %u out of %u", i, frames_in_flight);
                for (unsigned j = 0; j < i; ++j)
                {
                    jta_frame_job_queue_destroy(frame_queues[j]);
                }
                ill_jfree(G_ALLOCATOR, frame_queues);
                frame_queues = NULL;
                goto failed;
            }
        }
    }

    this->frame_queues = frame_queues;
    this->frames_in_flight = frames_in_flight;
    this->fen_swap = fen_swap;
    this->sem_present = sem_present;
    this->sem_available = sem_available;
    this->depth_img = depth_img;
    this->depth_view = depth_view;
    this->depth_fmt = depth_format;
    this->cmd_buffers = cmd_buffers;
    this->cmd_pool = cmd_pool;
    this->window_extent = extent;
    this->window_format = surface_format;
    this->image_views = views;
    this->image_count = n_images;
    this->swapchain = swapchain;
    this->has_stencil = has_stencil;
    this->current_img = 0;
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

failed:
    if (frame_queues)
    {
        for (unsigned i = 0; i < frames_in_flight; ++i)
        {
            jta_frame_job_queue_destroy(frame_queues[i]);
        }
        ill_jfree(G_ALLOCATOR, frame_queues);
    }
    if (fen_swap != VK_NULL_HANDLE)
    {
        for (unsigned i = 0; i < frames_in_flight; ++i)
        {
            vkDestroyFence(device, fen_swap[i], NULL);
        }
        ill_jfree(G_ALLOCATOR, fen_swap);
    }
    if (sem_available != VK_NULL_HANDLE)
    {
        for (unsigned i = 0; i < frames_in_flight; ++i)
        {
            vkDestroySemaphore(device, sem_available[i], NULL);
        }
        ill_jfree(G_ALLOCATOR, sem_available);
    }
    if (sem_present != VK_NULL_HANDLE)
    {
        for (unsigned i = 0; i < frames_in_flight; ++i)
        {
            vkDestroySemaphore(device, sem_present[i], NULL);
        }
        ill_jfree(G_ALLOCATOR, sem_present);
    }
    if (cmd_buffers != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(device, cmd_pool, frames_in_flight, cmd_buffers);
        ill_jfree(G_ALLOCATOR, cmd_buffers);
    }
    if (depth_view != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, depth_view, NULL);
    }
    if (depth_img != VK_NULL_HANDLE)
    {
        jvm_image_destroy(depth_img);
    }
    if (cmd_pool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(device, cmd_pool, NULL);
    }
    if (views)
    {
        for (unsigned i = 0; i < n_images; ++i)
        {
            vkDestroyImageView(device, views[i], NULL);
        }
        ill_jfree(G_ALLOCATOR, views);
    }
    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapchain, NULL);
    }
    lin_allocator_restore_current(G_LIN_ALLOCATOR, base);
    JDM_LEAVE_FUNCTION;
    return res;
}

static void destroy_swapchain(VkDevice device, jta_vulkan_swapchain* this)
{
    JDM_ENTER_FUNCTION;
    for (unsigned i = 0; i < this->frames_in_flight; ++i)
    {
        jta_frame_job_queue_destroy(this->frame_queues[i]);
    }
    ill_jfree(G_ALLOCATOR, this->frame_queues);
    for (unsigned i = 0; i < this->frames_in_flight; ++i)
    {
        vkDestroyFence(device, this->fen_swap[i], NULL);
    }
    ill_jfree(G_ALLOCATOR, this->fen_swap);
    for (unsigned i = 0; i < this->frames_in_flight; ++i)
    {
        vkDestroySemaphore(device, this->sem_available[i], NULL);
    }
    ill_jfree(G_ALLOCATOR, this->sem_available);
    for (unsigned i = 0; i < this->frames_in_flight; ++i)
    {
        vkDestroySemaphore(device, this->sem_present[i], NULL);
    }
    ill_jfree(G_ALLOCATOR, this->sem_present);
    vkFreeCommandBuffers(device, this->cmd_pool, this->frames_in_flight, this->cmd_buffers);
    ill_jfree(G_ALLOCATOR, this->cmd_buffers);
    vkDestroyImageView(device, this->depth_view, NULL);

    jvm_image_destroy(this->depth_img);

    vkDestroyCommandPool(device, this->cmd_pool, NULL);


    for (unsigned i = 0; i < this->image_count; ++i)
    {
        vkDestroyImageView(device, this->image_views[i], NULL);
    }
    ill_jfree(G_ALLOCATOR, this->image_views);

    vkDestroySwapchainKHR(device, this->swapchain, NULL);

    JDM_LEAVE_FUNCTION;
}

static gfx_result render_pass_state_create(
        VkDevice device, VkRenderPass render_pass, const jta_vulkan_swapchain* swapchain, jta_vulkan_render_pass* this,
        int has_depth)
{
    JDM_ENTER_FUNCTION;
    gfx_result res;
    VkFramebuffer* framebuffers = ill_alloc(G_ALLOCATOR, sizeof(VkFramebuffer) * swapchain->image_count);
    if (!framebuffers)
    {
        JDM_ERROR("Could not allocate memory for framebuffer array");
        res = GFX_RESULT_BAD_ALLOC;
        goto failed;
    }
    VkImageView attachments[2] =
            {
                    NULL, swapchain->depth_view
            };
    VkFramebufferCreateInfo create_info =
            {
                    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                    .renderPass = render_pass,
                    .attachmentCount = has_depth ? 2 : 1,
                    .pAttachments = attachments,
                    .width = swapchain->window_extent.width,
                    .height = swapchain->window_extent.height,
                    .layers = 1
            };
    VkResult vk_res;
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        attachments[0] = swapchain->image_views[i];
        vk_res = vkCreateFramebuffer(device, &create_info, NULL, framebuffers + i);
        if (vk_res != VK_SUCCESS)
        {
            for (unsigned j = 0; j < i; ++j)
            {
                vkDestroyFramebuffer(device, framebuffers[j], NULL);
            }
            JDM_ERROR("Could not create framebuffer %u out of %u, reason: %s(%d)", i + 1, swapchain->image_count,
                      vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_FRAMEBUFFER;
            ill_jfree(G_ALLOCATOR, framebuffers);
            framebuffers = NULL;
            goto failed;
        }
    }
    this->framebuffer_count = swapchain->image_count;
    this->framebuffers = framebuffers;

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

failed:
    ill_jfree(G_ALLOCATOR, framebuffers);
    JDM_LEAVE_FUNCTION;
    return res;
}

static gfx_result render_pass_state_destroy(VkDevice device, jta_vulkan_render_pass* this)
{
    for (unsigned i = 0; i < this->framebuffer_count; ++i)
    {
        vkDestroyFramebuffer(device, this->framebuffers[i], NULL);
    }
    ill_jfree(G_ALLOCATOR, this->framebuffers);
    return GFX_RESULT_SUCCESS;
}

static gfx_result queue_create(VkDevice dev, uint32_t idx, jta_vulkan_queue* this, const char* purpose)
{
    JDM_ENTER_FUNCTION;
    vkGetDeviceQueue(dev, idx, 0, &this->handle);
    VkCommandPoolCreateInfo pool_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
            .queueFamilyIndex = idx,
            };
    VkResult vk_res = vkCreateCommandPool(dev, &pool_info, NULL, &this->transient_pool);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not create command pool for %s queue, reason: %s(%d)", purpose, vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }
    VkFenceCreateInfo fence_info =
            {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
//            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
    vk_res = vkCreateFence(dev, &fence_info, NULL, &this->fence);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not create fence for %s queue, reason: %s(%d)", purpose, vk_result_to_str(vk_res), vk_res);
        vkDestroyCommandPool(dev, this->transient_pool, NULL);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

static gfx_result queue_destroy(VkDevice dev, jta_vulkan_queue* this)
{
    JDM_ENTER_FUNCTION;

    vkQueueWaitIdle(this->handle);
    vkDestroyCommandPool(dev, this->transient_pool, NULL);
    vkDestroyFence(dev, this->fence, NULL);

    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

static void jvm_error_report_fn(void* state, const char* msg, const char* file, int line, const char* function)
{
    (void) state;
    JDM_ERROR("JVM module error (%s:%d - %s): \"%s\"", file, line, function, msg);
}

gfx_result
jta_vulkan_window_context_create(jwin_window* win, jta_vulkan_context* ctx, jta_vulkan_window_context** p_out)
{
    JDM_ENTER_FUNCTION;
    jta_vulkan_window_context* this = ill_alloc(G_ALLOCATOR, sizeof(*this));
    if (!this)
    {
        JDM_ERROR("Failed allocating memory for vulkan window context");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_ALLOC;
    }
    gfx_result res = GFX_RESULT_SUCCESS;
#ifndef NDEBUG
    memset(this, 0xCC, sizeof(*this));
#endif
    this->ctx = ctx;
    this->window = win;
    
    //  Create window surface
    void* const base = lin_allocator_save_state(G_LIN_ALLOCATOR);

    VkResult vk_res;
    int swapchain_done = 0;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPipelineLayout layout_3d = VK_NULL_HANDLE;
    VkPipeline pipeline_mesh = VK_NULL_HANDLE;
    VkPipeline pipeline_cf = VK_NULL_HANDLE;
    int32_t i_gfx_queue;
    jta_vulkan_queue vk_queue_gfx;
    int32_t i_prs_queue;
    jta_vulkan_queue vk_queue_prs;
    int32_t i_trs_queue;
    jta_vulkan_queue vk_queue_trs;
    VkSampleCountFlags samples;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    jvm_allocator* allocator = NULL;
    VkRenderPass render_pass_mesh = VK_NULL_HANDLE;
    jta_vulkan_render_pass pass_mesh;
    VkRenderPass render_pass_cf = VK_NULL_HANDLE;
    jta_vulkan_render_pass pass_cf;
    jvm_buffer_allocation* transfer_buffer;

    VkPipelineLayout layout_2d = VK_NULL_HANDLE;
    VkPipeline pipeline_ui = VK_NULL_HANDLE;
    VkRenderPass render_pass_ui = VK_NULL_HANDLE;
    jta_vulkan_render_pass pass_ui;

    VkDescriptorSetLayout desc_set_layout_ui = VK_NULL_HANDLE;
    VkDescriptorPool ui_desc_pool = VK_NULL_HANDLE;


    //  Ask jwin to create the window surface
    const jwin_result jwin_res = jwin_window_create_window_vk_surface(ctx->instance, win, NULL, NULL, &vk_res, &surface);
    if (jwin_res != JWIN_RESULT_SUCCESS)
    {
        JDM_ERROR("jwin window failed at creating window surface, reason: %s (Vulkan error %s(%d))",
                  jwin_result_msg_str(jwin_res), vk_result_to_str(vk_res), vk_res);
        res = GFX_RESULT_BAD_VK_CALL;
        goto failed;
    }


    //  Find the physical device which supports the appropriate formats, get its queues and the logical interface
    {
        //  List all physical devices
        uint32_t physical_device_count;
        vk_res = vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not enumerate physical devices, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
        VkPhysicalDevice* const devices = lin_alloc(G_LIN_ALLOCATOR, sizeof(VkPhysicalDevice) * physical_device_count);
        if (!devices)
        {
            JDM_ERROR("Could not allocate buffer for physical device list");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        vk_res = vkEnumeratePhysicalDevices(ctx->instance, &physical_device_count, devices);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not enumerate physical devices, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        //  Find the max number of extensions and queue families
        uint32_t max_extensions = 0, max_queues = 0;
        for (uint32_t i = 0; i < physical_device_count; ++i)
        {
            uint32_t n;
            vk_res = vkEnumerateDeviceExtensionProperties(devices[i], NULL, &n, NULL);
            if (vk_res == VK_SUCCESS && n > max_extensions)
            {
                max_extensions = n;
            }
            vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &n, NULL);
            if (n > max_queues)
            {
                max_queues = n;
            }
        }

        assert(max_queues != 0);

        //  Allocate buffers to hold these extensions and queue families
        VkExtensionProperties* const extensions = lin_alloc(G_LIN_ALLOCATOR, sizeof(*extensions) * max_extensions);
        if (!extensions)
        {
            JDM_ERROR("Could not allocate memory for device extension array");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
        VkQueueFamilyProperties* const queues = lin_alloc(G_LIN_ALLOCATOR, sizeof(*queues) * max_queues);
        if (!queues)
        {
            JDM_ERROR("Could not allocate memory for device queues");
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }

        int32_t best_score = -1;

        for (uint32_t i = 0; i < physical_device_count; ++i)
        {
            int32_t score = -1, gfx = -1, prs = -1, trs = -1;
            VkSampleCountFlagBits s;
            res = score_physical_device(devices[i], surface, &score, max_extensions, extensions, &gfx, &prs, &trs, max_queues, queues, &s);
            if (res == GFX_RESULT_SUCCESS && score > best_score)
            {
                best_score = score;
                i_gfx_queue = gfx;
                i_prs_queue = prs;
                i_trs_queue = trs;
                samples = s;
                physical_device = devices[i];
            }
        }

        lin_jfree(G_LIN_ALLOCATOR, queues);
        lin_jfree(G_LIN_ALLOCATOR, extensions);
        lin_jfree(G_LIN_ALLOCATOR, devices);

        if (best_score == -1)
        {
            JDM_ERROR("No device was found suitable");
            res = GFX_RESULT_NO_DEVICES;
            goto failed;
        }
        assert(physical_device != VK_NULL_HANDLE);

        //  Create device queues
        const uint32_t queue_indices[] =
                {
                i_gfx_queue, i_trs_queue, i_prs_queue
                };
        static const uint32_t n_queues = sizeof(queue_indices) / sizeof(*queue_indices);
        VkDeviceQueueCreateInfo queue_create_info[n_queues];
        uint32_t unique_queues[n_queues];
        uint32_t unique_queue_count = 0;
        const float priority = 1.0f;    //  Here just because it has to be
        for (uint32_t i = 0, j; i < n_queues; ++i)
        {
            const uint32_t index = queue_indices[i];
            for (j = 0; j < unique_queue_count; ++j)
            {
                if (index == unique_queues[j]) break;
            }
            if (j == unique_queue_count)
            {
                assert(unique_queue_count <= n_queues);
                unique_queues[j] = index;
                queue_create_info[j] =
                        (VkDeviceQueueCreateInfo){
                                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                .queueFamilyIndex = index,
                                .queueCount = 1,
                                .pQueuePriorities = &priority,
                        };
                unique_queue_count += 1;
            }
        }
        VkPhysicalDeviceFeatures features = {};
        VkDeviceCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                        .queueCreateInfoCount = unique_queue_count,
                        .pQueueCreateInfos = queue_create_info,
                        .pEnabledFeatures = &features,
                        .ppEnabledExtensionNames = DEVICE_REQUIRED_NAMES,
                        .enabledExtensionCount = DEVICE_REQUIRED_COUNT,
                };
        vk_res = vkCreateDevice(physical_device, &create_info, NULL, &device);
        if (vk_res != VK_SUCCESS)
        {
            res = GFX_RESULT_BAD_VK_CALL;
            JDM_ERROR("Could not create Vulkan logical device interface, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            goto failed;
        }
        this->device = device;

        res = queue_create(device, i_gfx_queue, &vk_queue_gfx, "graphics");
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create graphics queue");
            goto failed;
        }
        res = queue_create(device, i_gfx_queue, &vk_queue_prs, "presentation");
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create presentation queue");
            goto failed;
        }
        res = queue_create(device, i_gfx_queue, &vk_queue_trs, "transfer");
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create transfer queue");
            goto failed;
        }
    }

    jvm_allocation_callbacks allocation_callbacks =
            {
            .state = G_ALLOCATOR,
            .allocate = (void* (*)(void*, uint64_t)) ill_alloc,
            .free = (void (*)(void*, void*)) ill_jfree,
            .reallocate = (void* (*)(void*, void*, uint64_t)) ill_jrealloc,
            };
    (void)allocation_callbacks;

    jvm_error_callbacks error_callbacks =
            {
            .state = NULL,
            .report = jvm_error_report_fn,
            };

    //  Create buffer allocator
    jvm_allocator_create_info allocator_create_info =
            {
            .automatically_free_unused = 1,
            .min_allocation_size = 1024,
            .device = device,
            .physical_device = physical_device,
            .min_pool_size = 0,
#ifndef NDEBUG
            .allocation_callbacks = NULL,
#else
            .allocation_callbacks = &allocation_callbacks,
#endif
            .error_callbacks = &error_callbacks,
            };
    vk_res = jvm_allocator_create(allocator_create_info, NULL, &allocator);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not create buffer allocator, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
        res = GFX_RESULT_BAD_ALLOC;
        goto failed;
    }

    //  Create the swapchain
    jta_vulkan_swapchain swapchain;
    res = create_swapchain(win, physical_device, surface, device, &swapchain, i_gfx_queue, i_prs_queue, 2, allocator);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not create window's swapchain, reason: %s", gfx_result_to_str(res));
        goto failed;
    }
    swapchain_done = 1;

    //  Create the pipeline layout for 3D rendering
    {
        VkPushConstantRange push_constant_3d_ubo =
                {
                    .offset = 0,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .size = sizeof(ubo_3d),
                };
        VkPipelineLayoutCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                        .pushConstantRangeCount = 1,
                        .pPushConstantRanges = &push_constant_3d_ubo,
                };

        vk_res = vkCreatePipelineLayout(device, &create_info, NULL, &layout_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create pipeline layout for 3d rendering, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
    }

    //  Create the render pass for the 3d model drawing
    {
        VkAttachmentDescription attachments[] =
                {
                [0] = //    color attachment
                        {
                        .format = swapchain.window_format.format,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        },
                [1] = //    depth attachment
                        {
                        .format = swapchain.depth_fmt,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = swapchain.has_stencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                        },
                };
        VkAttachmentReference attachment_references[] =
                {
                [0] = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,},
                [1] = {.attachment = 1, .layout = swapchain.has_stencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL},
                };
        VkSubpassDescription subpass_description =
                {
                .colorAttachmentCount = 1,
                .pColorAttachments = attachment_references + 0,
                .pDepthStencilAttachment = attachment_references + 1,
                };
        VkSubpassDependency subpass_dependency =
                {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = 0,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                };
        VkRenderPassCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = 2,
                .pAttachments = attachments,
                .dependencyCount = 1,
                .pDependencies = &subpass_dependency,
                .subpassCount = 1,
                .pSubpasses = &subpass_description,
                };
        vk_res = vkCreateRenderPass(device, &create_info, NULL, &render_pass_mesh);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create mesh render pass, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
        res = render_pass_state_create(device, render_pass_mesh, &swapchain, &pass_mesh, 1);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create render pass state, reason: %s", gfx_result_to_str(res));
            vkDestroyRenderPass(device, render_pass_mesh, NULL);
            render_pass_mesh = VK_NULL_HANDLE;
            goto failed;
        }
    }

    //  Coordinate frame pass
    {
        VkAttachmentDescription color_attach_description =
                {
                        .format = swapchain.window_format.format,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentReference color_attach_ref =
                {
                        .attachment = 0,
                        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentDescription depth_attach_description =
                {
                        .format = swapchain.depth_fmt,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentReference depth_attach_ref =
                {
                        .attachment = 1,
                        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                };
        VkSubpassDescription subpass_description_cf =
                {
                        .colorAttachmentCount = 1,
                        .pColorAttachments = &color_attach_ref,
                        .pDepthStencilAttachment = &depth_attach_ref,
                };
        VkSubpassDependency subpass_dependency_cf =
                {
                        .srcSubpass = VK_SUBPASS_EXTERNAL,
                        .dstSubpass = 0,
                        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        .srcAccessMask = 0,
                        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                };
        VkAttachmentDescription attachment_array_cf[] =
                {
                        color_attach_description, depth_attach_description
                };
        VkRenderPassCreateInfo render_pass_info_cf =
                {
                        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                        .attachmentCount = 2,
                        .pAttachments = attachment_array_cf,
                        .dependencyCount = 1,
                        .pDependencies = &subpass_dependency_cf,
                        .subpassCount = 1,
                        .pSubpasses = &subpass_description_cf,
                };
        vk_res = vkCreateRenderPass(device, &render_pass_info_cf, NULL, &render_pass_cf);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create 3d render pass: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_RENDER_PASS;
            goto failed;
        }

        res = render_pass_state_create(device, render_pass_cf, &swapchain, &pass_cf, 1);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create render pass state, reason: %s", gfx_result_to_str(res));
            vkDestroyRenderPass(device, render_pass_cf, NULL);
            render_pass_cf = VK_NULL_HANDLE;
            goto failed;
        }
    }

    //  Create the 3d pipeline
    {
        VkShaderModule module_vtx_3d, module_frg_3d;
        VkViewport viewport = {.x = 0.0f, .y = 0.0f, .width = (f32)swapchain.window_extent.width, .height = (f32)swapchain.window_extent.height,
                .minDepth = 0.0f, .maxDepth = 1.0f};
        VkRect2D scissors = {.offset = {0, 0}, .extent = swapchain.window_extent};
        this->viewport = viewport;
        this->scissor = scissors;
        VkVertexInputBindingDescription vtx_binding_desc_geometry =
                {
                        .binding = 0,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                        .stride = sizeof(jta_vertex),
                };
        VkVertexInputBindingDescription vtx_binding_desc_model =
                {
                        .binding = 1,
                        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
                        .stride = sizeof(jta_model_data),
                };
        VkVertexInputAttributeDescription position_attribute_description =
                {
                        .binding = 0,
                        .location = 0,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = offsetof(jta_vertex, x),
                };
        VkVertexInputAttributeDescription normal_attribute_description =
                {
                        .binding = 0,
                        .location = 1,
                        .format = VK_FORMAT_R32G32B32_SFLOAT,
                        .offset = offsetof(jta_vertex, nx),
                };
        VkVertexInputAttributeDescription color_attribute_description =
                {
                        .binding = 1,
                        .location = 2,
                        .format = VK_FORMAT_R8G8B8A8_UNORM,
                        .offset = offsetof(jta_model_data, color),
                };
        VkVertexInputAttributeDescription model_transform_attribute_description_col1 =
                {
                        .binding = 1,
                        .location = 3,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, model_data[0]),
                };
        VkVertexInputAttributeDescription model_transform_attribute_description_col2 =
                {
                        .binding = 1,
                        .location = 4,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, model_data[4]),
                };
        VkVertexInputAttributeDescription model_transform_attribute_description_col3 =
                {
                        .binding = 1,
                        .location = 5,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, model_data[8]),
                };
        VkVertexInputAttributeDescription model_transform_attribute_description_col4 =
                {
                        .binding = 1,
                        .location = 6,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, model_data[12]),
                };
        VkVertexInputAttributeDescription normal_transform_attribute_description_col1 =
                {
                        .binding = 1,
                        .location = 7,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, normal_data[0]),
                };
        VkVertexInputAttributeDescription normal_transform_attribute_description_col2 =
                {
                        .binding = 1,
                        .location = 8,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, normal_data[4]),
                };
        VkVertexInputAttributeDescription normal_transform_attribute_description_col3 =
                {
                        .binding = 1,
                        .location = 9,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, normal_data[8]),
                };
        VkVertexInputAttributeDescription normal_transform_attribute_description_col4 =
                {
                        .binding = 1,
                        .location = 10,
                        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                        .offset = offsetof(jta_model_data, normal_data[12]),
                };
        VkShaderModuleCreateInfo shader_vtx_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = vtx_shader3d_vert_size,
                        .pCode = vtx_shader3d_vert,
                };
        VkShaderModuleCreateInfo shader_frg_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = frg_shader3d_frag_size,
                        .pCode = frg_shader3d_frag,
                };
        vk_res = vkCreateShaderModule(device, &shader_vtx_create_info, NULL, &module_vtx_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create vertex shader module (3d), reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_VTX_SHADER;
            goto failed;
        }
        vk_res = vkCreateShaderModule(device, &shader_frg_create_info, NULL, &module_frg_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create fragment shader module (3d), reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            vkDestroyShaderModule(device, module_vtx_3d, NULL);
            res = GFX_RESULT_NO_FRG_SHADER;
            goto failed;
        }

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_vtx_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = module_vtx_3d,
                        .pName = "main",
                };

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_frg_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = module_frg_3d,
                        .pName = "main",
                };
        VkPipelineShaderStageCreateInfo shader_stage_info_array[] =
                {
                        pipeline_shader_stage_vtx_info, pipeline_shader_stage_frg_info
                };
        VkDynamicState dynamic_states[] =
                {
                        VK_DYNAMIC_STATE_VIEWPORT,
                        VK_DYNAMIC_STATE_SCISSOR,
                };
        VkPipelineDynamicStateCreateInfo dynamic_state_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                        .dynamicStateCount = sizeof(dynamic_states) / sizeof(*dynamic_states),
                        .pDynamicStates = dynamic_states,
                };
        VkVertexInputAttributeDescription attrib_description_array[] =
                {
                        position_attribute_description, normal_attribute_description, color_attribute_description, model_transform_attribute_description_col1, model_transform_attribute_description_col2, model_transform_attribute_description_col3, model_transform_attribute_description_col4,
                        normal_transform_attribute_description_col1, normal_transform_attribute_description_col2, normal_transform_attribute_description_col3, normal_transform_attribute_description_col4
                };
        VkVertexInputBindingDescription binding_description_array[] =
                {
                        vtx_binding_desc_geometry, vtx_binding_desc_model
                };
        VkPipelineVertexInputStateCreateInfo vtx_state_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                        .vertexAttributeDescriptionCount = sizeof(attrib_description_array) / sizeof(*attrib_description_array),
                        .pVertexAttributeDescriptions = attrib_description_array,
                        .vertexBindingDescriptionCount = sizeof(binding_description_array) / sizeof(*binding_description_array),
                        .pVertexBindingDescriptions = binding_description_array,
                };
        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                        .primitiveRestartEnable = VK_FALSE,
                        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                };
        VkPipelineViewportStateCreateInfo viewport_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                        .viewportCount = 1,
                        .pViewports = &viewport,
                        .scissorCount = 1,
                        .pScissors = &scissors,
                };
        VkPipelineRasterizationStateCreateInfo rasterizer_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                        .depthClampEnable = VK_FALSE,
                        .rasterizerDiscardEnable = VK_FALSE,
                        .polygonMode = VK_POLYGON_MODE_FILL,
                        .lineWidth = 1.0f,
                        .cullMode = VK_CULL_MODE_BACK_BIT,
                        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                        .depthBiasEnable = VK_FALSE,
                };
        VkPipelineMultisampleStateCreateInfo ms_state_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                        .sampleShadingEnable = VK_FALSE,
                        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                };
        VkPipelineColorBlendAttachmentState cb_attachment_state =
                {
                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT,
                        .blendEnable = VK_FALSE,
                };
        VkPipelineColorBlendStateCreateInfo cb_state_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                        .logicOpEnable = VK_FALSE,
                        .attachmentCount = 1,
                        .pAttachments = &cb_attachment_state,
                };
        VkPipelineDepthStencilStateCreateInfo dp_state_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                        .depthTestEnable = VK_TRUE,
                        .depthWriteEnable = VK_TRUE,
                        .depthCompareOp = VK_COMPARE_OP_LESS,
                        .depthBoundsTestEnable = VK_FALSE,
                        .maxDepthBounds = 1.0f,
                        .minDepthBounds = 0.01f,
                        .stencilTestEnable = VK_FALSE,
                };
        VkGraphicsPipelineCreateInfo create_info_mesh =
                {
                        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                        .stageCount = 2,
                        .pStages = shader_stage_info_array,
                        .pVertexInputState = &vtx_state_create_info,
                        .pInputAssemblyState = &input_assembly_create_info,
                        .pViewportState = &viewport_create_info,
                        .pRasterizationState = &rasterizer_create_info,
                        .pMultisampleState = &ms_state_create_info,
                        .pDepthStencilState = &dp_state_info,
                        .pColorBlendState = &cb_state_create_info,
                        .pDynamicState = &dynamic_state_info,
                        .layout = layout_3d,
                        .renderPass = render_pass_mesh,
                        .subpass = 0,
                        .basePipelineHandle = VK_NULL_HANDLE,
                        .basePipelineIndex = -1,
                };
        VkGraphicsPipelineCreateInfo create_info_cf =
                {
                        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                        .stageCount = 2,
                        .pStages = shader_stage_info_array,
                        .pVertexInputState = &vtx_state_create_info,
                        .pInputAssemblyState = &input_assembly_create_info,
                        .pViewportState = &viewport_create_info,
                        .pRasterizationState = &rasterizer_create_info,
                        .pMultisampleState = &ms_state_create_info,
                        .pDepthStencilState = &dp_state_info,
                        .pColorBlendState = &cb_state_create_info,
                        .pDynamicState = &dynamic_state_info,
                        .layout = layout_3d,
                        .renderPass = render_pass_cf,
                        .subpass = 0,
                        .basePipelineHandle = VK_NULL_HANDLE,
                        .basePipelineIndex = -1,
                };
        VkGraphicsPipelineCreateInfo create_info_array[2] =
                {
                [0] = create_info_mesh,
                [1] = create_info_cf,
                };
        VkPipeline pipelines[2];
        vk_res = vkCreateGraphicsPipelines(device, NULL, 2, create_info_array, NULL, pipelines);
        vkDestroyShaderModule(device, module_frg_3d, NULL);
        vkDestroyShaderModule(device, module_vtx_3d, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating the graphics pipelines, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            vkDestroyShaderModule(device, module_frg_3d, NULL);
            vkDestroyShaderModule(device, module_vtx_3d, NULL);
            res = GFX_RESULT_NO_PIPELINE;
            goto failed;
        }
        pipeline_mesh = pipelines[0];
        pipeline_cf = pipelines[1];
    }
    pass_cf.pipeline = pipeline_cf;
    pass_mesh.pipeline = pipeline_mesh;

    //  Transfer data
    {
        VkBufferCreateInfo transfer_buffer_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .size = 1 << 20,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                };
        vk_res = jvm_buffer_create(allocator, &transfer_buffer_create_info, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 0, 1, &transfer_buffer);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not allocate memory needed by transfer buffer, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_ALLOC;
            goto failed;
        }
    }


    //  Create the pipeline layout for UI rendering
    {
        VkPushConstantRange push_constant_ranges_2d[] =
                {
                [0] =   {
                         .offset = 0,
                         .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                         .size = sizeof(ubo_ui_vtx),
                        },
                [1] =   {
                                .offset = sizeof(ubo_ui_vtx),
                                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                                .size = sizeof(ubo_ui_frg),
                        },
                };
        VkDescriptorSetLayoutBinding set_layout =
                {
                        .binding = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                };
        VkDescriptorSetLayoutCreateInfo layout_info =
                {
                        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                        .bindingCount = 1,
                        .pBindings = &set_layout,
                };
        vk_res = vkCreateDescriptorSetLayout(device, &layout_info, NULL, &desc_set_layout_ui);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create descriptor set layout for ui rendering, reason: %s(%d)",
                      vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }

        VkPipelineLayoutCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                        .pushConstantRangeCount = sizeof(push_constant_ranges_2d) / sizeof(*push_constant_ranges_2d),
                        .pPushConstantRanges = push_constant_ranges_2d,
                        .setLayoutCount = 1,
                        .pSetLayouts = &desc_set_layout_ui,
                };

        vk_res = vkCreatePipelineLayout(device, &create_info, NULL, &layout_2d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create pipeline layout for ui rendering, reason: %s(%d)", vk_result_to_str(vk_res),
                      vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
    }
    
    //  Create descriptor sets and pools
    {
        VkDescriptorPoolSize pool_size =
                {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                };
        VkDescriptorPoolCreateInfo pool_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .maxSets = 1,
                .pPoolSizes = &pool_size,
                .poolSizeCount = 1,
                };
        vk_res = vkCreateDescriptorPool(device, &pool_create_info, NULL, &ui_desc_pool);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create descriptor pool for ui rendering, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
        VkDescriptorSetAllocateInfo allocate_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorSetCount = 1,
                .descriptorPool = ui_desc_pool,
                .pSetLayouts = &desc_set_layout_ui
                };
        vk_res = vkAllocateDescriptorSets(device, &allocate_info, &this->descriptor_ui);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not allocate descriptor for ui rendering, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
    }

    //  Create the render pass for the UI drawing
    {
        //    color attachment
        VkAttachmentDescription color_attachment_desc =
                {
                        .format = swapchain.window_format.format,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
//                        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//                        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                };
        VkAttachmentReference color_attach_ref =
                {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
        VkSubpassDescription subpass_description =
                {
                        .colorAttachmentCount = 1,
                        .pColorAttachments = &color_attach_ref,
                };
        VkSubpassDependency subpass_dependency =
                {
                        .srcSubpass = VK_SUBPASS_EXTERNAL,
                        .dstSubpass = 0,
                        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        .srcAccessMask = 0,
                        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                };
        VkRenderPassCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                        .attachmentCount = 1,
                        .pAttachments = &color_attachment_desc,
                        .dependencyCount = 1,
                        .pDependencies = &subpass_dependency,
                        .subpassCount = 1,
                        .pSubpasses = &subpass_description,
                };
        vk_res = vkCreateRenderPass(device, &create_info, NULL, &render_pass_ui);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create mesh render pass, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_BAD_VK_CALL;
            goto failed;
        }
        res = render_pass_state_create(device, render_pass_ui, &swapchain, &pass_ui, 0);
        if (res != GFX_RESULT_SUCCESS)
        {
            JDM_ERROR("Could not create render pass state, reason: %s", gfx_result_to_str(res));
            vkDestroyRenderPass(device, render_pass_mesh, NULL);
            render_pass_mesh = VK_NULL_HANDLE;
            goto failed;
        }
    }

    //  Create the UI pipeline
    {
        VkShaderModule module_vtx_ui, module_frg_ui;
        VkVertexInputBindingDescription vtx_binding_desc_geometry =
                {
                        .binding = 0,
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
                        .stride = sizeof(jrui_vertex),
                };
        VkVertexInputAttributeDescription position_attribute_description =
                {
                        .binding = 0,
                        .location = 0,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(jrui_vertex, x),
                };
        VkVertexInputAttributeDescription tex_coords_attribute_description =
                {
                        .binding = 0,
                        .location = 1,
                        .format = VK_FORMAT_R32G32_SFLOAT,
                        .offset = offsetof(jrui_vertex, u),
                };
        VkVertexInputAttributeDescription tex_idx_attribute_description =
                {
                        .binding = 0,
                        .location = 2,
                        .format = VK_FORMAT_R32_SINT,
                        .offset = offsetof(jrui_vertex, texture_idx),
                };
        VkVertexInputAttributeDescription color_attribute_description =
                {
                        .binding = 0,
                        .location = 3,
                        .format = VK_FORMAT_R8G8B8A8_UNORM,
                        .offset = offsetof(jrui_vertex, color),
                };
        VkShaderModuleCreateInfo shader_vtx_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = vtx_shader_ui_vert_size,
                        .pCode = vtx_shader_ui_vert,
                };
        VkShaderModuleCreateInfo shader_frg_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                        .codeSize = frg_shader_ui_frag_size,
                        .pCode = frg_shader_ui_frag,
                };
        vk_res = vkCreateShaderModule(device, &shader_vtx_create_info, NULL, &module_vtx_ui);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create vertex shader module (UI), reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_VTX_SHADER;
            goto failed;
        }
        vk_res = vkCreateShaderModule(device, &shader_frg_create_info, NULL, &module_frg_ui);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create fragment shader module (UI), reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            vkDestroyShaderModule(device, module_vtx_ui, NULL);
            res = GFX_RESULT_NO_FRG_SHADER;
            goto failed;
        }

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_vtx_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_VERTEX_BIT,
                        .module = module_vtx_ui,
                        .pName = "main",
                };

        VkPipelineShaderStageCreateInfo pipeline_shader_stage_frg_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                        .module = module_frg_ui,
                        .pName = "main",
                };
        VkPipelineShaderStageCreateInfo shader_stage_info_array[] =
                {
                        pipeline_shader_stage_vtx_info, pipeline_shader_stage_frg_info
                };
        VkDynamicState dynamic_states[] =
                {
                        VK_DYNAMIC_STATE_VIEWPORT,
                        VK_DYNAMIC_STATE_SCISSOR,
                };
        VkPipelineDynamicStateCreateInfo dynamic_state_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                        .dynamicStateCount = sizeof(dynamic_states) / sizeof(*dynamic_states),
                        .pDynamicStates = dynamic_states,
                };
        VkVertexInputAttributeDescription attrib_description_array[] =
                {
                        position_attribute_description, tex_coords_attribute_description, tex_idx_attribute_description, color_attribute_description,
                };
        VkVertexInputBindingDescription binding_description_array[] =
                {
                        vtx_binding_desc_geometry
                };
        VkPipelineVertexInputStateCreateInfo vtx_state_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                        .vertexAttributeDescriptionCount = sizeof(attrib_description_array) / sizeof(*attrib_description_array),
                        .pVertexAttributeDescriptions = attrib_description_array,
                        .vertexBindingDescriptionCount = sizeof(binding_description_array) / sizeof(*binding_description_array),
                        .pVertexBindingDescriptions = binding_description_array,
                };
        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                        .primitiveRestartEnable = VK_FALSE,
                        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                };
        VkPipelineViewportStateCreateInfo viewport_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                        .viewportCount = 1,
                        .pViewports = &this->viewport,
                        .scissorCount = 1,
                        .pScissors = &this->scissor,
                };
        VkPipelineRasterizationStateCreateInfo rasterizer_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                        .depthClampEnable = VK_FALSE,
                        .rasterizerDiscardEnable = VK_FALSE,
                        .polygonMode = VK_POLYGON_MODE_FILL,
                        .lineWidth = 1.0f,
                        .cullMode = VK_CULL_MODE_BACK_BIT,
                        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                        .depthBiasEnable = VK_FALSE,
                };
        VkPipelineMultisampleStateCreateInfo ms_state_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                        .sampleShadingEnable = VK_FALSE,
                        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                };
        VkPipelineColorBlendAttachmentState cb_attachment_state =
                {
                        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT,
                        .blendEnable = VK_TRUE,
                        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                        .colorBlendOp = VK_BLEND_OP_ADD,
                        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                        .alphaBlendOp = VK_BLEND_OP_ADD,
                };
        VkPipelineColorBlendStateCreateInfo cb_state_create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                        .logicOpEnable = VK_FALSE,
                        .attachmentCount = 1,
                        .pAttachments = &cb_attachment_state,
                };
        VkPipelineDepthStencilStateCreateInfo ds_state =
                {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                    .depthBoundsTestEnable = VK_FALSE,
                    .depthTestEnable = VK_FALSE,
                    .depthWriteEnable = VK_FALSE,
                    .stencilTestEnable = VK_FALSE,
                };
        VkGraphicsPipelineCreateInfo create_info_ui =
                {
                        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                        .stageCount = 2,
                        .pStages = shader_stage_info_array,
                        .pVertexInputState = &vtx_state_create_info,
                        .pInputAssemblyState = &input_assembly_create_info,
                        .pViewportState = &viewport_create_info,
                        .pRasterizationState = &rasterizer_create_info,
                        .pMultisampleState = &ms_state_create_info,
                        .pDepthStencilState = &ds_state,
                        .pColorBlendState = &cb_state_create_info,
                        .pDynamicState = &dynamic_state_info,
                        .layout = layout_2d,
                        .renderPass = render_pass_ui,
                        .subpass = 0,
                        .basePipelineHandle = VK_NULL_HANDLE,
                        .basePipelineIndex = -1,
                };
        VkPipeline pipeline;
        vk_res = vkCreateGraphicsPipelines(device, NULL, 1, &create_info_ui, NULL, &pipeline);
        vkDestroyShaderModule(device, module_frg_ui, NULL);
        vkDestroyShaderModule(device, module_vtx_ui, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating the graphics pipelines, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
            res = GFX_RESULT_NO_PIPELINE;
            goto failed;
        }
        pipeline_ui = pipeline;
    }
    this->pipeline_ui = pipeline_ui;

    vkGetPhysicalDeviceProperties(physical_device, &this->device_properties);

    (void) samples;
    this->swapchain = swapchain;
    this->transfer_buffer = transfer_buffer;
    this->vulkan_allocator = allocator;
    this->pipeline_cf = pipeline_cf;
    this->pipeline_mesh = pipeline_mesh;
    this->render_pass_ui = render_pass_ui;
    this->pass_ui = pass_ui;
    this->pass_cf = pass_cf;
    this->render_pass_cf = render_pass_cf;
    this->pass_mesh = pass_mesh;
    this->render_pass_mesh = render_pass_mesh;
    this->layout_mesh = layout_3d;
    this->physical_device = physical_device;
    this->device = device;
    this->queue_graphics_data = vk_queue_gfx;
    this->queue_present_data = vk_queue_prs;
    this->queue_transfer_data = vk_queue_trs;
    this->window_surface = surface;

    this->layout_ui = layout_2d;
    this->descriptor_layout_ui = desc_set_layout_ui;
    this->desc_pool_ui = ui_desc_pool;

    *p_out = this;
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;

failed:
    if (desc_set_layout_ui != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(device, desc_set_layout_ui, NULL);
    }
    if (ui_desc_pool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, ui_desc_pool, NULL);
    }
    if (vk_queue_gfx.handle != VK_NULL_HANDLE)
    {
        queue_destroy(device, &vk_queue_gfx);
    }
    if (vk_queue_prs.handle != VK_NULL_HANDLE)
    {
        queue_destroy(device, &vk_queue_prs);
    }
    if (vk_queue_trs.handle != VK_NULL_HANDLE)
    {
        queue_destroy(device, &vk_queue_trs);
    }
    if (pipeline_cf != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline_cf, NULL);
    }
    if (pipeline_mesh != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline_mesh, NULL);
    }
    if (render_pass_cf != VK_NULL_HANDLE)
    {
        render_pass_state_destroy(device, &pass_cf);
        vkDestroyRenderPass(device, render_pass_cf, NULL);
    }
    if (render_pass_mesh != VK_NULL_HANDLE)
    {
        render_pass_state_destroy(device, &pass_mesh);
        vkDestroyRenderPass(device, render_pass_mesh, NULL);
    }
    if (layout_3d)
    {
        vkDestroyPipelineLayout(device, layout_3d, NULL);
    }
    if (swapchain_done)
    {
        destroy_swapchain(device, &swapchain);
    }
    if (allocator)
    {
        jvm_allocator_destroy(allocator);
    }
    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(ctx->instance, surface, NULL);
    }
    if (layout_2d != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, layout_2d, NULL);
    }
    if (pipeline_ui != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline_ui, NULL);
    }
    if (render_pass_ui != VK_NULL_HANDLE)
    {
        render_pass_state_destroy(device, &pass_ui);
        vkDestroyRenderPass(device, render_pass_ui, NULL);
    }

    lin_allocator_restore_current(G_LIN_ALLOCATOR, base);
    JDM_LEAVE_FUNCTION;
    return res;
}

void jta_vulkan_window_context_destroy(jta_vulkan_window_context* ctx)
{
    JDM_ENTER_FUNCTION;
    vkDeviceWaitIdle(ctx->device);

    //  Process all enqueued work in the swapchain
    for (unsigned i = 0; i < ctx->swapchain.frames_in_flight; ++i)
    {
        jta_frame_job_queue_execute(ctx->swapchain.frame_queues[i]);
    }

    vkDestroyDescriptorSetLayout(ctx->device, ctx->descriptor_layout_ui, NULL);
    vkDestroyDescriptorPool(ctx->device, ctx->desc_pool_ui, NULL);

    queue_destroy(ctx->device, &ctx->queue_graphics_data);
    queue_destroy(ctx->device, &ctx->queue_present_data);
    queue_destroy(ctx->device, &ctx->queue_transfer_data);
    vkDestroyPipelineLayout(ctx->device, ctx->layout_ui, NULL);
    vkDestroyPipeline(ctx->device, ctx->pipeline_ui, NULL);
    render_pass_state_destroy(ctx->device, &ctx->pass_ui);
    vkDestroyRenderPass(ctx->device, ctx->render_pass_ui, NULL);

    jvm_buffer_destroy(ctx->transfer_buffer);
    vkDestroyPipeline(ctx->device, ctx->pipeline_cf, NULL);
    vkDestroyPipeline(ctx->device, ctx->pipeline_mesh, NULL);
    render_pass_state_destroy(ctx->device, &ctx->pass_cf);
    vkDestroyRenderPass(ctx->device, ctx->render_pass_cf, NULL);
    render_pass_state_destroy(ctx->device, &ctx->pass_mesh);
    vkDestroyRenderPass(ctx->device, ctx->render_pass_mesh, NULL);
    vkDestroyPipelineLayout(ctx->device, ctx->layout_mesh, NULL);
    destroy_swapchain(ctx->device, &ctx->swapchain);
    jvm_allocator_destroy(ctx->vulkan_allocator);
    vkDestroySurfaceKHR(ctx->ctx->instance, ctx->window_surface, NULL);
    vkDestroyDevice(ctx->device, NULL);
    //    memset(ctx, 0xCC, sizeof(*ctx));
    ill_jfree(G_ALLOCATOR, ctx);
    JDM_LEAVE_FUNCTION;
}

gfx_result jta_vulkan_begin_draw(jta_vulkan_window_context* ctx, VkCommandBuffer* p_buffer, unsigned* p_img)
{
    JDM_ENTER_FUNCTION;
    gfx_result res;
    const unsigned i_frame = ctx->swapchain.current_img;
    VkResult vk_res = vkWaitForFences(ctx->device, 1, ctx->swapchain.fen_swap + i_frame, VK_TRUE, UINT64_MAX);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Failed waiting on fence for swap interval, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_FENCE_WAIT;
    }

    uint32_t image_index;
acquire:
    vk_res = vkAcquireNextImageKHR(ctx->device, ctx->swapchain.swapchain, UINT64_MAX, ctx->swapchain.sem_available[i_frame], VK_NULL_HANDLE, &image_index);
    switch (vk_res)
    {
    case VK_NOT_READY:
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_NOT_READY;
    case VK_SUCCESS: break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
        //  Must recreate the swapchain
        {

            jta_vulkan_swapchain new_swapchain;
            res = create_swapchain(
                    ctx->window,
                    ctx->physical_device,
                    ctx->window_surface,
                    ctx->device,
                    &new_swapchain,
                    ctx->queue_graphics_data.index,
                    ctx->queue_present_data.index,
                    2, ctx->vulkan_allocator);
            if (res != GFX_RESULT_SUCCESS)
            {
                JDM_ERROR("Could not recreate window swapchain, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
                JDM_LEAVE_FUNCTION;
                return res;
            }
            destroy_swapchain(ctx->device, &ctx->swapchain);
            ctx->swapchain = new_swapchain;
        }
        goto acquire;
    default:
        JDM_ERROR("Unexpected error acquiring swapchain image: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }
    ctx->swapchain.last_img = image_index;
    vk_res = vkResetFences(ctx->device, 1, ctx->swapchain.fen_swap + i_frame);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not reset fence for swap interval, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_FENCE_RESET;
    }


    //  Begin recording the correct command buffer
    VkCommandBuffer cmd_buffer = ctx->swapchain.cmd_buffers[i_frame];
    vk_res = vkResetCommandBuffer(cmd_buffer, 0);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not reset command buffer for rendering the current frame, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_CMDBUF_RESET;
    }
    VkCommandBufferBeginInfo begin_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pInheritanceInfo = NULL,
            .flags = 0,
            };
    vk_res = vkBeginCommandBuffer(ctx->swapchain.cmd_buffers[i_frame], &begin_info);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not begin recording the command buffer, reason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }
    //  Set the correct frame job queue
    jta_frame_job_queue* current_queue = ctx->swapchain.frame_queues[i_frame];
    //  Execute all previous jobs
    jta_frame_job_queue_execute(current_queue);
    ctx->current_queue = current_queue;

    *p_buffer = cmd_buffer;
    *p_img = ctx->swapchain.last_img;
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result jta_vulkan_end_draw(jta_vulkan_window_context* ctx, VkCommandBuffer cmd_buffer)
{
    JDM_ENTER_FUNCTION;
    const unsigned i_frame = ctx->swapchain.current_img;
    assert(ctx->swapchain.cmd_buffers[i_frame] == cmd_buffer);

    VkResult vk_res = vkEndCommandBuffer(cmd_buffer);
    static const VkPipelineStageFlags stage_flags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSubmitInfo submit_info =
            {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = ctx->swapchain.cmd_buffers + i_frame,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = ctx->swapchain.sem_present + i_frame,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = ctx->swapchain.sem_available + i_frame,
            .pWaitDstStageMask = stage_flags,
            };
    vk_res = vkQueueSubmit(ctx->queue_graphics_data.handle, 1, &submit_info, ctx->swapchain.fen_swap[i_frame]);
    if (vk_res)
    {
        JDM_ERROR("Could not submit command buffer to graphics queue, resason: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_QUEUE_SUBMIT;
    }

    const VkPresentInfoKHR present_info =
            {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains = &ctx->swapchain.swapchain,
            .pImageIndices = &ctx->swapchain.last_img,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = ctx->swapchain.sem_present + i_frame,
            };
    vk_res = vkQueuePresentKHR(ctx->queue_present_data.handle, &present_info);
    ctx->swapchain.current_img = (i_frame + 1) % ctx->swapchain.frames_in_flight;
    switch (vk_res)
    {
    case VK_SUBOPTIMAL_KHR:
    case VK_SUCCESS: break;
    case VK_ERROR_OUT_OF_DATE_KHR:
        JDM_WARN("Swapchain became outdated");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_SWAPCHAIN_OUT_OF_DATE;
    default:
        JDM_ERROR("Unexpected vulkan error code for vkQueuePresentKHR: %s(%d)", vk_result_to_str(vk_res), vk_res);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_UNEXPECTED;
    }


    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result jta_vulkan_memory_to_buffer(
        jta_vulkan_window_context* ctx, uint64_t offset, uint64_t size, const void* ptr, uint64_t destination_offset,
        jvm_buffer_allocation* destination)
{
    JDM_ENTER_FUNCTION;
    VkCommandBuffer cmd_buffer;
    gfx_result res = jta_vulkan_queue_begin_transient(ctx, &ctx->queue_transfer_data, &cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not begin transient command, reason: %s", gfx_result_to_str(res));
        JDM_LEAVE_FUNCTION;
        return res;
    }

    size_t mapped_size;
    void* mapped_memory;
    VkResult vk_res = jvm_buffer_map(ctx->transfer_buffer, &mapped_size, &mapped_memory);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not map transfer buffer to host memory, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
        res = GFX_RESULT_MAP_FAILED;
        goto failed;
    }
    uint64_t transfer_size = size;
    uint64_t left_over = 0;
    if (transfer_size > jvm_buffer_allocation_get_size(ctx->transfer_buffer))
    {
        transfer_size = jvm_buffer_allocation_get_size(ctx->transfer_buffer);
        left_over = size - transfer_size;
    }
    memcpy(mapped_memory, ((const uint8_t*)ptr) + offset, transfer_size);
    vk_res = jvm_buffer_unmap(ctx->transfer_buffer);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not unmap transfer buffer to host memory, reason: %s (%d)", vk_result_to_str(vk_res), vk_res);
        res = GFX_RESULT_MAP_FAILED;
        goto failed;
    }
    mapped_memory = (void*)0xCCCCCCCCCCCCCCCC;

    const VkBufferCopy cpy_info =
            {
            .size = transfer_size,
            .dstOffset = 0 + destination_offset,
            .srcOffset = 0
            };
    vkCmdCopyBuffer(cmd_buffer, jvm_buffer_allocation_get_buffer(ctx->transfer_buffer), jvm_buffer_allocation_get_buffer(destination), 1, &cpy_info);
    res = jta_vulkan_queue_end_transient(ctx, &ctx->queue_transfer_data, cmd_buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Could not end transient command, reason: %s", gfx_result_to_str(res));
        res = GFX_RESULT_BAD_VK_CALL;
        goto failed;
    }

    JDM_LEAVE_FUNCTION;
    if (left_over == 0)
    {
        return GFX_RESULT_SUCCESS;
    }
    //  Tail recursion FTW
    return jta_vulkan_memory_to_buffer(ctx, offset + left_over, left_over, ((const uint8_t*)ptr) + transfer_size, destination_offset + transfer_size, destination);
failed:
    JDM_LEAVE_FUNCTION;
    return res;
}

gfx_result jta_vulkan_queue_begin_transient(
        const jta_vulkan_window_context* ctx, const jta_vulkan_queue* queue, VkCommandBuffer* p_cmd_buffer)
{
    JDM_ENTER_FUNCTION;

    const VkCommandBufferAllocateInfo allocate_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandBufferCount = 1,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = queue->transient_pool,
            };
    VkCommandBuffer buffer;
    VkResult result = vkAllocateCommandBuffers(ctx->device, &allocate_info, &buffer);
    if (result != VK_SUCCESS)
    {
        JDM_ERROR("Could not allocate transient command buffer for queue %"PRIu32", reason: %s(%d)", queue->index,
                  vk_result_to_str(result), result);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }
    static const VkCommandBufferBeginInfo begin_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pInheritanceInfo = NULL,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            };
    result = vkBeginCommandBuffer(buffer, &begin_info);
    if (result != VK_SUCCESS)
    {
        JDM_ERROR("Could not begin command buffer");
        vkFreeCommandBuffers(ctx->device, queue->transient_pool, 1, &buffer);
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_VK_CALL;
    }
    *p_cmd_buffer = buffer;
    
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

gfx_result jta_vulkan_queue_end_transient(
        const jta_vulkan_window_context* ctx, const jta_vulkan_queue* queue, VkCommandBuffer cmd_buffer)
{
    JDM_ENTER_FUNCTION;
    VkResult result = vkEndCommandBuffer(cmd_buffer);
    gfx_result res = GFX_RESULT_SUCCESS;
    if (result == VK_SUCCESS)
    {
        const VkSubmitInfo submit_info =
                {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1,
                .pCommandBuffers = &cmd_buffer,
                };
        (void)vkQueueSubmit(queue->handle, 1, &submit_info, queue->fence);
        (void)vkWaitForFences(ctx->device, 1, &queue->fence, VK_TRUE, UINT64_MAX);
        (void)vkResetFences(ctx->device, 1, &queue->fence);
    }
    else
    {
        JDM_ERROR("Could not end command buffer, reason: %s(%d)", vk_result_to_str(result), result);
        res = GFX_RESULT_BAD_VK_CALL;
    }
    vkFreeCommandBuffers(ctx->device, queue->transient_pool, 1, &cmd_buffer);
    
    
    JDM_LEAVE_FUNCTION;
    return res;
}

gfx_result jta_vulkan_context_enqueue_frame_job(const jta_vulkan_window_context* ctx, void(*job_callback)(void* job_param), void* job_param)
{
    JDM_ENTER_FUNCTION;
    const gfx_result res = jta_frame_job_queue_add_job(ctx->current_queue, job_callback, job_param);
    JDM_LEAVE_FUNCTION;
    return res;
}



static void destroy_buffer_queue_job(void* param)
{
    JDM_ENTER_FUNCTION;
    jvm_buffer_allocation* const allocation = param;
    const VkResult result = jvm_buffer_destroy(allocation);
    if (result != VK_SUCCESS)
    {
        JDM_ERROR("Destroying an enqueued buffer returned %s (%d)", vk_result_to_str(result), result);
    }
    JDM_LEAVE_FUNCTION;
}

void
jta_vulkan_context_enqueue_destroy_buffer(const jta_vulkan_window_context* ctx, jvm_buffer_allocation* buffer)
{
    JDM_ENTER_FUNCTION;

    const gfx_result res = jta_vulkan_context_enqueue_frame_job(ctx, destroy_buffer_queue_job, buffer);
    if (res != GFX_RESULT_SUCCESS)
    {
        JDM_ERROR("Failed enqueueing buffer destruction, reason: %s", gfx_result_to_str(res));
    }

    JDM_LEAVE_FUNCTION;
}

