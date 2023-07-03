//
// Created by jan on 30.6.2023.
//

#include "gfxerr.h"

static const char* const jio_result_strings[GFX_RESULT_COUNT] =
        {
                [GFX_RESULT_SUCCESS] = "Success",
                [GFX_RESULT_BAD_SWAPCHAIN_WAIT] = "Waiting on swapchain failed",
                [GFX_RESULT_BAD_SWAPCHAIN_IMG] = "Could not get next swapchain image",
                [GFX_RESULT_SWAPCHAIN_OUT_OF_DATE] = "Swapchain was outdated",
                [GFX_RESULT_UNEXPECTED] = "Unexpected error occurred",
                [GFX_RESULT_BAD_FENCE_RESET] = "Fence could not be reset",
                [GFX_RESULT_BAD_CMDBUF_RESET] = "Command buffer could not be reset",
                [GFX_RESULT_BAD_CMDBUF_END] = "Failed ending the command buffer recording",
                [GFX_RESULT_BAD_QUEUE_SUBMIT] = "Failed submitting commands to a queue",
                [GFX_RESULT_BAD_ALLOC] = "Could not allocate memory",
                [GFX_RESULT_NO_IMG_FMT] = "No appropriate image format was found",
                [GFX_RESULT_NO_DEPBUF_IMG] = "Could not create depth buffer image",
                [GFX_RESULT_NO_MEM_TYPE] = "Could not find appropriate memory type",
                [GFX_RESULT_BAD_IMG_BIND] = "Failed binding image to memory",
                [GFX_RESULT_NO_IMG_VIEW] = "Failed creating image view",
                [GFX_RESULT_NO_RENDER_PASS] = "Failed creating render pass",
                [GFX_RESULT_NO_DESC_SET] = "Failed creating descriptor set",
                [GFX_RESULT_NO_VTX_SHADER] = "Failed creating vertex shader",
                [GFX_RESULT_NO_FRG_SHADER] = "Failed creating fragment shader",
                [GFX_RESULT_NO_PIPELINE_LAYOUT] = "Failed creating pipeline layout",
                [GFX_RESULT_NO_PIPELINE] = "Failed creating pipeline",
                [GFX_RESULT_NO_FRAMEBUFFER] = "Failed creating framebuffer",
                [GFX_RESULT_NO_FENCE] = "Fence creation failed",
                [GFX_RESULT_NO_DESC_POOL] = "Descriptor pool failed",
                [GFX_RESULT_MAP_FAILED] = "Mapping the memory failed",
        };

const char* gfx_result_to_str(gfx_result res)
{
    if (res < GFX_RESULT_COUNT)
        return jio_result_strings[res];
    return "Unknown";
}