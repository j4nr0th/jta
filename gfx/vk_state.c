//
// Created by jan on 14.5.2023.
//
#include <jdm.h>
#include "vk_state.h"


#include "../shaders/vtx_shader3d.spv"
#include "../shaders/frg_shader3d.spv"

#include "drawing_3d.h"


//VkRenderPass render_pass_3D;
//VkRenderPass render_pass_UI;
//u32 framebuffer_count;
//VkFramebuffer* framebuffers;
//VkImageView* frame_views;
//u32 frame_index;
//VkViewport viewport;
//VkRect2D scissor;
//VkPipelineLayout layout_3D;
//VkPipeline gfx_pipeline_3D;
//VkPipelineLayout layout_UI;
//VkPipeline gfx_pipeline_UI;
//vk_buffer_allocator* buffer_allocator;
//vk_buffer_allocation buffer_device_local, buffer_transfer, buffer_uniform;
//VkDescriptorSetLayout ubo_layout;
//void** p_mapped_array;
//VkDescriptorSet* desc_set;
//VkDescriptorPool desc_pool;
//VkImageView depth_view;
//VkImage depth_img;
//VkDeviceMemory depth_mem;
//VkFormat depth_format;

gfx_result vk_state_create(vk_state* const p_state, const jfw_window_vk_resources* const vk_resources)
{
    memset(p_state, 0, sizeof*p_state);
    //  Create buffer allocators
    vk_buffer_allocator* allocator = vk_buffer_allocator_create(vk_resources->device, vk_resources->physical_device, 1 << 20);
    if (!allocator)
    {
        JDM_ERROR("Failed creating Vulkan buffer allocator");
        return GFX_RESULT_BAD_ALLOC;
    }
    p_state->buffer_allocator = allocator;

    u32 max_samples;
    VkResult vk_res;
    //  Find the appropriate sample flag
    {
        VkSampleCountFlags flags = vk_resources->sample_flags;
        if (flags & VK_SAMPLE_COUNT_64_BIT) max_samples = VK_SAMPLE_COUNT_64_BIT;
        else if (flags & VK_SAMPLE_COUNT_32_BIT) max_samples = VK_SAMPLE_COUNT_32_BIT;
        else if (flags & VK_SAMPLE_COUNT_16_BIT) max_samples = VK_SAMPLE_COUNT_16_BIT;
        else if (flags & VK_SAMPLE_COUNT_8_BIT) max_samples = VK_SAMPLE_COUNT_8_BIT;
        else if (flags & VK_SAMPLE_COUNT_4_BIT) max_samples = VK_SAMPLE_COUNT_4_BIT;
        else if (flags & VK_SAMPLE_COUNT_2_BIT) max_samples = VK_SAMPLE_COUNT_2_BIT;
        else max_samples = VK_SAMPLE_COUNT_1_BIT;
        assert(flags & VK_SAMPLE_COUNT_1_BIT);
    }

    //  Create the depth buffer
    {
        VkImage depth_image;
        VkDeviceMemory depth_memory;
        VkImageView depth_view;
        VkFormat depth_format;
        i32 has_stencil;

        const VkFormat possible_depth_formats[3] = {VK_FORMAT_D32_SFLOAT_S8_UINT , VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT};
        const u32 n_possible_depth_formats = sizeof(possible_depth_formats) / sizeof(*possible_depth_formats);
        VkFormatProperties props;
        u32 i;
        for (i = 0; i < n_possible_depth_formats; ++i)
        {
            vkGetPhysicalDeviceFormatProperties(vk_resources->physical_device, possible_depth_formats[i], &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                break;
            }
        }
        if (i == n_possible_depth_formats)
        {
            JDM_ERROR("Could not find a good image format for the depth buffer");
            vk_buffer_allocator_destroy(allocator);
            return GFX_RESULT_NO_IMG_FMT;
        }
        depth_format = possible_depth_formats[i];
        has_stencil = (depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT);
        VkImageCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .extent = { .width = vk_resources->extent.width, .height = vk_resources->extent.height, .depth = 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .format = depth_format,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                };
        vk_res = vkCreateImage(vk_resources->device, &create_info, NULL, &depth_image);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create depth buffer image, reason: %s", jfw_vk_error_msg(vk_res));
            vkDestroyImage(vk_resources->device, depth_image, NULL);
            vk_buffer_allocator_destroy(allocator);
            return GFX_RESULT_NO_DEPBUF_IMG;
        }
        VkMemoryRequirements mem_req;
        vkGetImageMemoryRequirements(vk_resources->device, depth_image, &mem_req);
        u32 heap_type;
        {
            const VkPhysicalDeviceMemoryProperties* mem_props = vk_allocator_mem_props(allocator);
            for (heap_type = 0; heap_type < mem_props->memoryTypeCount; ++heap_type)
            {
                if (mem_req.memoryTypeBits & (1 << heap_type) && mem_props->memoryTypes[heap_type].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                {
                    break;
                }
            }
            if (heap_type == mem_props->memoryTypeCount)
            {
                JDM_ERROR("Could not find appropriate memory type for the depth image storage");
                vkDestroyImage(vk_resources->device, depth_image, NULL);
                vk_buffer_allocator_destroy(allocator);
                return GFX_RESULT_NO_MEM_TYPE;
            }
        }
        VkMemoryAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize = mem_req.size,
                .memoryTypeIndex = heap_type,
                .pNext = nullptr,
                };
        vk_res = vkAllocateMemory(vk_resources->device, &alloc_info, NULL, &depth_memory);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not allocate memory for the depth image: %s", jfw_vk_error_msg(vk_res));
            vkDestroyImage(vk_resources->device, depth_image, NULL);
            vk_buffer_allocator_destroy(allocator);
            return GFX_RESULT_BAD_ALLOC;
        }
        vk_res = vkBindImageMemory(vk_resources->device, depth_image, depth_memory, 0);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed binding depth image to its memory allocation: %s", jfw_vk_error_msg(vk_res));
            vkFreeMemory(vk_resources->device, depth_memory, NULL);
            vkDestroyImage(vk_resources->device, depth_image, NULL);
            vk_buffer_allocator_destroy(allocator);
            return GFX_RESULT_BAD_IMG_BIND;
        }
        VkImageViewCreateInfo view_info =
                {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = depth_format,
                .image = depth_image,

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
        vk_res = vkCreateImageView(vk_resources->device, &view_info, NULL, &depth_view);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create image view for the depth buffer: %s", jfw_vk_error_msg(vk_res));
            vkFreeMemory(vk_resources->device, depth_memory, NULL);
            vkDestroyImage(vk_resources->device, depth_image, NULL);
            vk_buffer_allocator_destroy(allocator);
            return GFX_RESULT_NO_IMG_VIEW;
        }
        p_state->depth_view = depth_view;
        p_state->depth_img = depth_image;
        p_state->depth_mem = depth_memory;
        p_state->depth_format = depth_format;
    }
    gfx_result gfx_res = GFX_RESULT_SUCCESS;
    //  Create render pass(es)
    {
        VkRenderPass render_pass_3d;
        VkAttachmentDescription color_attach_description =
                {
                .format = vk_resources->surface_format.format,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentReference color_attach_ref =
                {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentDescription depth_attach_description =
                {
                        .format = p_state->depth_format,
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
        VkSubpassDescription subpass_description_3d =
                {
                .colorAttachmentCount = 1,
                .pColorAttachments = &color_attach_ref,
                .pDepthStencilAttachment = &depth_attach_ref,
                };
        VkSubpassDependency subpass_dependency_3d =
                {
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = 0,
                .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                };
        VkAttachmentDescription attachment_array_3d[] =
                {
                color_attach_description, depth_attach_description
                };
        VkRenderPassCreateInfo render_pass_info_3d =
                {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .attachmentCount = 2,
                .pAttachments = attachment_array_3d,
                .dependencyCount = 1,
                .pDependencies = &subpass_dependency_3d,
                .subpassCount = 1,
                .pSubpasses = &subpass_description_3d,
                };
        vk_res = vkCreateRenderPass(vk_resources->device, &render_pass_info_3d, NULL, &render_pass_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create 3d render pass: %s", jfw_vk_error_msg(vk_res));
            gfx_res = GFX_RESULT_NO_RENDER_PASS;
            goto after_depth;
        }


        p_state->render_pass_3D = render_pass_3d;
    }
    //  Create Coordinate frame render pass
    {
        VkRenderPass render_pass_cf;
        VkAttachmentDescription color_attach_description =
                {
                        .format = vk_resources->surface_format.format,
                        .samples = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                };
        VkAttachmentReference color_attach_ref =
                {
                        .attachment = 0,
                        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                };
        VkAttachmentDescription depth_attach_description =
                {
                        .format = p_state->depth_format,
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
        vk_res = vkCreateRenderPass(vk_resources->device, &render_pass_info_cf, NULL, &render_pass_cf);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create 3d render pass: %s", jfw_vk_error_msg(vk_res));
            gfx_res = GFX_RESULT_NO_RENDER_PASS;
            goto after_render_pass_3d;
        }


        p_state->render_pass_cf = render_pass_cf;


    }

    //  Create ubo layout descriptors
    {
        VkDescriptorSetLayout ubo_layout;
        VkDescriptorSetLayoutBinding binding =
                {
                .binding = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = NULL,
                };
        VkDescriptorSetLayoutCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .bindingCount = 1,
                .pBindings = &binding,
                };
        vk_res = vkCreateDescriptorSetLayout(vk_resources->device, &create_info, NULL, &ubo_layout);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create UBO descriptor set layout: %s", jfw_vk_error_msg(vk_res));
            gfx_res = GFX_RESULT_NO_DESC_SET;
            goto after_render_pass_cf;
        }
        p_state->ubo_layout = ubo_layout;
    }

    //  Create pipeline layout(s)
    {
        VkPipelineLayout layout_3d;
        VkShaderModule module_vtx_3d, module_frg_3d, module_geo_3d = NULL;
        VkPipeline pipeline_3d;
        VkViewport viewport = {.x = 0.0f, .y = 0.0f, .width = (f32)vk_resources->extent.width, .height = (f32)vk_resources->extent.height,
                .minDepth = 0.0f, .maxDepth = 1.0f};
        VkRect2D scissors = {.offset = {0, 0}, .extent = vk_resources->extent};
        p_state->viewport = viewport;
        p_state->scissor = scissors;
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
                .codeSize = sizeof(vtx_shader3d),
                .pCode = vtx_shader3d,
                };
        VkShaderModuleCreateInfo shader_frg_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .codeSize = sizeof(frg_shader3d),
                .pCode = frg_shader3d,
                };
        vk_res = vkCreateShaderModule(vk_resources->device, &shader_vtx_create_info, NULL, &module_vtx_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create vertex shader module (3d), reason: %s", jfw_vk_error_msg(vk_res));
            gfx_res = GFX_RESULT_NO_VTX_SHADER;
            goto after_ubo_layout;
        }
        vk_res = vkCreateShaderModule(vk_resources->device, &shader_frg_create_info, NULL, &module_frg_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create fragment shader module (3d), reason: %s", jfw_vk_error_msg(vk_res));
            vkDestroyShaderModule(vk_resources->device, module_vtx_3d, NULL);
            gfx_res = GFX_RESULT_NO_FRG_SHADER;
            goto after_ubo_layout;
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
        VkPipelineLayoutCreateInfo layout_create_info =
                {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount = 1,
                .pSetLayouts = &p_state->ubo_layout,
                };
        vk_res = vkCreatePipelineLayout(vk_resources->device, &layout_create_info, NULL, &layout_3d);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating 3d pipeline layout, reason: %s", jfw_vk_error_msg(vk_res));
            vkDestroyShaderModule(vk_resources->device, module_frg_3d, NULL);
            vkDestroyShaderModule(vk_resources->device, module_vtx_3d, NULL);
            gfx_res = GFX_RESULT_NO_PIPELINE_LAYOUT;
            goto after_ubo_layout;
        }
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
        VkGraphicsPipelineCreateInfo create_info =
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
                .renderPass = p_state->render_pass_3D,
                .subpass = 0,
                .basePipelineHandle = VK_NULL_HANDLE,
                .basePipelineIndex = -1,
                };
        vk_res = vkCreateGraphicsPipelines(vk_resources->device, NULL, 1, &create_info, NULL, &pipeline_3d);
        vkDestroyShaderModule(vk_resources->device, module_frg_3d, NULL);
        vkDestroyShaderModule(vk_resources->device, module_vtx_3d, NULL);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating a gfx 3d pipeline, reason: %s", jfw_vk_error_msg(vk_res));
            vkDestroyPipelineLayout(vk_resources->device, layout_3d, NULL);
            gfx_res = GFX_RESULT_NO_PIPELINE;
            goto after_ubo_layout;
        }
        p_state->gfx_pipeline_3D = pipeline_3d;
        p_state->layout_3D = layout_3d;
    }

    //  Create the framebuffer(s)
    {
        VkFramebuffer* framebuffer_array;
        jfw_result jfw_result = jfw_calloc(vk_resources->n_images, sizeof(VkFramebuffer), &framebuffer_array);
        if (JFW_RESULT_SUCCESS !=(jfw_result))
        {
            JDM_ERROR("Could not allocate memory for framebuffer array");
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_3d_pipeline;
        }
        VkImageView attachments[2] =
                {
                NULL, p_state->depth_view
                };
        VkFramebufferCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = p_state->render_pass_3D,
                .attachmentCount = 2,
                .pAttachments = attachments,
                .width = vk_resources->extent.width,
                .height = vk_resources->extent.height,
                .layers = 1
                };
        for (u32 i = 0; i < vk_resources->n_images; ++i)
        {
            attachments[0] = vk_resources->views[i];
            vk_res = vkCreateFramebuffer(vk_resources->device, &create_info, NULL, framebuffer_array + i);
            if (vk_res != VK_SUCCESS)
            {

                JDM_ERROR("Could not create framebuffer %u out of %u, reason: %s", i + 1, vk_resources->n_images,
                          jfw_vk_error_msg(vk_res));
                gfx_res = GFX_RESULT_NO_FRAMEBUFFER;
                jfw_free(&framebuffer_array);
                goto after_3d_pipeline;
            }
        }
        p_state->framebuffer_count = vk_resources->n_images;
        p_state->framebuffers = framebuffer_array;
    }

    //  Reserve space for vtx/idx buffers
    i32 ret_v = vk_buffer_reserve(allocator, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    if (ret_v != 0)
    {
        ret_v = vk_buffer_reserve(allocator, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    }
    if (ret_v != 0)
    {
        JDM_ERROR("Could not reserve device memory for vtx and idx buffers");
        gfx_res = GFX_RESULT_BAD_ALLOC;
        goto after_3d_framebuffers;
    }
    //  Reserve space for transfer buffer
    ret_v = vk_buffer_reserve(allocator, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
    if (ret_v != 0)
    {
//        //  Have to allocate them seperately
//        ret_v = vk_buffer_allocate(allocator, 1 << 8, 1 << 12, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, &p_state->buffer_transfer);
//        if (ret_v != 0)
//        {
//            JDM_ERROR("Could not reserve device memory for transfer buffer(s)");
//            jfw_result = JFW_RES_VK_FAIL;
//            goto after_3d_framebuffers;
//        }
        //  Reserve space for uniform buffer(s)
        ret_v = vk_buffer_reserve(allocator, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE);
        if (ret_v != 0)
        {
            JDM_ERROR("Could not reserve device memory for uniform buffer(s)");
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_3d_framebuffers;
        }
    }
    //  Have to allocate them seperately
    ret_v = vk_buffer_allocate(allocator, 1 << 8, 1 << 12, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, &p_state->buffer_transfer);
    if (ret_v != 0)
    {
        JDM_ERROR("Could not reserve device memory for transfer buffer(s)");
        gfx_res = GFX_RESULT_BAD_ALLOC;
        goto after_3d_framebuffers;
    }

    //  Create transfer fence
    {
        VkFenceCreateInfo create_info =
                {
                        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                };
        vk_res = vkCreateFence(vk_resources->device, &create_info, NULL, &p_state->fence_transfer_free);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Failed creating transfer buffer fence");
            gfx_res = GFX_RESULT_NO_FENCE;
            goto after_3d_framebuffers;
        }
    }

    //  Create (and map uniform buffer)
    {
        ubo_3d** mapped_array;
        jfw_result jfw_result = jfw_calloc(vk_resources->n_frames_in_flight, sizeof(void*), &mapped_array);
        if (JFW_RESULT_SUCCESS !=(jfw_result))
        {
            JDM_ERROR("Could not allocate memory for UBO mapping pointers");
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_transfer_fence;
        }
        ret_v = vk_buffer_allocate(allocator, sizeof(ubo_3d), vk_resources->n_frames_in_flight, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_SHARING_MODE_EXCLUSIVE, &p_state->buffer_uniform);
        if (ret_v != 0)
        {
            JDM_ERROR("Could not allocate 3d UBO");
            jfw_free(&mapped_array);
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_transfer_fence;
        }
        ubo_3d* const original_mapping = vk_map_allocation(&p_state->buffer_uniform);
        for (u32 i = 0; i < vk_resources->n_frames_in_flight; ++i)
        {
            mapped_array[i] = (void*)((uintptr_t)original_mapping + (uintptr_t)(i * p_state->buffer_uniform.offset_alignment));
        }
        p_state->p_mapped_array = mapped_array;
    }

    //  Create UBO descriptor pool and descriptor sets
    {
        VkDescriptorPool desc_pool;
        VkDescriptorSet* desc_sets;
        VkDescriptorPoolSize descriptor_pool_size =
                {
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = vk_resources->n_frames_in_flight,
                };
        VkDescriptorPoolCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                .poolSizeCount = 1,
                .pPoolSizes = &descriptor_pool_size,
                .maxSets = vk_resources->n_frames_in_flight,
                };
        vk_res = vkCreateDescriptorPool(vk_resources->device, &create_info, NULL, &desc_pool);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not allocate descriptor pool for UBO data, reason: %s", jfw_vk_error_msg(vk_res));
            gfx_res = GFX_RESULT_NO_DESC_POOL;
            goto after_mapped_array;
        }
        VkDescriptorSetLayout* layouts;
        jfw_result jfw_result = jfw_calloc(vk_resources->n_frames_in_flight, sizeof(*layouts), &layouts);
        if (JFW_RESULT_SUCCESS !=(jfw_result))
        {
            JDM_ERROR("Could not allocate memory for descriptor sets");
            vkDestroyDescriptorPool(vk_resources->device, desc_pool, NULL);
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_mapped_array;
        }
        jfw_result = jfw_calloc(vk_resources->n_frames_in_flight, sizeof(*desc_sets), &desc_sets);
        if (JFW_RESULT_SUCCESS !=(jfw_result))
        {
            JDM_ERROR("Could not allocate memory for descriptor sets");
            jfw_free(&layouts);
            vkDestroyDescriptorPool(vk_resources->device, desc_pool, NULL);
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_mapped_array;
        }

        for (u32 i = 0; i < vk_resources->n_frames_in_flight; ++i)
        {
            layouts[i] = p_state->ubo_layout;
        }
        VkDescriptorSetAllocateInfo allocate_info =
                {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                .descriptorSetCount = vk_resources->n_frames_in_flight,
                .descriptorPool = desc_pool,
                .pSetLayouts = layouts,
                };
        vk_res = vkAllocateDescriptorSets(vk_resources->device, &allocate_info, desc_sets);
        jfw_free(&layouts);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not allocate descriptor sets, reason: %s", jfw_vk_error_msg(vk_res));
            jfw_free(&desc_sets);
            vkDestroyDescriptorPool(vk_resources->device, desc_pool, NULL);
            gfx_res = GFX_RESULT_BAD_ALLOC;
            goto after_mapped_array;
        }
        VkDescriptorBufferInfo buffer_info =
                {
                .buffer = p_state->buffer_uniform.buffer,
                .range = sizeof(ubo_3d),
                //  Set offset in loop
                };
        VkWriteDescriptorSet write_set =
                {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &buffer_info,
                .pImageInfo = NULL,
                .pTexelBufferView = NULL,
                };
        for (u32 i = 0; i < vk_resources->n_frames_in_flight; ++i)
        {
            buffer_info.offset = p_state->buffer_uniform.offset + i * p_state->buffer_uniform.offset_alignment;
            write_set.dstSet = desc_sets[i];
            vkUpdateDescriptorSets(vk_resources->device, 1, &write_set, 0, NULL);
        }

        p_state->desc_pool = desc_pool;
        p_state->desc_set = desc_sets;
    }
    {
        VkCommandPool cmd_pool;
        VkCommandBuffer cmd_buffer;
        VkCommandPoolCreateInfo create_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .queueFamilyIndex = vk_resources->i_trs_queue,
                .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                };
        vk_res = vkCreateCommandPool(vk_resources->device, &create_info, NULL, &cmd_pool);
        if (vk_res != VK_SUCCESS)
        {
            JDM_ERROR("Could not create transfer command pool, reason: %s", jfw_vk_error_msg(vk_res));
            goto after_descriptor_sets;
        }
        VkCommandBufferAllocateInfo alloc_info =
                {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandPool = cmd_pool,
                .commandBufferCount = 1,
                };
        vk_res = vkAllocateCommandBuffers(vk_resources->device, &alloc_info, &cmd_buffer);
        if (vk_res != VK_SUCCESS)
        {
            vkDestroyCommandPool(vk_resources->device, cmd_pool, NULL);
            JDM_ERROR("Could not allocate transfer command buffer, reason: %s", jfw_vk_error_msg(vk_res));
            goto after_descriptor_sets;
        }
        p_state->transfer_cmd_pool = cmd_pool;
        p_state->transfer_cmd_buffer = cmd_buffer;
    }


    return gfx_res;
