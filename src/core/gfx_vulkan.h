/**
 * file:    vulkan_device.h
 * created: 2017-03-13
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "platform/platform.h"
#include "core/assets.h"

enum ShaderStage {
    ShaderStage_vertex,
    ShaderStage_fragment,
    ShaderStage_max
};

enum ShaderID {
    ShaderID_mesh_vert,
    ShaderID_mesh_frag,
    ShaderID_basic2d_vert,
    ShaderID_basic2d_frag,
    ShaderID_gui_basic_vert,
    ShaderID_gui_basic_frag,
    ShaderID_font_frag,
    ShaderID_terrain_vert,
    ShaderID_terrain_frag,
    ShaderID_wireframe_vert,
    ShaderID_wireframe_frag
};

enum PipelineID {
    Pipeline_font,
    Pipeline_mesh,
    Pipeline_terrain,
    Pipeline_basic2d,
    Pipeline_gui_basic,
    Pipeline_wireframe,
    Pipeline_wireframe_lines,

    Pipeline_count
};

struct VulkanBuffer {
    usize          size;
    VkBuffer       handle;
    VkDeviceMemory memory;
};

struct VulkanUniformBuffer {
    VulkanBuffer staging;
    VulkanBuffer buffer;
};

struct VulkanShader {
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
    const char            *name;
};

struct GfxDescriptorPool
{
    i32 capacity;
    i32 count;
    VkDescriptorPool vk_pool;
    VkDescriptorSet  *sets;
};

struct GfxDescriptorSet
{
    i32 id      = -1;
    i32 pool_id = -1;
    VkDescriptorSet vk_set;
};

struct GfxTexture
{
    i32                     width;
    i32                     height;
    VkFormat                vk_format;
    VkImage                 vk_image;
    VkImageView             vk_view;
    VkDeviceMemory          vk_memory;
    VkImageLayout           vk_layout;
    VkPipelineStageFlagBits vk_stage;
};


struct VulkanPipeline {
    PipelineID id;
    VkPipeline            handle;

    VkPipelineLayout      layout;

    GfxDescriptorSet      descriptor_set;

    // TODO(jesper): better name. The suffix implies the bind frequency. E.g.
    // _pipeline will contain descriptors bound once per pipeline, e.g.
    // projection matrix
    VkDescriptorSetLayout descriptor_layout_pipeline;
    VkDescriptorSetLayout descriptor_layout_material;

    VulkanShader          shaders[ShaderStage_max];

    i32                   sampler_count;
    VkSampler             *samplers;
};

struct VulkanDepthBuffer {
    VkFormat       format;
    VkImage        image;
    VkImageView    imageview;
    VkDeviceMemory memory;
};

struct VulkanSwapchain {
    VkSurfaceKHR      surface;
    VkFormat          format;
    VkSwapchainKHR    handle;
    VkExtent2D        extent;

    u32               images_count;
    VkImage           *images;
    VkImageView       *imageviews;

    VulkanDepthBuffer depth;
};

struct VulkanPhysicalDevice {
    VkPhysicalDevice                 handle;
    VkPhysicalDeviceProperties       properties;
    VkPhysicalDeviceMemoryProperties memory;
    VkPhysicalDeviceFeatures         features;
};

struct GfxFrame {
    VkCommandBuffer cmd;
    VkFence         fence;
    VkSemaphore     available;
    VkSemaphore     complete;
    VkQueryPool     timestamps;
    i32             current_timestamp;
    bool            submitted;
    u32             swapchain_index;
};

#define GFX_NUM_FRAMES (2)
#define GFX_NUM_TIMESTAMP_QUERIES (2)

struct VulkanDevice {
    f32 gpu_time = 0.0f;

    VkDevice                 handle;

    // TODO(jesper): this maps 1:1 to swapchain.extents, de-duplicate and clean
    Vector2                  resolution;
    Vector2                  offset;

    Array<GfxDescriptorPool> descriptor_pools[2];

    GfxFrame frames[GFX_NUM_FRAMES];
    i32      current_frame = 0;

    VkQueryPool timestamp_queries;

    VkInstance               instance;
    VkDebugReportCallbackEXT debug_callback;

    // TODO(jesper): split present and graphics queue
    VkQueue                  queue;
    u32                      queue_family_index;

    VulkanSwapchain          swapchain;
    VulkanPhysicalDevice     physical_device;

    VkCommandPool            command_pool;
    VkRenderPass             renderpass;

    VulkanPipeline pipelines[Pipeline_count];

    Array<VkCommandBuffer>      commands_queued;
    Array<VkSemaphore>          semaphores_submit_wait;
    Array<VkPipelineStageFlags> semaphores_submit_wait_stages;
    Array<VkSemaphore>          semaphores_submit_signal;

    Array<VkSemaphore>          present_semaphores;

    StaticArray<VkFramebuffer> framebuffers;
};

enum MaterialID {
    Material_basic2d,
    Material_phong
};

enum ResourceSlot {
    ResourceSlot_mvp,
    ResourceSlot_diffuse,
};

struct Material {
    MaterialID       id;
    // NOTE(jesper): does this make sense? we need to use the pipeline when
    // creating the material for its descriptor layout, but maybe the dependency
    // makes more sense if it goes the other way around?
    PipelineID       pipeline;
    GfxDescriptorSet descriptor_set;
};

struct PushConstants {
    u32  offset;
    u32  size;
    void *data;
};

void update_vk_texture(Texture *texture, Texture ntexture);
void init_vk_texture(Texture *texture, VkComponentMapping components);

VulkanBuffer create_vbo(void *data, usize size);
VulkanBuffer create_vbo(usize size);

VulkanBuffer create_ibo(u32 *indices, usize size);

VulkanUniformBuffer create_ubo(usize size);
void destroy_buffer(VulkanBuffer buffer);

void buffer_data(VulkanUniformBuffer ubo, void *data, usize offset, usize size);

Material create_material(PipelineID pipeline_id, MaterialID id);

void set_ubo(VulkanPipeline *pipeline, ResourceSlot slot, VulkanUniformBuffer *ubo);
void set_texture(Material *material, ResourceSlot slot, Texture *texture);

void gfx_set_texture(
    GfxDescriptorSet descriptor,
    GfxTexture texture,
    ResourceSlot slot,
    PipelineID pipeline);

GfxDescriptorSet gfx_create_descriptor(
    VkDescriptorType type,
    VkDescriptorSetLayout layout);

void gfx_bind_descriptors(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    Array<GfxDescriptorSet> descriptors);

void gfx_bind_descriptor(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    GfxDescriptorSet descriptor);

GfxTexture gfx_create_texture(
    VkFormat format,
    i32 width,
    i32 height,
    u32 usage, // VkImageUsageFlagBits
    VkMemoryPropertyFlags properties);

void gfx_transition_immediate(
    GfxTexture *texture,
    VkImageLayout layout,
    VkPipelineStageFlagBits stage);

GfxTexture gfx_create_staging_texture(
    VkFormat format,
    i32 width,
    i32 height);

void gfx_copy_texture(
    GfxTexture *src,
    GfxTexture *dst,
    Vector3i src_offset,
    Vector3i dst_offset);

Vector2 camera_from_screen(Vector2 v);
Vector3 camera_from_screen(Vector3 v);

#endif /* VULKAN_DEVICE_H */

