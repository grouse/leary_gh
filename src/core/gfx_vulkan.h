/**
 * file:    vulkan_device.h
 * created: 2017-03-13
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

enum ShaderStage {
    ShaderStage_vertex,
    ShaderStage_fragment,
    ShaderStage_max
};

enum PipelineID {
    Pipeline_font,
    Pipeline_mesh,
    Pipeline_terrain,
    Pipeline_basic2d,
    Pipeline_gui_basic,
    Pipeline_wireframe,
    Pipeline_wireframe_lines,
    Pipeline_line,

    Pipeline_count
};

struct VulkanBuffer {
    usize              size;
    VkBuffer           handle;
    VkDeviceMemory     memory;
    VkBufferUsageFlags usage;
};

struct VulkanUniformBuffer {
    VulkanBuffer staging;
    VulkanBuffer buffer;
    void *mapped = nullptr;
};

struct VulkanShader {
    VkShaderModule        module;
    VkShaderStageFlagBits stage;
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
    u32                     mip_levels = 1;
    VkFormat                vk_format;
    VkImage                 vk_image;
    VkImageView             vk_view;
    VkDeviceMemory          vk_memory;
    VkImageLayout           vk_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkPipelineStageFlagBits vk_stage  = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
};


struct VulkanPipeline {
    PipelineID id;
    VkPipeline handle;
    VkPipelineLayout layout;

    GfxDescriptorSet descriptor_set;

    VkDescriptorSetLayout set_layouts[2];

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

enum GfxQueueId {
    GFX_QUEUE_GRAPHICS,
    GFX_QUEUE_TRANSFER,
    GFX_QUEUE_COUNT,
};

struct GfxCommandBuffer {
    VkCommandBuffer handle;
    GfxQueueId      queue;
};

struct GfxQueue {
    VkQueue vk_queue;
    u32 family_index;
    u32 flags;
    VkCommandPool command_pool;
    Array<VkCommandBuffer> commands_queued;
};

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

    GfxQueue queues[GFX_QUEUE_COUNT];

    VulkanSwapchain          swapchain;
    VulkanPhysicalDevice     physical_device;

    VkRenderPass             renderpass;

    VulkanPipeline pipelines[Pipeline_count];

    Array<VkSemaphore>          semaphores_submit_wait;
    Array<VkPipelineStageFlags> semaphores_submit_wait_stages;
    Array<VkSemaphore>          semaphores_submit_signal;

    Array<VkSemaphore>          present_semaphores;

    VkFramebuffer *framebuffers;
    i32 framebuffers_count;
};

enum MaterialID {
    Material_basic2d,
    Material_phong
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

#if 0
struct Texture;
void update_vk_texture(Texture *texture, Texture *ntexture);
void init_vk_texture(Texture *texture, VkComponentMapping components);
#endif

VulkanBuffer create_vbo(void *data, usize size);
VulkanBuffer create_vbo(usize size);

VulkanBuffer create_ibo(u32 *indices, usize size);

VulkanUniformBuffer create_ubo(usize size);
void destroy_buffer(VulkanBuffer buffer);

void buffer_data(VulkanUniformBuffer ubo, void *data, usize offset, usize size);

Material create_material(PipelineID pipeline_id, MaterialID id);

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
    i32 width,
    i32 height,
    u32 mip_levels,
    VkFormat format,
    VkImageTiling tiling,
    VkComponentMapping components,
    u32 usage, // VkImageUsageFlagBits
    VkMemoryPropertyFlags properties);

GfxTexture gfx_create_texture(
    u32 width,
    u32 height,
    u32 mip_levels,
    VkFormat format,
    VkComponentMapping components,
    void *pixels);

void gfx_destroy_texture(GfxTexture texture);

void gfx_transition_immediate(
    GfxTexture *texture,
    VkImageLayout layout,
    VkPipelineStageFlagBits stage);

void gfx_copy_texture(
    GfxTexture *dst,
    GfxTexture *src);

void gfx_update_buffer(VulkanBuffer *dst, void *data, usize size);

void gfx_set_texture(
    PipelineID pipeline,
    GfxDescriptorSet descriptor,
    i32 binding,
    GfxTexture texture);

Vector2 camera_from_screen(Vector2 v);
Vector3 camera_from_screen(Vector3 v);

void create_pipeline(PipelineID id);
void gfx_flush_and_wait(GfxQueueId queue_id);