after_transfer_buffers:
    vkDestroyCommandPool(vk_resources->device, p_state->transfer_cmd_pool, NULL);
after_descriptor_sets:
    vkDestroyDescriptorPool(vk_resources->device, p_state->desc_pool, NULL);
    jfw_free(&p_state->desc_set);
after_mapped_array:
    jfw_free(&p_state->p_mapped_array);
after_transfer_fence:
    vkDestroyFence(vk_resources->device, p_state->fence_transfer_free, NULL);
after_3d_framebuffers:
    for (u32 i = 0; i < p_state->framebuffer_count; ++i)
    {
        vkDestroyFramebuffer(vk_resources->device, p_state->framebuffers[i], NULL);
    }
    jfw_free(&p_state->framebuffers);
after_3d_pipeline:
    vkDestroyPipeline(vk_resources->device, p_state->gfx_pipeline_3D, NULL);
    vkDestroyPipelineLayout(vk_resources->device, p_state->layout_3D, NULL);
after_ubo_layout:
    vkDestroyDescriptorSetLayout(vk_resources->device, p_state->ubo_layout, NULL);
after_render_pass_cf:
    vkDestroyRenderPass(vk_resources->device, p_state->render_pass_cf, NULL);
after_render_pass_3d:
    vkDestroyRenderPass(vk_resources->device, p_state->render_pass_3D, NULL);
