//
// Created by jan on 2.4.2023.
//

#include "error_codes.h"

static const char* const ERROR_MESSAGES[jfw_count] =
        {
                [jfw_res_success] = "NO ERROR",
                [jfw_res_error] = "GENERIC ERROR",
        };

const char* jfw_error_message(jfw_res err)
{
    if (err >= 0 && err < jfw_count) return ERROR_MESSAGES[err];
    return "Invalid error code";
}

jfw_res jfw_error_message_r(jfw_res error_code, size_t buffer_size, char* buffer)
{
    const char* message =  NULL;
    if (error_code >= 0 && error_code < jfw_count)
    {
        message = ERROR_MESSAGES[error_code];
        const size_t len =strlen(message);
        if (len < buffer_size)
        {
            memcpy(buffer, message, len);
            buffer[len] = 0;
        }
        else
        {
            memcpy(buffer, message, buffer_size);
            buffer[buffer_size - 1] = 0;
        }
        return jfw_res_success;
    }


    snprintf(buffer, buffer_size, "Unknown Error Code: %u", error_code);
    return jfw_res_bad_error;
}


#include <vulkan/vulkan_core.h>
#define ADD_CODE_TO_SWITCH(i) case (i): return #i;
const char* jfw_vk_error_msg(u32 vk_code)
{
    VkResult code = vk_code;
    switch (code)
    {
        ADD_CODE_TO_SWITCH(VK_SUCCESS)
        ADD_CODE_TO_SWITCH(VK_NOT_READY)
        ADD_CODE_TO_SWITCH(VK_TIMEOUT)
        ADD_CODE_TO_SWITCH(VK_EVENT_SET)
        ADD_CODE_TO_SWITCH(VK_EVENT_RESET)
        ADD_CODE_TO_SWITCH(VK_INCOMPLETE)
        ADD_CODE_TO_SWITCH(VK_ERROR_OUT_OF_HOST_MEMORY)
        ADD_CODE_TO_SWITCH(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        ADD_CODE_TO_SWITCH(VK_ERROR_INITIALIZATION_FAILED)
        ADD_CODE_TO_SWITCH(VK_ERROR_DEVICE_LOST)
        ADD_CODE_TO_SWITCH(VK_ERROR_MEMORY_MAP_FAILED)
        ADD_CODE_TO_SWITCH(VK_ERROR_LAYER_NOT_PRESENT)
        ADD_CODE_TO_SWITCH(VK_ERROR_EXTENSION_NOT_PRESENT)
        ADD_CODE_TO_SWITCH(VK_ERROR_FEATURE_NOT_PRESENT)
        ADD_CODE_TO_SWITCH(VK_ERROR_INCOMPATIBLE_DRIVER)
        ADD_CODE_TO_SWITCH(VK_ERROR_TOO_MANY_OBJECTS)
        ADD_CODE_TO_SWITCH(VK_ERROR_FORMAT_NOT_SUPPORTED)
        ADD_CODE_TO_SWITCH(VK_ERROR_FRAGMENTED_POOL)
        ADD_CODE_TO_SWITCH(VK_ERROR_UNKNOWN)
        ADD_CODE_TO_SWITCH(VK_ERROR_OUT_OF_POOL_MEMORY)
        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_EXTERNAL_HANDLE)
        ADD_CODE_TO_SWITCH(VK_ERROR_FRAGMENTATION)
        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        ADD_CODE_TO_SWITCH(VK_PIPELINE_COMPILE_REQUIRED)
        ADD_CODE_TO_SWITCH(VK_ERROR_SURFACE_LOST_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        ADD_CODE_TO_SWITCH(VK_SUBOPTIMAL_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_OUT_OF_DATE_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_VALIDATION_FAILED_EXT)
        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_SHADER_NV)
        ADD_CODE_TO_SWITCH(VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        ADD_CODE_TO_SWITCH(VK_ERROR_NOT_PERMITTED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        ADD_CODE_TO_SWITCH(VK_THREAD_IDLE_KHR)
        ADD_CODE_TO_SWITCH(VK_THREAD_DONE_KHR)
        ADD_CODE_TO_SWITCH(VK_OPERATION_DEFERRED_KHR)
        ADD_CODE_TO_SWITCH(VK_OPERATION_NOT_DEFERRED_KHR)
        ADD_CODE_TO_SWITCH(VK_ERROR_COMPRESSION_EXHAUSTED_EXT)
        ADD_CODE_TO_SWITCH(VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT)
//        ADD_CODE_TO_SWITCH(VK_ERROR_OUT_OF_POOL_MEMORY_KHR)
//        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR)
//        ADD_CODE_TO_SWITCH(VK_ERROR_FRAGMENTATION_EXT)
//        ADD_CODE_TO_SWITCH(VK_ERROR_NOT_PERMITTED_EXT)
//        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_DEVICE_ADDRESS_EXT)
//        ADD_CODE_TO_SWITCH(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR)
//        ADD_CODE_TO_SWITCH(VK_PIPELINE_COMPILE_REQUIRED_EXT)
//        ADD_CODE_TO_SWITCH(VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT)
        ADD_CODE_TO_SWITCH(VK_RESULT_MAX_ENUM)
    
    }
    return NULL;
}
