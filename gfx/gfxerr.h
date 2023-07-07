//
// Created by jan on 30.6.2023.
//

#ifndef JTB_GFXERR_H
#define JTB_GFXERR_H


enum gfx_result_enum : unsigned
{
    GFX_RESULT_SUCCESS = 0,
    GFX_RESULT_BAD_SWAPCHAIN_WAIT,
    GFX_RESULT_BAD_SWAPCHAIN_IMG,
    GFX_RESULT_SWAPCHAIN_OUT_OF_DATE,
    GFX_RESULT_UNEXPECTED,
    GFX_RESULT_BAD_FENCE_RESET,
    GFX_RESULT_BAD_CMDBUF_RESET,
    GFX_RESULT_BAD_CMDBUF_END,
    GFX_RESULT_BAD_QUEUE_SUBMIT,
    GFX_RESULT_BAD_ALLOC,
    GFX_RESULT_NO_IMG_FMT,
    GFX_RESULT_NO_DEPBUF_IMG,
    GFX_RESULT_NO_MEM_TYPE,
    GFX_RESULT_BAD_IMG_BIND,
    GFX_RESULT_NO_IMG_VIEW,
    GFX_RESULT_NO_RENDER_PASS,
    GFX_RESULT_NO_DESC_SET,
    GFX_RESULT_NO_VTX_SHADER,
    GFX_RESULT_NO_FRG_SHADER,
    GFX_RESULT_NO_PIPELINE_LAYOUT,
    GFX_RESULT_NO_PIPELINE,
    GFX_RESULT_NO_FRAMEBUFFER,
    GFX_RESULT_NO_FENCE,
    GFX_RESULT_NO_DESC_POOL,
    GFX_RESULT_MAP_FAILED,
    GFX_RESULT_COUNT,
};
typedef enum gfx_result_enum gfx_result;

const char* gfx_result_to_str(gfx_result res);

#endif //JTB_GFXERR_H