after_depth:
    vkDestroyImageView(vk_resources->device, p_state->depth_view, NULL);
    vkFreeMemory(vk_resources->device, p_state->depth_mem, NULL);
    vkDestroyImage(vk_resources->device, p_state->depth_img, NULL);
    vk_buffer_allocator_destroy(allocator);

    memset(p_state, 0, sizeof*p_state);
    return gfx_res;
}

gfx_result vk_transfer_memory_to_buffer(
        jfw_window_vk_resources* vk_resources, vk_state* p_state, vk_buffer_allocation* buffer, size_t size, void* data)
{
    JDM_ENTER_FUNCTION;
    assert(size <= p_state->buffer_transfer.size);
    VkResult vk_res = vkWaitForFences(vk_resources->device, 1, &p_state->fence_transfer_free, VK_TRUE, UINT64_MAX);
    vkResetFences(vk_resources->device, 1, &p_state->fence_transfer_free);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not wait for transfer buffer fence, reason: %s", jfw_vk_error_msg(vk_res));
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_FENCE_WAIT;
    }
    void* mapped_memory = vk_map_allocation(&p_state->buffer_transfer);
    if (!mapped_memory)
    {
        JDM_ERROR("Could not map buffer to memory");
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_MAP_FAILED;
    }
    memcpy(mapped_memory, data, size);
    vk_unmap_allocation(mapped_memory, &p_state->buffer_transfer);
    VkCommandBuffer cmd_buffer = p_state->transfer_cmd_buffer;
    static const VkCommandBufferBeginInfo begin_info =
            {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            };
    vk_res = vkResetCommandBuffer(cmd_buffer, 0);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not begin reset the command buffer, reason: %s", jfw_vk_error_msg(vk_res));
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_CMDBUF_RESET;
    }
    vk_res = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    if (vk_res != VK_SUCCESS)
    {
        JDM_ERROR("Could not begin recording of the command transfer buffer, reason: %s", jfw_vk_error_msg(vk_res));
        JDM_LEAVE_FUNCTION;
        return GFX_RESULT_BAD_CMDBUF_RESET;
    }
    const VkBufferCopy buffer_copy_info =
            {
            .size = size,
            .srcOffset = p_state->buffer_transfer.offset,
            .dstOffset = buffer->offset,
            };
    vkCmdCopyBuffer(cmd_buffer, p_state->buffer_transfer.buffer, buffer->buffer, 1, &buffer_copy_info);
    vkEndCommandBuffer(cmd_buffer);
    const VkSubmitInfo submit_info =
            {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd_buffer,
            };
    vkQueueSubmit(vk_resources->queue_transfer, 1, &submit_info, p_state->fence_transfer_free);
    JDM_LEAVE_FUNCTION;
    return GFX_RESULT_SUCCESS;
}

