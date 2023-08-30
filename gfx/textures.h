//
// Created by jan on 22.8.2023.
//

#ifndef JTA_TEXTURES_H
#define JTA_TEXTURES_H
#include "error.h"
#include "vk_resources.h"

struct jta_texture_struct
{
    jvm_image_allocation* img;
    VkImageView view;
    unsigned width, height;
    VkSampler sampler;
};
typedef struct jta_texture_struct jta_texture;

struct jta_texture_create_info_struct
{
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageTiling tiling;
};
typedef struct jta_texture_create_info_struct jta_texture_create_info;


gfx_result jta_texture_transition(const jta_vulkan_window_context* ctx, const jta_texture* texture, VkFormat fmt, VkImageLayout layout_old, VkImageLayout layout_new);

gfx_result jta_vulkan_memory_to_texture(
        const jta_vulkan_window_context* ctx, uint64_t size, const void* ptr, const jta_texture* destination);

gfx_result jta_texture_load(
        unsigned w, unsigned h, const void* ptr, const jta_vulkan_window_context* ctx, jta_texture_create_info info,
        jta_texture** p_out);

gfx_result jta_texture_destroy(const jta_vulkan_window_context* ctx, jta_texture* tex);


#endif //JTA_TEXTURES_H