void vk_state_destroy(vk_state* p_state, jfw_window_vk_resources* vk_resources)
{
    vkDestroyCommandPool(vk_resources->device, p_state->transfer_cmd_pool, NULL);
    vkDestroyDescriptorPool(vk_resources->device, p_state->desc_pool, NULL);
    jfw_free(&p_state->desc_set);
    jfw_free(&p_state->p_mapped_array);
    vkDestroyFence(vk_resources->device, p_state->fence_transfer_free, NULL);
    for (u32 i = 0; i < p_state->framebuffer_count; ++i)
    {
        vkDestroyFramebuffer(vk_resources->device, p_state->framebuffers[i], NULL);
    }
    jfw_free(&p_state->framebuffers);
    vkDestroyPipeline(vk_resources->device, p_state->gfx_pipeline_3D, NULL);
    vkDestroyPipelineLayout(vk_resources->device, p_state->layout_3D, NULL);
    vkDestroyDescriptorSetLayout(vk_resources->device, p_state->ubo_layout, NULL);
    vkDestroyRenderPass(vk_resources->device, p_state->render_pass_cf, NULL);
    vkDestroyRenderPass(vk_resources->device, p_state->render_pass_3D, NULL);
    vkDestroyImageView(vk_resources->device, p_state->depth_view, NULL);
    vkFreeMemory(vk_resources->device, p_state->depth_mem, NULL);
    vkDestroyImage(vk_resources->device, p_state->depth_img, NULL);
    vk_buffer_allocator_destroy(p_state->buffer_allocator);

    memset(p_state, 0, sizeof*p_state);
}
