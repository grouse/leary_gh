/**
 * file:    vulkan_device.cpp
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2018 - all rights reserved
 */

extern VulkanDevice *g_vulkan;

PFN_vkCreateDebugReportCallbackEXT   CreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT  DestroyDebugReportCallbackEXT;

Array<VulkanBuffer> g_buffers;

extern Settings g_settings;

static const char*
spv_binary_name(EShLanguage stage)
{
    switch (stage) {
    case EShLangVertex:
        return "vert.spv";
    case EShLangTessControl:
        return "tesc.spv";
    case EShLangTessEvaluation:
        return "tese.spv";
    case EShLangGeometry:
        return "geom.spv";
    case EShLangFragment:
        return "frag.spv";
    case EShLangCompute:
        return "comp.spv";
    default:
        return "unknown";
    }
}


static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback_func(VkFlags flags,
                    VkDebugReportObjectTypeEXT object_type,
                    u64 object,
                    usize location,
                    i32 message_code,
                    const char* layer,
                    const char* message,
                    void * user_data)
{
    (void)object;
    (void)location;
    (void)user_data;
    // TODO(jesper): multiple flags can be set at the same time, I don't really
    // have a way to express this in my current logging system so I let the log
    // type be decided by a severity precedence
    LogType channel;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        channel = LOG_TYPE_ERROR;
    } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        channel = LOG_TYPE_WARNING;
    } else if (flags & (VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
    {
        channel = LOG_TYPE_INFO;
    } else {
        // NOTE: this would only happen if they extend the report callback
        // flags
        channel = LOG_TYPE_INFO;
    }

    const char *object_str;
    switch (object_type) {
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
        object_str = "VkBuffer";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
        object_str = "VkBufferView";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
        object_str = "VkCommandBuffer";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
        object_str = "VkCommandPool";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT:
        object_str = "VkDebugReport";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
        object_str = "VkDescriptorPool";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
        object_str = "VkDescriptorSet";
        // TODO(jesper): silencing all the descriptor sets for now to make a
        // spurious ridiculous warning shut up. need to filter more fine-grained
        // at some point
        return VK_FALSE;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
        object_str = "VkDescriptorSetLayout";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
        object_str = "VkDevice";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
        object_str = "VkDeviceMemory";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
        object_str = "VkEvent";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
        object_str = "VkFence";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
        object_str = "VkFramebuffer";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
        object_str = "VkImage";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
        object_str = "VkImageView";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
        object_str = "VkInstance";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
        object_str = "VkPhysicalDevice";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
        object_str = "VkPipelineCache";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
        object_str = "VkPipeline";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
        object_str = "VkPipelineLayout";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
        object_str = "VkQueryPool";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
        object_str = "VkQueue";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
        object_str = "VkRenderPass";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
        object_str = "VkSampler";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
        object_str = "VkSemaphore";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
        object_str = "VkShaderModule";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
        object_str = "VkSurfaceKHR";
        break;
    case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
        object_str = "VkSwapchainKHR";
        break;

    default:
    case VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT:
        object_str = "unknown";
        break;
    }

    LOG(channel,
        "[Vulkan:%s] [%s:%d] - %s",
        layer,
        object_str,
        message_code,
        message);
    ASSERT(channel != LOG_TYPE_ERROR);

    return VK_FALSE;
}

const char *vendor_string(u32 id)
{
    switch (id) {
    case 0x10DE: return "NVIDIA";
    case 0x1002: return "AMD";
    case 0x8086: return "INTEL";
    default:     return "unknown";
    }
}

u32 find_memory_type(
    VulkanPhysicalDevice physical_device,
    u32 filter,
    VkMemoryPropertyFlags req_flags)
{
    for (u32 i = 0; i < physical_device.memory.memoryTypeCount; i++) {
        auto flags = physical_device.memory.memoryTypes[i].propertyFlags;

        if ((filter & (1 << i)) &&
            (flags & req_flags) == req_flags)
        {
            return i;
        }
    }

    return UINT32_MAX;
}

VkExtensionProperties* find_extension(
    VkExtensionProperties *extensions,
    i32 extensions_count,
    const char *name)
{
    for (i32 i = 0; i < extensions_count; i++) {
        if (strcmp(extensions[i].extensionName, name) == 0) {
            return &extensions[i];
        }
    }
    return nullptr;
}

VkLayerProperties* find_layer(
    VkLayerProperties *layers,
    i32 layers_count,
    const char *name)
{
    for (i32 i = 0; i < layers_count; i++) {
        if (strcmp(layers[i].layerName, name) == 0) {
            return &layers[i];
        }
    }
    return nullptr;
}

static const VkFormat g_depth_formats[] = {
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT
};

VkFormat find_depth_format(VulkanPhysicalDevice *device)
{
    VkFormatFeatureFlags flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    for (i32 i = 0; i < ARRAY_SIZE(g_depth_formats); i++) {
        VkFormat format = g_depth_formats[i];

        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(device->handle, format, &properties);

        if ((properties.optimalTilingFeatures & flags) == flags) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

bool has_stencil(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
           format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void present_semaphore(VkSemaphore semaphore)
{
    array_add(&g_vulkan->present_semaphores, semaphore);
}

void submit_semaphore_wait(VkSemaphore semaphore, VkPipelineStageFlags stage)
{
    array_add(&g_vulkan->semaphores_submit_wait,        semaphore);
    array_add(&g_vulkan->semaphores_submit_wait_stages, stage);
}

void submit_semaphore_signal(VkSemaphore semaphore)
{
    array_add(&g_vulkan->semaphores_submit_signal, semaphore);
}

void gfx_flush_and_wait(GfxQueueId queue_id)
{
    GfxQueue *queue = &g_vulkan->queues[queue_id];

    if (queue->commands_queued.count > 0) {
        VkSubmitInfo info = {};
        info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = (u32)queue->commands_queued.count;
        info.pCommandBuffers    = queue->commands_queued.data;

        vkQueueSubmit(queue->vk_queue, 1, &info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue->vk_queue);

        // TODO(jesper): move the command buffers into a free list instead of
        // actually freeing them, to be reset and reused with
        // begin_cmd_buffer
        vkFreeCommandBuffers(
            g_vulkan->handle,
            queue->command_pool,
            (u32)queue->commands_queued.count,
            queue->commands_queued.data);

        queue->commands_queued.count = 0;
    }
}

#define VK_LOAD_FUNC(i, f) (PFN_##f)vkGetInstanceProcAddr(i, #f)
void load_vulkan(VkInstance instance)
{
    CreateDebugReportCallbackEXT  = VK_LOAD_FUNC(instance, vkCreateDebugReportCallbackEXT);
    DestroyDebugReportCallbackEXT = VK_LOAD_FUNC(instance, vkDestroyDebugReportCallbackEXT);
}

GfxCommandBuffer gfx_begin_command(GfxQueueId queue_id)
{
    GfxQueue *queue = &g_vulkan->queues[queue_id];
    GfxCommandBuffer command = {};
    command.queue = queue_id;

    // TODO(jesper): don't alloc command buffers on demand; alloc a big
    // pool of them in the device init and keep a freelist if unused ones, or
    // ring buffer, or something
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool        = queue->command_pool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkResult result = vkAllocateCommandBuffers(g_vulkan->handle, &alloc_info, &command.handle);
    ASSERT(result == VK_SUCCESS);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(command.handle, &begin_info);
    ASSERT(result == VK_SUCCESS);

    return command;
}

void gfx_end_command(GfxCommandBuffer command)
{
    VkResult result = vkEndCommandBuffer(command.handle);
    ASSERT(result == VK_SUCCESS);

    ASSERT(command.queue < ARRAY_SIZE(g_vulkan->queues));
    GfxQueue *queue = &g_vulkan->queues[command.queue];
    array_add(&queue->commands_queued, command.handle);
}

void im_transition_image(
    VkImage image,
    VkFormat format,
    u32 mip_levels,
    VkImageLayout src,
    VkImageLayout dst,
    VkPipelineStageFlagBits psrc,
    VkPipelineStageFlagBits pdst)
{
    GfxCommandBuffer command = gfx_begin_command(GFX_QUEUE_TRANSFER);
    VkImageAspectFlags aspect_mask = 0;
    switch (dst) {
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;

        if (has_stencil(format)) {
            aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        break;
    default:
        aspect_mask |= VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    }

    VkImageMemoryBarrier barrier = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout                       = src;
    barrier.newLayout                       = dst;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = image;
    barrier.subresourceRange.aspectMask     = aspect_mask;
    // TODO(jesper): support mip layers
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    switch (src) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_GENERAL:
        barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_HOST_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    default:
        // TODO(jesper): unimplemented transfer
        ASSERT(false);
        barrier.srcAccessMask = 0;
        break;
    }

    switch (dst) {
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_GENERAL:
        barrier.dstAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_HOST_READ_BIT;
        break;
    default:
        // TODO(jesper): unimplemented transfer
        ASSERT(false);
        barrier.dstAccessMask = 0;
        break;
    }

    vkCmdPipelineBarrier(
        command.handle, psrc, pdst,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
    gfx_end_command(command);
    // TODO(jesper): move flush/wait out of here
    gfx_flush_and_wait(GFX_QUEUE_TRANSFER);
}

void gfx_transition_immediate(
    GfxTexture *texture,
    VkImageLayout layout,
    VkPipelineStageFlagBits stage)
{
    im_transition_image(
        texture->vk_image,
        texture->vk_format,
        texture->mip_levels,
        texture->vk_layout, layout,
        texture->vk_stage, stage);

    texture->vk_layout = layout;
    texture->vk_stage = stage;
}



VkImage image_create(VkFormat format,
                     u32 width,
                     u32 height,
                     VkImageTiling tiling,
                     VkImageUsageFlags usage,
                     VkMemoryPropertyFlags properties,
                     VkDeviceMemory *memory)
{
    VkImage image;

    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType         = VK_IMAGE_TYPE_2D;
    info.format            = format;
    info.extent.width      = width;
    info.extent.height     = height;
    info.extent.depth      = 1;
    info.mipLevels         = 1;
    info.arrayLayers       = 1;
    info.samples           = VK_SAMPLE_COUNT_1_BIT;
    info.tiling            = tiling;
    info.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage             = usage;
    info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(g_vulkan->handle, &info, nullptr, &image);
    ASSERT(result == VK_SUCCESS);

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(g_vulkan->handle, image, &mem_requirements);

    // TODO(jesper): look into host coherent
    u32 memory_type = find_memory_type(g_vulkan->physical_device,
                                       mem_requirements.memoryTypeBits,
                                       properties);
    ASSERT(memory_type != UINT32_MAX);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize       = mem_requirements.size;
    alloc_info.memoryTypeIndex      = memory_type;

    result = vkAllocateMemory(g_vulkan->handle, &alloc_info, nullptr, memory);
    ASSERT(result == VK_SUCCESS);

    vkBindImageMemory(g_vulkan->handle, image, *memory, 0);

    return image;
}

VulkanSwapchain swapchain_create(VulkanPhysicalDevice *physical_device,
                                 VkSurfaceKHR surface)

{
    VkResult result;
    VulkanSwapchain swapchain = {};
    swapchain.surface = surface;

    // figure out the color space for the swapchain
    u32 formats_count;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device->handle,
        swapchain.surface,
        &formats_count,
        nullptr);
    ASSERT(result == VK_SUCCESS);

    auto formats = alloc_array(g_frame, VkSurfaceFormatKHR, formats_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        physical_device->handle,
        swapchain.surface,
        &formats_count,
        formats);
    ASSERT(result == VK_SUCCESS);

    VkColorSpaceKHR surface_colorspace = {};

    // NOTE: if impl. reports only 1 surface format and that is undefined
    // it has no preferred format, so we choose BGRA8_UNORM
    if (formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        // TODO(jesper): color space?
        swapchain.format = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
        for (u32 i = 0; i < formats_count; i++) {
            // TODO(jesper): properly check sRGB
            if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) {
                swapchain.format = formats[i].format;
                surface_colorspace = formats[i].colorSpace;
                goto format_found;
            }
        }

        surface_colorspace = formats[0].colorSpace;
        swapchain.format = formats[0].format;
    }

format_found:

    VkSurfaceCapabilitiesKHR surface_capabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physical_device->handle,
        swapchain.surface,
        &surface_capabilities);
    ASSERT(result == VK_SUCCESS);

    // figure out the present mode for the swapchain
    u32 present_modes_count;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device->handle,
        swapchain.surface,
        &present_modes_count,
        nullptr);
    ASSERT(result == VK_SUCCESS);

    auto present_modes = alloc_array(g_frame, VkPresentModeKHR, present_modes_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physical_device->handle,
        swapchain.surface,
        &present_modes_count,
        present_modes);
    ASSERT(result == VK_SUCCESS);

    VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < present_modes_count; ++i) {
        const VkPresentModeKHR &mode = present_modes[i];

        if (g_settings.video.vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            surface_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }

        if (!g_settings.video.vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            surface_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        }
    }

    swapchain.extent = surface_capabilities.currentExtent;
    if (swapchain.extent.width == (u32) (-1)) {
        // TODO(grouse): clean up usage of window dimensions
        ASSERT(g_settings.video.resolution.width  >= 0);
        ASSERT(g_settings.video.resolution.height >= 0);

        swapchain.extent.width  = (u32)g_settings.video.resolution.width;
        swapchain.extent.height = (u32)g_settings.video.resolution.height;
    }
    g_vulkan->resolution.x = (f32)swapchain.extent.width;
    g_vulkan->resolution.y = (f32)swapchain.extent.height;

    // TODO(jesper): this offset corresponds to the window border/titlebar
    // offsetse, do something much better here!
    g_vulkan->offset = { 1.0f, 14.0f };

    // TODO: determine the number of VkImages to use in the swapchain
    u32 desired_swapchain_images = surface_capabilities.minImageCount + 1;

    if (surface_capabilities.maxImageCount > 0) {
        desired_swapchain_images = MIN(desired_swapchain_images,
                                       surface_capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface               = swapchain.surface;
    create_info.minImageCount         = desired_swapchain_images;
    create_info.imageFormat           = swapchain.format;
    create_info.imageColorSpace       = surface_colorspace;
    create_info.imageExtent           = swapchain.extent;
    create_info.imageArrayLayers      = 1;
    create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 1;
    create_info.pQueueFamilyIndices   = &g_vulkan->queues[GFX_QUEUE_GRAPHICS].family_index;
    create_info.preTransform          = surface_capabilities.currentTransform;
    create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode           = surface_present_mode;
    create_info.clipped               = VK_TRUE;
    create_info.oldSwapchain          = VK_NULL_HANDLE;

    result = vkCreateSwapchainKHR(g_vulkan->handle,
                                  &create_info,
                                  nullptr,
                                  &swapchain.handle);

    ASSERT(result == VK_SUCCESS);

    result = vkGetSwapchainImagesKHR(g_vulkan->handle,
                                     swapchain.handle,
                                     &swapchain.images_count,
                                     nullptr);
    ASSERT(result == VK_SUCCESS);

    swapchain.images = alloc_array(g_frame, VkImage, swapchain.images_count);

    result = vkGetSwapchainImagesKHR(g_vulkan->handle,
                                     swapchain.handle,
                                     &swapchain.images_count,
                                     swapchain.images);
    ASSERT(result == VK_SUCCESS);

    swapchain.imageviews = alloc_array(g_persistent, VkImageView, swapchain.images_count);

    VkImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource_range.baseMipLevel   = 0;
    subresource_range.levelCount     = 1;
    subresource_range.baseArrayLayer = 0;
    subresource_range.layerCount     = 1;

    VkImageViewCreateInfo imageview_create_info = {};
    imageview_create_info.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageview_create_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    imageview_create_info.format           = swapchain.format;
    imageview_create_info.subresourceRange = subresource_range;

    for (i32 i = 0; i < (i32) swapchain.images_count; ++i)
    {
        imageview_create_info.image = swapchain.images[i];

        result = vkCreateImageView(g_vulkan->handle, &imageview_create_info, nullptr,
                                   &swapchain.imageviews[i]);
        ASSERT(result == VK_SUCCESS);
    }

    swapchain.depth.format = find_depth_format(physical_device);
    ASSERT(swapchain.depth.format != VK_FORMAT_UNDEFINED);

    swapchain.depth.image = image_create(
        swapchain.depth.format,
        swapchain.extent.width,
        swapchain.extent.height,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &swapchain.depth.memory);

    VkImageViewCreateInfo view_info = {};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = swapchain.depth.image;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = swapchain.depth.format,
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    result = vkCreateImageView(g_vulkan->handle, &view_info, nullptr,
                               &swapchain.depth.imageview);
    ASSERT(result == VK_SUCCESS);

    im_transition_image(
        swapchain.depth.image,
        swapchain.depth.format,
        1,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

    return swapchain;
}

VkSampler create_sampler(f32 max_lod, f32 lod_bias)
{
    VkSampler sampler;

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter               = VK_FILTER_LINEAR;
    sampler_info.minFilter               = VK_FILTER_LINEAR;
    sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable           = VK_FALSE;
    sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;

    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.maxLod     = max_lod;
    sampler_info.mipLodBias = lod_bias;

    VkResult result = vkCreateSampler(g_vulkan->handle,
                                      &sampler_info,
                                      nullptr,
                                      &sampler);
    ASSERT(result == VK_SUCCESS);

    return sampler;
}

bool load_shader(char *src, i32 size, glslang::TShader *shader)
{
    EShLanguage stage = shader->getStage();

    std::string preamble;
    switch (stage) {
    case EShLangVertex:
        preamble += "#define VERTEX_SHADER\n";
        break;
    case EShLangFragment:
        preamble += "#define FRAGMENT_SHADER\n";
        break;
    case EShLangTessControl:
    case EShLangTessEvaluation:
    case EShLangGeometry:
    case EShLangCompute:
        LOG_ERROR("unsupported shader type");
        return false;
    case EShLangCount:
        LOG_ERROR("invalid shader type");
        return false;
    }

    shader->setStringsWithLengths(&src, &size, 1);
    shader->setPreamble(preamble.c_str());
    shader->setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    TBuiltInResource resources = {
        32, 6, 32, 32, 64, 4096, 64, 32, 80, 32, 4096, 32, 128, 8, 16, 16,
        15, -8, 7, 8, 65535, 65535, 65535, 1024, 1024, 64, 1024, 16, 8, 8,
        1, 60, 64, 64, 128, 128, 8, 8, 8, 0, 0, 0, 0, 0, 8, 8, 16, 256, 1024,
        1024, 64, 128, 128, 16, 1024, 4096, 128, 128, 16, 1024, 120, 32, 64,
        16, 0, 0, 0, 0, 8, 8, 1, 0, 0, 0, 0, 1, 1, 16384, 4, 64, 8, 8, 4,
        { 1, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    return shader->parse(&resources, 100, false, (EShMessages)0xffffff);
}

void create_pipeline(PipelineID id)
{
    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    VkResult result;

    VulkanPipeline pipeline = g_vulkan->pipelines[id];
    pipeline.id = id;

    StringView shader_name;
    switch (id) {
    case Pipeline_font:
        shader_name = "font.glsl";
        break;
    case Pipeline_basic2d:
        shader_name = "basic2d.glsl";
        break;
    case Pipeline_gui_basic:
        shader_name = "gui_basic.glsl";
        break;
    case Pipeline_terrain:
        shader_name = "terrain.glsl";
        break;
    case Pipeline_mesh:
        shader_name = "mesh.glsl";
        break;
    case Pipeline_line:
        shader_name = "line.glsl";
        break;
    case Pipeline_wireframe:
    case Pipeline_wireframe_lines:
        shader_name = "wireframe.glsl";
        break;
    default: break;
    }

    FilePath shader_path = resolve_file_path(GamePath_shaders, shader_name, g_frame);

    usize size;
    char *src = read_file(shader_path, &size, g_frame);

    glslang::TProgram program;
    glslang::TShader vtx(EShLangVertex);
    glslang::TShader frag(EShLangFragment);

    if (load_shader(src, (i32)size, &vtx) == false) {
        LOG_ERROR("failed parsing glsl vertex shader: %s, %s",
                  vtx.getInfoLog(),
                  vtx.getInfoDebugLog());
        return;
    }

    if (load_shader(src, (i32)size, &frag) == false) {
        LOG_ERROR("failed parsing glsl fragment shader: %s, %s",
                  vtx.getInfoLog(),
                  vtx.getInfoDebugLog());
        return;
    }

    program.addShader(&vtx);
    program.addShader(&frag);

    if (program.link((EShMessages)0xffffff) == false) {
        const char* info_log = program.getInfoLog();
        const char* debug_info_log = program.getInfoDebugLog();
        LOG_ERROR("failed linking glsl %s, %s", info_log, debug_info_log);
    }

    glslang::SpvOptions spvOptions;
    spvOptions.generateDebugInfo = true;
    spvOptions.disableOptimizer  = true;
    spvOptions.optimizeSize      = false;
    spvOptions.disassemble       = false;
    spvOptions.validate          = false;

    for (i32 stage = 0; stage < EShLangCount; stage++) {
        if (program.getIntermediate((EShLanguage)stage)) {

            const char *ext = spv_binary_name((EShLanguage)stage);
            std::vector<unsigned int> spirv;

            spv::SpvBuildLogger logger;
            glslang::GlslangToSpv(
                *program.getIntermediate((EShLanguage)stage),
                spirv,
                &logger,
                &spvOptions);

            glslang::OutputSpvBin(spirv, ext);

            VulkanShader shader = {};

            switch (stage) {
            case EShLangVertex:
                shader.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case EShLangFragment:
                shader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            }

            VkShaderModuleCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            info.codeSize = spirv.size() * sizeof spirv[0];
            info.pCode    = spirv.data();

            result = vkCreateShaderModule(
                g_vulkan->handle, &info,
                nullptr,
                &shader.module);
            ASSERT(result == VK_SUCCESS);

            switch (stage) {
            case EShLangVertex:
                pipeline.shaders[ShaderStage_vertex] = shader;
                break;
            case EShLangFragment:
                pipeline.shaders[ShaderStage_fragment] = shader;
                break;
            }
        }
    }

    if (pipeline.handle == VK_NULL_HANDLE) {
        Array<VkDescriptorSetLayoutBinding> sampler_bindings;
        Array<VkDescriptorSetLayoutBinding> uniform_bindings;
        Array<VkPushConstantRange> push_constants;

        init_array(&sampler_bindings, g_stack);
        init_array(&uniform_bindings, g_stack);
        init_array(&push_constants, g_stack);

        program.buildReflection();
        i32 num_uniforms = program.getNumLiveUniformVariables();
        i32 num_uniform_blocks = program.getNumLiveUniformBlocks();

        i32 sampler_binding = 1;
        i32 uniform_binding = 0;

        for (i32  i = 0; i < num_uniforms; i++) {
            const char *name = program.getUniformName(i);
            const glslang::TType *type = program.getUniformTType(i);

            EShLanguageMask stages = program.getUniformStages(i);
            VkShaderStageFlags vk_stages = 0;

            if (stages & EShLangVertexMask) {
                vk_stages |= VK_SHADER_STAGE_VERTEX_BIT;
            }

            if (stages & EShLangFragmentMask) {
                vk_stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }

            const glslang::TQualifier &qualifier = type->getQualifier();

            switch (type->getBasicType()) {
            case glslang::EbtSampler: {
                VkDescriptorSetLayoutBinding binding = {};
                binding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                binding.descriptorCount = 1;
                binding.stageFlags      = vk_stages;

                if (qualifier.hasBinding()) {
                    binding.binding = qualifier.layoutBinding;
                    sampler_binding = binding.binding + 1;
                } else {
                    binding.binding = sampler_binding++;
                }

                array_add(&sampler_bindings, binding);
            } break;
            default: break;
            }

            LOG_INFO("uniform: %s", name);
        }

        for (i32  i = 0; i < num_uniform_blocks; i++) {
            const glslang::TType *type = program.getUniformBlockTType(i);

            EShLanguageMask stages = program.getUniformStages(i);
            VkShaderStageFlags vk_stages = 0;

            if (stages & EShLangVertexMask) {
                vk_stages |= VK_SHADER_STAGE_VERTEX_BIT;
            }

            if (stages & EShLangFragmentMask) {
                vk_stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
            }

            const glslang::TQualifier &qualifier = type->getQualifier();
            if (qualifier.storage == glslang::EvqUniform) {
                if (qualifier.layoutPushConstant == false) {
                    VkDescriptorSetLayoutBinding binding = {};
                    binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    binding.descriptorCount = 1;
                    binding.stageFlags      = vk_stages;

                    if (qualifier.hasBinding()) {
                        binding.binding = qualifier.layoutBinding;
                        uniform_binding = binding.binding + 1;
                    } else {
                        binding.binding = uniform_binding++;
                    }

                    array_add(&uniform_bindings, binding);
                } else {
                    VkPushConstantRange pc = {};
                    pc.stageFlags = vk_stages;
                    // TODO(jesper): can we have several push constants with
                    // offsets?
                    pc.offset     = 0;
                    pc.size       = program.getUniformBlockSize(i);
                    array_add(&push_constants, pc);
                }
            }
        }

        // NOTE(jesper): the only thing stopping us from creating these from the
        // reflection data is the LOD bias and max mip level
        switch (id) {
        case Pipeline_font:
        case Pipeline_basic2d:
            pipeline.sampler_count = 1;
            pipeline.samplers = alloc_array(g_persistent, VkSampler, pipeline.sampler_count);
            pipeline.samplers[0] = create_sampler(10.0f, 0.0f);
            break;
        case Pipeline_mesh:
            pipeline.sampler_count = 2;
            pipeline.samplers = alloc_array(g_persistent, VkSampler, pipeline.sampler_count);
            pipeline.samplers[0] = create_sampler(20.0f, -2.0f);
            pipeline.samplers[1] = create_sampler(20.0f, -2.0f);
            break;
        case Pipeline_terrain:
            pipeline.sampler_count = 5;
            pipeline.samplers = alloc_array(g_persistent, VkSampler, pipeline.sampler_count);
            pipeline.samplers[0] = create_sampler(20.0f, -2.0f);
            pipeline.samplers[1] = create_sampler(20.0f, -2.0f);
            pipeline.samplers[2] = create_sampler(20.0f, -2.0f);
            pipeline.samplers[3] = create_sampler(20.0f, -2.0f);
            pipeline.samplers[4] = create_sampler(0.0f, 0.0f);
            break;
        default: break;
        }

        ASSERT(sampler_bindings.count == pipeline.sampler_count);

        auto layouts = create_array<VkDescriptorSetLayout>(g_frame);

        switch (id) {
        case Pipeline_basic2d:
        case Pipeline_font: {
            VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
            descriptor_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptor_layout_info.bindingCount = (i32)sampler_bindings.count;
            descriptor_layout_info.pBindings    = sampler_bindings.data;

            result = vkCreateDescriptorSetLayout(
                g_vulkan->handle,
                &descriptor_layout_info,
                nullptr,
                &pipeline.set_layouts[1]);
            ASSERT(result == VK_SUCCESS);
            array_add(&layouts, pipeline.set_layouts[1]);
        } break;
        case Pipeline_mesh: {
            { // pipeline
                VkDescriptorSetLayoutCreateInfo layout_info = {};
                layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = (u32)uniform_bindings.count;
                layout_info.pBindings    = uniform_bindings.data;

                result = vkCreateDescriptorSetLayout(
                    g_vulkan->handle,
                    &layout_info,
                    nullptr,
                    &pipeline.set_layouts[0]);
                ASSERT(result == VK_SUCCESS);

                array_add(&layouts, pipeline.set_layouts[0]);
            }

            { // material
                VkDescriptorSetLayoutCreateInfo layout_info = {};
                layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = (u32)sampler_bindings.count;
                layout_info.pBindings    = sampler_bindings.data;

                result = vkCreateDescriptorSetLayout(
                    g_vulkan->handle,
                    &layout_info,
                    nullptr,
                    &pipeline.set_layouts[1]);
                ASSERT(result == VK_SUCCESS);

                array_add(&layouts, pipeline.set_layouts[1]);
            }
        } break;
        case Pipeline_terrain: {
            { // pipeline
                VkDescriptorSetLayoutCreateInfo layout_info = {};
                layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = (u32)uniform_bindings.count;
                layout_info.pBindings    = uniform_bindings.data;

                result = vkCreateDescriptorSetLayout(
                    g_vulkan->handle,
                    &layout_info,
                    nullptr,
                    &pipeline.set_layouts[0]);
                ASSERT(result == VK_SUCCESS);

                array_add(&layouts, pipeline.set_layouts[0]);
            }

            { // material
                VkDescriptorSetLayoutCreateInfo layout_info = {};
                layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = (u32)sampler_bindings.count;
                layout_info.pBindings    = sampler_bindings.data;

                result = vkCreateDescriptorSetLayout(
                    g_vulkan->handle,
                    &layout_info,
                    nullptr,
                    &pipeline.set_layouts[1]);
                ASSERT(result == VK_SUCCESS);

                array_add(&layouts, pipeline.set_layouts[1]);
            }
        } break;
        case Pipeline_wireframe:
        case Pipeline_wireframe_lines: {
            { // pipeline
                VkDescriptorSetLayoutCreateInfo layout_info = {};
                layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = (u32)uniform_bindings.count;
                layout_info.pBindings    = uniform_bindings.data;

                result = vkCreateDescriptorSetLayout(
                    g_vulkan->handle,
                    &layout_info,
                    nullptr,
                    &pipeline.set_layouts[0]);
                ASSERT(result == VK_SUCCESS);

                array_add(&layouts, pipeline.set_layouts[0]);
            }
        } break;
        case Pipeline_line: {
            { // pipeline
                VkDescriptorSetLayoutCreateInfo layout_info = {};
                layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layout_info.bindingCount = (u32)uniform_bindings.count;
                layout_info.pBindings    = uniform_bindings.data;

                result = vkCreateDescriptorSetLayout(
                    g_vulkan->handle,
                    &layout_info,
                    nullptr,
                    &pipeline.set_layouts[0]);
                ASSERT(result == VK_SUCCESS);

                array_add(&layouts, pipeline.set_layouts[0]);
            }
        } break;
        default: break;
        }

        switch (id) {
        case Pipeline_terrain:
        case Pipeline_line:
        case Pipeline_wireframe:
        case Pipeline_wireframe_lines:
        case Pipeline_mesh: {
            pipeline.descriptor_set = gfx_create_descriptor(
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                layouts[0]);
            ASSERT(pipeline.descriptor_set.id != -1);
        } break;
        default: break;
        }

        VkPipelineLayoutCreateInfo layout_info = {};
        layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount         = (i32)layouts.count;
        layout_info.pSetLayouts            = layouts.data;
        layout_info.pushConstantRangeCount = push_constants.count;
        layout_info.pPushConstantRanges    = push_constants.data;

        result = vkCreatePipelineLayout(
            g_vulkan->handle,
            &layout_info,
            nullptr,
            &pipeline.layout);
        ASSERT(result == VK_SUCCESS);
    }

    auto vbinds = create_array<VkVertexInputBindingDescription>(g_stack);
    auto vdescs = create_array<VkVertexInputAttributeDescription>(g_stack);

    switch (id) {
    case Pipeline_font:
        array_add(&vbinds, { 0, sizeof(f32) * 8, VK_VERTEX_INPUT_RATE_VERTEX });

        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
        array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(f32) * 2 });
        array_add(&vdescs, { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(f32) * 4 });
        break;
    case Pipeline_gui_basic:
        array_add(&vbinds, { 0, sizeof(f32) * 6, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
        array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(f32) * 2 });
        break;
    case Pipeline_basic2d:
        array_add(&vbinds, { 0, sizeof(f32) * 4, VK_VERTEX_INPUT_RATE_VERTEX });

        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 });
        array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(f32) * 2 });
        break;
    case Pipeline_mesh:
        array_add(&vbinds, { 0, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 1, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 2, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 3, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 4, sizeof(f32) * 2, VK_VERTEX_INPUT_RATE_VERTEX });

        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 3, 3, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 4, 4, VK_FORMAT_R32G32_SFLOAT,    0 });
        break;
    case Pipeline_terrain:
        array_add(&vbinds, { 0, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 1, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 2, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 3, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vbinds, { 4, sizeof(f32) * 2, VK_VERTEX_INPUT_RATE_VERTEX });

        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 1, 1, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 3, 3, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 4, 4, VK_FORMAT_R32G32_SFLOAT,    0 });
        break;
    case Pipeline_line:
        array_add(&vbinds, { 0, sizeof(f32) * 7, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        array_add(&vdescs, { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(f32) * 3 });
        break;
    case Pipeline_wireframe:
    case Pipeline_wireframe_lines:
        array_add(&vbinds, { 0, sizeof(f32) * 3, VK_VERTEX_INPUT_RATE_VERTEX });
        array_add(&vdescs, { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
        break;
    default: break;
    }

    VkPipelineVertexInputStateCreateInfo vii = {};
    vii.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount   = vbinds.count;
    vii.pVertexBindingDescriptions      = vbinds.data;
    vii.vertexAttributeDescriptionCount = vdescs.count;
    vii.pVertexAttributeDescriptions    = vdescs.data;

    VkPipelineInputAssemblyStateCreateInfo iai = {};
    iai.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.primitiveRestartEnable = VK_FALSE;

    switch (id) {
    case Pipeline_line:
    case Pipeline_wireframe_lines:
        iai.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    default:
        iai.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    }

    VkViewport viewport = {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = (f32) g_vulkan->swapchain.extent.width;
    viewport.height   = (f32) g_vulkan->swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_vulkan->swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports    = &viewport;
    viewport_info.scissorCount  = 1;
    viewport_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo raster = {};
    raster.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.depthClampEnable        = VK_FALSE;
    raster.rasterizerDiscardEnable = VK_FALSE;
    raster.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    raster.depthBiasEnable         = VK_FALSE;
    raster.lineWidth               = 1.0f;

    switch (id) {
    case Pipeline_line:
    case Pipeline_wireframe:
    case Pipeline_wireframe_lines:
        raster.polygonMode = VK_POLYGON_MODE_LINE;
        raster.cullMode    = VK_CULL_MODE_NONE;
        break;
    case Pipeline_gui_basic:
    case Pipeline_font:
    case Pipeline_basic2d:
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode    = VK_CULL_MODE_NONE;
        break;
    default:
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode    = VK_CULL_MODE_BACK_BIT;
        break;
    }

    VkPipelineColorBlendAttachmentState cba = {};
    switch (id) {
    case Pipeline_gui_basic:
    case Pipeline_font:
        cba.blendEnable         = VK_TRUE;
        cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cba.colorBlendOp        = VK_BLEND_OP_ADD;
        cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cba.alphaBlendOp        = VK_BLEND_OP_ADD;
        break;
    default:
        cba.blendEnable         = VK_FALSE;
        break;
    }
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                         VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT |
                         VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbi = {};
    cbi.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbi.logicOpEnable     = VK_FALSE;
    cbi.logicOp           = VK_LOGIC_OP_CLEAR;
    cbi.attachmentCount   = 1;
    cbi.pAttachments      = &cba;
    cbi.blendConstants[0] = 0.0f;
    cbi.blendConstants[1] = 0.0f;
    cbi.blendConstants[2] = 0.0f;
    cbi.blendConstants[3] = 0.0f;

    VkPipelineMultisampleStateCreateInfo msi = {};
    msi.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    msi.sampleShadingEnable   = VK_FALSE;
    msi.minSampleShading      = 0;
    msi.pSampleMask           = nullptr;
    msi.alphaToCoverageEnable = VK_FALSE;
    msi.alphaToOneEnable      = VK_FALSE;

    auto stages = create_array<VkPipelineShaderStageCreateInfo>(g_stack);
    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_vertex].stage,
        pipeline.shaders[ShaderStage_vertex].module,
        "main",
        nullptr
    });

    array_add(&stages, {
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr, 0,
        pipeline.shaders[ShaderStage_fragment].stage,
        pipeline.shaders[ShaderStage_fragment].module,
        "main",
        nullptr
    });

    VkPipelineDepthStencilStateCreateInfo ds = {};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthBoundsTestEnable = VK_FALSE;
    switch (id) {
    case Pipeline_mesh:
    case Pipeline_terrain:
    case Pipeline_line:
    case Pipeline_wireframe:
    case Pipeline_wireframe_lines:
        ds.depthTestEnable  = VK_TRUE;
        ds.depthWriteEnable = VK_TRUE;
        ds.depthCompareOp   = VK_COMPARE_OP_LESS;
        break;
    default:
        ds.depthTestEnable  = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
        break;
    }

    VkGraphicsPipelineCreateInfo pinfo = {};
    pinfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pinfo.stageCount          = (i32)stages.count;
    pinfo.pStages             = stages.data;
    pinfo.pVertexInputState   = &vii;
    pinfo.pInputAssemblyState = &iai;
    pinfo.pViewportState      = &viewport_info;
    pinfo.pRasterizationState = &raster;
    pinfo.pMultisampleState   = &msi;
    pinfo.pColorBlendState    = &cbi;
    pinfo.pDepthStencilState  = &ds;
    pinfo.layout              = pipeline.layout;
    pinfo.renderPass          = g_vulkan->renderpass;
    pinfo.basePipelineHandle  = VK_NULL_HANDLE;
    pinfo.basePipelineIndex   = -1;

    result = vkCreateGraphicsPipelines(
        g_vulkan->handle,
        VK_NULL_HANDLE,
        1,
        &pinfo,
        nullptr,
        &pipeline.handle);
    ASSERT(result == VK_SUCCESS);

    if (result == VK_SUCCESS) {
        VulkanPipeline *existing = &g_vulkan->pipelines[id];

        if (existing->handle != VK_NULL_HANDLE) {
            vkDestroyShaderModule(
                g_vulkan->handle,
                existing->shaders[ShaderStage_vertex].module,
                nullptr);
            vkDestroyShaderModule(
                g_vulkan->handle,
                existing->shaders[ShaderStage_fragment].module,
                nullptr);
            vkDestroyPipeline(g_vulkan->handle, existing->handle, nullptr);
        }

        g_vulkan->pipelines[id] = pipeline;
    } else {
        LOG_ERROR("Failed creating vulkan pipeline");
    }
}

void image_copy(
    u32 width,
    u32 height,
    VkImage src,
    VkImage dst)
{
    GfxCommandBuffer command = gfx_begin_command(GFX_QUEUE_TRANSFER);

    // TODO(jesper): support mip layers
    VkImageSubresourceLayers subresource = {};
    subresource.aspectMask               = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.baseArrayLayer           = 0;
    subresource.mipLevel                 = 0;
    subresource.layerCount               = 1;

    VkImageCopy region = {};
    // TODO(jesper): support copying images from/to different subresources?
    region.srcSubresource = subresource;
    region.dstSubresource = subresource;
    // TODO(jesper): support subregion copy
    region.srcOffset = { 0, 0, 0 };
    region.dstOffset = { 0, 0, 0 };
    region.extent.width = width;
    region.extent.height = height;
    region.extent.depth = 1;

    vkCmdCopyImage(
        command.handle,
        src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &region);

    gfx_end_command(command);
    // TODO(jesper): move flush_wait out of here
    gfx_flush_and_wait(GFX_QUEUE_GRAPHICS);
}

void vkdebug_create()
{
    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

    create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
        VK_DEBUG_REPORT_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
        VK_DEBUG_REPORT_DEBUG_BIT_EXT;

    create_info.pfnCallback = &debug_callback_func;

    VkResult result = CreateDebugReportCallbackEXT(g_vulkan->instance,
                                                   &create_info,
                                                   nullptr,
                                                   &g_vulkan->debug_callback);
    ASSERT(result == VK_SUCCESS);
}

void vkdebug_destroy()
{
    DestroyDebugReportCallbackEXT(g_vulkan->instance, g_vulkan->debug_callback, nullptr);
    g_vulkan->debug_callback = nullptr;
}

void gfx_create_command_pools(GfxQueue *queue)
{
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = queue->family_index;

    VkResult result = vkCreateCommandPool(
        g_vulkan->handle,
        &create_info,
        nullptr,
        &queue->command_pool);
    ASSERT(result == VK_SUCCESS);

    init_array(&queue->commands_queued, g_heap);
}

void init_vulkan()
{
    // NOTE(jesper): initialise glslang compiler/linker
    ShInitialize();

    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    g_vulkan = ialloc<VulkanDevice>(g_persistent);
    init_array(&g_vulkan->descriptor_pools[0], g_heap);
    init_array(&g_vulkan->descriptor_pools[1], g_heap);
    init_array(&g_buffers, g_heap);

    VkResult result;
    /**************************************************************************
     * Create VkInstance
     *************************************************************************/
    {
        // NOTE(jesper): currently we don't store any internal state about
        // which ones we've enabled.
        u32 count = 0;
        result = vkEnumerateInstanceLayerProperties(&count, nullptr);
        ASSERT(result == VK_SUCCESS);

        auto supported_layers = alloc_array(g_stack, VkLayerProperties, count);
        i32 supported_layers_count = count;

        result = vkEnumerateInstanceLayerProperties(&count, supported_layers);
        ASSERT(supported_layers_count == (i32)count);
        ASSERT(result == VK_SUCCESS);

        for (i32 i = 0; i < supported_layers_count; i++) {
            LOG("VkLayerProperties[%d]", i);
            LOG("  layerName            : %s",
                      supported_layers[i].layerName);
            LOG("  specVersion          : %u.%u.%u",
                      VK_VERSION_MAJOR(supported_layers[i].specVersion),
                      VK_VERSION_MINOR(supported_layers[i].specVersion),
                      VK_VERSION_PATCH(supported_layers[i].specVersion));
            LOG("  implementationVersion: %u",
                      supported_layers[i].implementationVersion);
            LOG("  description          : %s",
                      supported_layers[i].description);
        }

        result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        ASSERT(result == VK_SUCCESS);

        auto supported_extensions = alloc_array(g_stack, VkExtensionProperties, count);
        i32 supported_extensions_count = count;

        result = vkEnumerateInstanceExtensionProperties(
            nullptr,
            &count, supported_extensions);
        ASSERT(supported_extensions_count == (i32)count);
        ASSERT(result == VK_SUCCESS);

        for (i32 i = 0; i < supported_extensions_count; i++) {
            LOG("vkExtensionProperties[%d]", i);
            LOG("  extensionName: %s", supported_extensions[i].extensionName);
            LOG("  specVersion  : %u", supported_extensions[i].specVersion);
        }


        // NOTE(jesper): we might want to store these in the device for future
        // usage/debug information
        auto required_extensions = create_array<const char*>(g_stack);
        array_add(&required_extensions, VK_KHR_SURFACE_EXTENSION_NAME);
        gfx_platform_required_extensions(&required_extensions);

        auto debug_extensions = create_array<const char*>(g_stack);
        array_add(&debug_extensions, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

        auto debug_layers = create_array<const char*>(g_stack);
        array_add(&debug_layers, "VK_LAYER_LUNARG_standard_validation");

        auto enabled_extensions = create_array<const char*>(g_stack);
        auto enabled_layers     = create_array<const char*>(g_stack);

        for (const char *name : required_extensions) {
            VkExtensionProperties *ext = find_extension(
                supported_extensions,
                supported_extensions_count,
                name);
            if (ext == nullptr) {
                LOG_ERROR("missing requied vulkan extension: %s", name);
                continue;
            }

            array_add(&enabled_extensions, (const char*)&ext->extensionName[0]);
        }

        for (const char *name: debug_extensions) {
            VkExtensionProperties *ext = find_extension(
                supported_extensions,
                supported_extensions_count,
                name);
            if (ext == nullptr) {
                LOG_WARNING("missing debug vulkan extension: %s", name);
                continue;
            }

            array_add(&enabled_extensions, (const char*)&ext->extensionName[0]);
        }

        for (const char *name : debug_layers) {
            VkLayerProperties *layer = find_layer(
                supported_layers,
                supported_layers_count,
                name);
            if (layer == nullptr) {
                LOG_WARNING("missing debug vulkan extension: %s", name);
                continue;
            }

            array_add(&enabled_layers, (const char*)&layer->layerName[0]);
        }

        // Create the VkInstance
        VkApplicationInfo app_info = {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = "leary";
        app_info.applicationVersion = 1;
        app_info.pEngineName        = "leary";
        app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 61);

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledLayerCount       = enabled_layers.count;
        create_info.ppEnabledLayerNames     = enabled_layers.data;
        create_info.enabledExtensionCount   = enabled_extensions.count;
        create_info.ppEnabledExtensionNames = enabled_extensions.data;

        result = vkCreateInstance(&create_info, nullptr, &g_vulkan->instance);
        ASSERT(result == VK_SUCCESS);
    }

    /**************************************************************************
     * Create debug callbacks
     *************************************************************************/
    {
        load_vulkan(g_vulkan->instance);
        vkdebug_create();
    }

    /**************************************************************************
     * Create and choose VkPhysicalDevice
     *************************************************************************/
    {
        u32 count = 0;
        result = vkEnumeratePhysicalDevices(g_vulkan->instance, &count, nullptr);
        ASSERT(result == VK_SUCCESS);

        auto physical_devices = alloc_array(g_frame, VkPhysicalDevice, count);
        result = vkEnumeratePhysicalDevices(g_vulkan->instance,
                                            &count, physical_devices);
        ASSERT(result == VK_SUCCESS);

        bool found_device = false;
        for (u32 i = 0; i < count; i++) {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

            LOG("VkPhysicalDeviceProperties[%u]", i);
            LOG("  apiVersion    : %d.%d.%d",
                      VK_VERSION_MAJOR(properties.apiVersion),
                      VK_VERSION_MINOR(properties.apiVersion),
                      VK_VERSION_PATCH(properties.apiVersion));
            // NOTE(jesper): only confirmed to be accurate, by experimentation,
            // on nvidia
            LOG("  driverVersion : %u.%u",
                      (properties.driverVersion >> 22),
                      (properties.driverVersion >> 14) & 0xFF);
            LOG("  vendorID      : 0x%X %s",
                      properties.vendorID,
                      vendor_string(properties.vendorID));
            LOG("  deviceID      : 0x%X", properties.deviceID);
            switch (properties.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                LOG("  deviceType: Integrated GPU");
                if (!found_device) {
                    g_vulkan->physical_device.handle     = physical_devices[i];
                    g_vulkan->physical_device.properties = properties;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                LOG("  deviceType    : Discrete GPU");
                g_vulkan->physical_device.handle     = physical_devices[i];
                g_vulkan->physical_device.properties = properties;
                found_device = true;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                LOG("  deviceType    : Virtual GPU");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                LOG("  deviceType    : CPU");
                break;
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                LOG("  deviceTyacquire_swapchainpe    : Unknown");
                break;
            }
            LOG("  deviceName    : %s", properties.deviceName);
        }

        vkGetPhysicalDeviceMemoryProperties(g_vulkan->physical_device.handle,
                                            &g_vulkan->physical_device.memory);

        vkGetPhysicalDeviceFeatures(g_vulkan->physical_device.handle,
                                    &g_vulkan->physical_device.features);
    }

    // NOTE(jesper): this is so annoying. The surface belongs with the swapchain
    // (imo), but to create the swapchain we need the device, and to create the
    // device we need the surface
    VkSurfaceKHR surface;
    result = gfx_platform_create_surface(g_vulkan->instance, &surface);
    ASSERT(result == VK_SUCCESS);


    /**************************************************************************
     * Create VkDevice and get its queue
     *************************************************************************/
    {
        u32 queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(
            g_vulkan->physical_device.handle,
            &queue_family_count,
            nullptr);

        auto queue_families = alloc_array(g_frame, VkQueueFamilyProperties, queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            g_vulkan->physical_device.handle,
            &queue_family_count,
            queue_families);

        for (u32 i = 0; i < queue_family_count; ++i) {
            VkQueueFamilyProperties &property = queue_families[i];

            VkBool32 supports_present = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                g_vulkan->physical_device.handle,
                i,
                surface,
                &supports_present);
            ASSERT(result == VK_SUCCESS);

            LOG("VkQueueFamilyProperties[%u]", i);
            LOG("  queueCount : %u", property.queueCount);
            LOG("  timestampValidBits : %u", property.timestampValidBits);
            LOG("  minImageTransferGranualrity : (%u, %u, %u)",
                property.minImageTransferGranularity.depth,
                property.minImageTransferGranularity.height,
                property.minImageTransferGranularity.depth);
            LOG("  flags : %u", property.queueFlags);
            LOG("  supportsPresent : %s", supports_present ? "yes" : "no");
        }

        GfxQueue present_queue = {};
        GfxQueue transfer_queue = {};

        for (u32 i = 0; i < queue_family_count; ++i) {
            VkQueueFamilyProperties &property = queue_families[i];

            VkBool32 supports_present = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                g_vulkan->physical_device.handle,
                i,
                surface,
                &supports_present);
            ASSERT(result == VK_SUCCESS);

            if (present_queue.flags == 0 && supports_present) {
                if (property.queueFlags == VK_QUEUE_GRAPHICS_BIT)
                {
                    present_queue.family_index = i;
                    present_queue.flags = property.queueFlags;
                }
            }

            if (transfer_queue.flags == 0) {
                if (property.queueFlags == VK_QUEUE_TRANSFER_BIT)
                {
                    transfer_queue.family_index = i;
                    transfer_queue.flags = property.queueFlags;
                }
            }
        }

        for (u32 i = 0; i < queue_family_count; ++i) {
            VkQueueFamilyProperties &property = queue_families[i];

            VkBool32 supports_present = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                g_vulkan->physical_device.handle,
                i,
                surface,
                &supports_present);
            ASSERT(result == VK_SUCCESS);

            if (present_queue.flags == 0 && supports_present) {
                if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                    (property.queueFlags & VK_QUEUE_TRANSFER_BIT) == 0)
                {
                    present_queue.family_index = i;
                    present_queue.flags = property.queueFlags;
                }
            }

            if (transfer_queue.flags == 0) {
                if (property.queueFlags & VK_QUEUE_TRANSFER_BIT &&
                    (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                {
                    transfer_queue.family_index = i;
                    transfer_queue.flags = property.queueFlags;
                }
            }
        }

        for (u32 i = 0; i < queue_family_count; ++i) {
            VkQueueFamilyProperties &property = queue_families[i];

            VkBool32 supports_present = VK_FALSE;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(
                g_vulkan->physical_device.handle,
                i,
                surface,
                &supports_present);
            ASSERT(result == VK_SUCCESS);

            if (present_queue.flags == 0 && supports_present) {
                if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    present_queue.family_index = i;
                    present_queue.flags = property.queueFlags;
                }
            }

            if (transfer_queue.flags == 0) {
                if (property.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                    transfer_queue.family_index = i;
                    transfer_queue.flags = property.queueFlags;
                }
            }
        }

        {
            g_vulkan->queues[GFX_QUEUE_GRAPHICS] = present_queue;
            g_vulkan->queues[GFX_QUEUE_TRANSFER] = transfer_queue;

            LOG("Vulkan Present Queue Family chosen: %u", present_queue.family_index);
            LOG("Vulkan Transfer Queue Family chosen: %u", transfer_queue.family_index);
        }

        // TODO: when we have more than one queue we'll need to figure out how
        // to handle this, for now just set highest queue priroity for the 1
        // queue we create
        f32 priority = 1.0f;

        VkDeviceQueueCreateInfo queue_infos[2] = {};
        queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_infos[0].queueFamilyIndex = g_vulkan->queues[GFX_QUEUE_GRAPHICS].family_index;
        queue_infos[0].queueCount       = 1;
        queue_infos[0].pQueuePriorities = &priority;

        queue_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_infos[1].queueFamilyIndex = g_vulkan->queues[GFX_QUEUE_TRANSFER].family_index;
        queue_infos[1].queueCount       = 1;
        queue_infos[1].pQueuePriorities = &priority;

        // TODO: look into other extensions
        const char *device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        VkDeviceCreateInfo device_info = {};
        device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_info.queueCreateInfoCount    = ARRAY_SIZE(queue_infos);
        device_info.pQueueCreateInfos       = queue_infos;
        device_info.enabledExtensionCount   = 1;
        device_info.ppEnabledExtensionNames = device_extensions;
        device_info.pEnabledFeatures        = &g_vulkan->physical_device.features;

        result = vkCreateDevice(
            g_vulkan->physical_device.handle,
            &device_info,
            nullptr,
            &g_vulkan->handle);
        ASSERT(result == VK_SUCCESS);

        // NOTE: does it matter which queue we choose?
        u32 queue_index = 0;
        vkGetDeviceQueue(
            g_vulkan->handle,
            g_vulkan->queues[GFX_QUEUE_GRAPHICS].family_index,
            queue_index,
            &g_vulkan->queues[GFX_QUEUE_GRAPHICS].vk_queue);

        if (present_queue.family_index == transfer_queue.family_index) {
            queue_index++;
        }

        vkGetDeviceQueue(
            g_vulkan->handle,
            g_vulkan->queues[GFX_QUEUE_TRANSFER].family_index,
            queue_index,
            &g_vulkan->queues[GFX_QUEUE_TRANSFER].vk_queue);
    }

    /**************************************************************************
     * Create VkCommandPool
     *************************************************************************/
    {
        gfx_create_command_pools(&g_vulkan->queues[GFX_QUEUE_GRAPHICS]);
        gfx_create_command_pools(&g_vulkan->queues[GFX_QUEUE_TRANSFER]);

        init_array(&g_vulkan->semaphores_submit_wait, g_heap);
        init_array(&g_vulkan->semaphores_submit_wait_stages, g_heap);
        init_array(&g_vulkan->semaphores_submit_signal, g_heap);
        init_array(&g_vulkan->present_semaphores, g_heap);
    }

    /**************************************************************************
     * Create VkSwapchainKHR
     *************************************************************************/
    // NOTE(jesper): the swapchain is stuck in the VulkanDevice creation right
    // now because we're still hardcoding the depth buffer creation, among other
    // things, in the VulkanDevice, which requires the created swapchain.
    // Really I think it mostly/only need the extent, but same difference
    g_vulkan->swapchain = swapchain_create(&g_vulkan->physical_device, surface);

    /**************************************************************************
     * Create vkRenderPass
     *************************************************************************/
    {
        auto descs = create_array<VkAttachmentDescription>(g_stack);
        array_add(&descs, {
             0,
             g_vulkan->swapchain.format,
             VK_SAMPLE_COUNT_1_BIT,
             VK_ATTACHMENT_LOAD_OP_CLEAR,
             VK_ATTACHMENT_STORE_OP_STORE,
             VK_ATTACHMENT_LOAD_OP_DONT_CARE,
             VK_ATTACHMENT_STORE_OP_DONT_CARE,
             VK_IMAGE_LAYOUT_UNDEFINED,
             VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        });

        array_add(&descs, {
            0,
            g_vulkan->swapchain.depth.format,
            VK_SAMPLE_COUNT_1_BIT,
            VK_ATTACHMENT_LOAD_OP_CLEAR,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            VK_ATTACHMENT_STORE_OP_DONT_CARE,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        });

        auto color = create_array<VkAttachmentReference>(g_stack);
        array_add(&color, { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

        VkAttachmentReference depth = {
            1,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkSubpassDescription subpass_description = {};
        subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount    = (u32)color.count;
        subpass_description.pColorAttachments       = color.data;
        subpass_description.pDepthStencilAttachment = &depth;

        VkRenderPassCreateInfo create_info = {};
        create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        create_info.attachmentCount = (u32)descs.count;
        create_info.pAttachments    = descs.data;
        create_info.subpassCount    = 1;
        create_info.pSubpasses      = &subpass_description;
        create_info.dependencyCount = 0;
        create_info.pDependencies   = nullptr;

        result = vkCreateRenderPass(g_vulkan->handle,
                                    &create_info,
                                    nullptr,
                                    &g_vulkan->renderpass);
        ASSERT(result == VK_SUCCESS);
    }

    /**************************************************************************
     * Create Framebuffers
     *************************************************************************/
    {
        auto buffer = alloc_array(g_persistent, VkFramebuffer, g_vulkan->swapchain.images_count);
        g_vulkan->framebuffers = (VkFramebuffer*)buffer;

        VkFramebufferCreateInfo create_info = {};
        create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass      = g_vulkan->renderpass;
        create_info.width           = g_vulkan->swapchain.extent.width;
        create_info.height          = g_vulkan->swapchain.extent.height;
        create_info.layers          = 1;

        auto attachments = alloc_array(g_stack, VkImageView, 2);
        attachments[1] = g_vulkan->swapchain.depth.imageview;

        for (i32 i = 0; i < (i32)g_vulkan->swapchain.images_count; ++i)
        {
            attachments[0] = g_vulkan->swapchain.imageviews[i];

            create_info.attachmentCount = 2;
            create_info.pAttachments    = attachments;

            VkFramebuffer framebuffer;

            result = vkCreateFramebuffer(
                g_vulkan->handle,
                &create_info,
                nullptr,
                &framebuffer);
            ASSERT(result == VK_SUCCESS);

            g_vulkan->framebuffers[g_vulkan->framebuffers_count++] = framebuffer;
        }
    }

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool        = g_vulkan->queues[GFX_QUEUE_GRAPHICS].command_pool;
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = GFX_NUM_FRAMES;

    VkCommandBuffer command_buffers[GFX_NUM_FRAMES];
    result = vkAllocateCommandBuffers(g_vulkan->handle, &alloc_info, &command_buffers[0]);
    ASSERT(result == VK_SUCCESS);

    for (i32 i = 0; i < GFX_NUM_FRAMES; i++)
    {
        GfxFrame& frame = g_vulkan->frames[i];
        frame.cmd = command_buffers[i];

        result = vkCreateSemaphore(
            g_vulkan->handle,
            &semaphore_info,
            nullptr,
            &frame.available);
        ASSERT(result == VK_SUCCESS);

        result = vkCreateSemaphore(
            g_vulkan->handle,
            &semaphore_info,
            nullptr,
            &frame.complete);
        ASSERT(result == VK_SUCCESS);

        result = vkCreateFence(g_vulkan->handle, &fence_info, nullptr, &frame.fence);
        ASSERT(result == VK_SUCCESS);

        VkQueryPoolCreateInfo query_info = {};
        query_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        query_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        query_info.queryCount = GFX_NUM_TIMESTAMP_QUERIES;

        result = vkCreateQueryPool(
            g_vulkan->handle,
            &query_info,
            nullptr,
            &frame.timestamps);
        ASSERT(result == VK_SUCCESS);
    }
}

void destroy_pipeline(VulkanPipeline pipeline)
{
    // TODO(jesper): find a better way to clean these up; not every pipeline
    // will have every shader stage and we'll probably want to keep shader
    // stages in a map of some sort of in the device to reuse
    vkDestroyShaderModule(
        g_vulkan->handle,
        pipeline.shaders[ShaderStage_vertex].module,
        nullptr);

    vkDestroyShaderModule(
        g_vulkan->handle,
        pipeline.shaders[ShaderStage_fragment].module,
        nullptr);

    for (i32 i = 0; i < pipeline.sampler_count; i++) {
        vkDestroySampler(g_vulkan->handle, pipeline.samplers[i], nullptr);
    }

    vkDestroyDescriptorSetLayout(g_vulkan->handle, pipeline.set_layouts[0], nullptr);
    vkDestroyDescriptorSetLayout(g_vulkan->handle, pipeline.set_layouts[1], nullptr);
    vkDestroyPipelineLayout(g_vulkan->handle, pipeline.layout, nullptr);
    vkDestroyPipeline(g_vulkan->handle, pipeline.handle, nullptr);
}

void destroy_swapchain(VulkanSwapchain swapchain)
{
    for (i32 i = 0; i < (i32)g_vulkan->swapchain.images_count; i++) {
        vkDestroyImageView(g_vulkan->handle, swapchain.imageviews[i], nullptr);
    }

    vkDestroyImageView(g_vulkan->handle, swapchain.depth.imageview, nullptr);
    vkDestroyImage(g_vulkan->handle, swapchain.depth.image, nullptr);
    vkFreeMemory(g_vulkan->handle, swapchain.depth.memory, nullptr);

    vkDestroySwapchainKHR(g_vulkan->handle, swapchain.handle, nullptr);
    vkDestroySurfaceKHR(g_vulkan->instance, swapchain.surface, nullptr);
}

void destroy_texture(Texture *texture)
{
    vkDestroyImageView(g_vulkan->handle, texture->image_view, nullptr);
    vkDestroyImage(g_vulkan->handle, texture->image, nullptr);
    vkFreeMemory(g_vulkan->handle, texture->memory, nullptr);
}

void destroy_buffer(VulkanBuffer buffer)
{
    for (i32 i = 0; i < g_buffers.count; i++) {
        if (g_buffers[i].handle == buffer.handle) {
            array_remove(&g_buffers, i);
            break;
        }
    }

    vkFreeMemory(g_vulkan->handle, buffer.memory, nullptr);
    vkDestroyBuffer(g_vulkan->handle, buffer.handle, nullptr);
}

void destroy_buffer_ubo(VulkanUniformBuffer ubo)
{
    destroy_buffer(ubo.staging);
    destroy_buffer(ubo.buffer);
}

void gfx_destroy_descriptors(Array<GfxDescriptorPool> *pools)
{
    for (i32 i = 0; i < pools->count; i++)
    {
        vkDestroyDescriptorPool(g_vulkan->handle, (*pools)[i].vk_pool, nullptr);
    }
}

void destroy_vulkan()
{
    for (i32 i = 0; i < g_buffers.count; i++) {
        vkFreeMemory(g_vulkan->handle, g_buffers[i].memory, nullptr);
        vkDestroyBuffer(g_vulkan->handle, g_buffers[i].handle, nullptr);
    }
    g_buffers.count = 0;

    gfx_destroy_descriptors(&g_vulkan->descriptor_pools[0]);
    gfx_destroy_descriptors(&g_vulkan->descriptor_pools[1]);

    for (i32 i = 0; i < g_vulkan->framebuffers_count; ++i) {
        vkDestroyFramebuffer(g_vulkan->handle, g_vulkan->framebuffers[i], nullptr);
    }

    for (i32 i = 0; i < (i32)Pipeline_count; i++) {
        destroy_pipeline(g_vulkan->pipelines[i]);
    }

    vkDestroyRenderPass(g_vulkan->handle, g_vulkan->renderpass, nullptr);

    for (i32 i = 0; i < ARRAY_SIZE(g_vulkan->queues); i++) {
        vkDestroyCommandPool(g_vulkan->handle, g_vulkan->queues[i].command_pool, nullptr);
    }

    for (i32 i = 0; i < GFX_NUM_FRAMES; i++) {
        GfxFrame frame = g_vulkan->frames[i];

        vkDestroyFence(g_vulkan->handle, frame.fence, nullptr);
        vkDestroySemaphore(g_vulkan->handle, frame.available, nullptr);
        vkDestroySemaphore(g_vulkan->handle, frame.complete, nullptr);
        vkDestroyQueryPool(g_vulkan->handle, frame.timestamps, nullptr);
    }


    // TODO(jesper): move out of here when the swapchain<->device dependency is
    // fixed
    destroy_swapchain(g_vulkan->swapchain);


    vkDestroyDevice(g_vulkan->handle,     nullptr);

    vkdebug_destroy();

    vkDestroyInstance(g_vulkan->instance, nullptr);
}

VulkanBuffer create_buffer(
    usize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memory_flags)
{
    VulkanBuffer buffer = {};
    buffer.size  = size;
    buffer.usage = usage;

    VkBufferCreateInfo create_info = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size                  = size;
    create_info.usage                 = usage;
    create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;

    VkResult result = vkCreateBuffer(g_vulkan->handle,
                                     &create_info,
                                     nullptr,
                                     &buffer.handle);
    ASSERT(result == VK_SUCCESS);

    // TODO: alloc buffers from large memory pool in VulkanDevice
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(g_vulkan->handle, buffer.handle, &memory_requirements);

    u32 index = find_memory_type(
        g_vulkan->physical_device,
        memory_requirements.memoryTypeBits,
        memory_flags);
    ASSERT(index != UINT32_MAX);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = memory_requirements.size;
    alloc_info.memoryTypeIndex = index;

    result = vkAllocateMemory(
        g_vulkan->handle,
        &alloc_info,
        nullptr,
        &buffer.memory);
    ASSERT(result == VK_SUCCESS);

    result = vkBindBufferMemory(
        g_vulkan->handle,
        buffer.handle,
        buffer.memory,
        0);
    ASSERT(result == VK_SUCCESS);

    array_add(&g_buffers, buffer);


    return buffer;
}

void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    GfxCommandBuffer command = gfx_begin_command(GFX_QUEUE_GRAPHICS);

    VkBufferCopy region = {};
    region.srcOffset    = 0;
    region.dstOffset    = 0;
    region.size         = size;

    vkCmdCopyBuffer(command.handle, src, dst, 1, &region);

    // TODO(jesper): move flush/wait out of here
    gfx_end_command(command);
    gfx_flush_and_wait(GFX_QUEUE_GRAPHICS);
}

VulkanBuffer create_vbo(void *data, usize size)
{
    if (size == 0) {
        return {};
    }

    VulkanBuffer vbo = create_buffer(
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    void *mapped;
    VkResult result = vkMapMemory(g_vulkan->handle, vbo.memory, 0, VK_WHOLE_SIZE, 0, &mapped);
    ASSERT(result == VK_SUCCESS);

    memcpy(mapped, data, size);
    vkUnmapMemory(g_vulkan->handle, vbo.memory);

    return vbo;
}

void gfx_update_buffer(VulkanBuffer *dst, void *data, usize size)
{
    VulkanBuffer staging = create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *mapped;
    vkMapMemory(g_vulkan->handle, staging.memory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(g_vulkan->handle, staging.memory);

    if (dst->size >= size) {
        // TODO(jesper): if dst->size > size we're wasting a bunch of memory
        // here. might not happen enough that we actually care?
        buffer_copy(staging.handle, dst->handle, size);
        dst->size = size;
    } else {
        VulkanBuffer resized = create_buffer(
            size,
            dst->usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        buffer_copy(staging.handle, resized.handle, size);
        *dst = resized;
    }

    destroy_buffer(staging);
}

VulkanBuffer create_vbo(usize size)
{
    VulkanBuffer vbo = create_buffer(
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    return vbo;
}

VulkanBuffer create_ibo(u32 *indices, usize size)
{
    VulkanBuffer staging = create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(g_vulkan->handle, staging.memory, 0, size, 0, &data);
    memcpy(data, indices, size);
    vkUnmapMemory(g_vulkan->handle, staging.memory);

    VulkanBuffer ib = create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    buffer_copy(staging.handle, ib.handle, size);

    destroy_buffer(staging);

    return ib;
}

VulkanUniformBuffer create_ubo(usize size)
{
    VulkanUniformBuffer ubo;
    ubo.staging = create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ubo.buffer = create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkResult result = vkMapMemory(
        g_vulkan->handle,
        ubo.staging.memory,
        0, VK_WHOLE_SIZE,
        0,
        &ubo.mapped);
    ASSERT(result == VK_SUCCESS);

    return ubo;
}


void buffer_data(VulkanUniformBuffer ubo, void *data, usize offset, usize size)
{
    PROFILE_FUNCTION();
    memcpy((u8*)ubo.mapped + offset, data, size);
    buffer_copy(ubo.staging.handle, ubo.buffer.handle, size);
}

Material create_material(PipelineID pipeline_id, MaterialID id)
{
    Material mat = {};
    mat.id       = id;
    mat.pipeline = pipeline_id;

    VulkanPipeline &pipeline = g_vulkan->pipelines[pipeline_id];

    mat.descriptor_set = gfx_create_descriptor(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        pipeline.set_layouts[1]);
    ASSERT(mat.descriptor_set.id != -1);
    return mat;
}

void gfx_set_texture(
    PipelineID pipeline,
    GfxDescriptorSet descriptor,
    i32 binding,
    GfxTexture texture)
{
    VkDescriptorImageInfo image_info = {};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView   = texture.vk_view;
    // TODO(jesper): 1:1 mapping between binding and sampler won't be correct
    // for long
    image_info.sampler     = g_vulkan->pipelines[pipeline].samplers[binding];

    VkWriteDescriptorSet writes = {};
    writes.sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes.dstSet     = descriptor.vk_set;
    writes.dstBinding = binding;

    writes.dstArrayElement = 0;
    writes.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes.descriptorCount = 1;
    writes.pImageInfo      = &image_info;

    vkUpdateDescriptorSets(g_vulkan->handle, 1, &writes, 0, nullptr);
}

void gfx_set_ubo(
    PipelineID pipeline_id,
    i32 binding,
    VulkanUniformBuffer *ubo)
{
    VulkanPipeline &pipeline = g_vulkan->pipelines[pipeline_id];

    VkDescriptorBufferInfo buffer_info = {};
    buffer_info.buffer = ubo->buffer.handle;
    // TODO(jesper): support ubo offsets
    buffer_info.offset = 0;
    buffer_info.range  = ubo->buffer.size;

    VkWriteDescriptorSet writes = {};
    writes.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes.dstSet          = pipeline.descriptor_set.vk_set;
    writes.dstBinding      = binding;
    writes.dstArrayElement = 0;
    writes.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes.descriptorCount = 1;
    writes.pBufferInfo     = &buffer_info;

    vkUpdateDescriptorSets(g_vulkan->handle, 1, &writes, 0, nullptr);
}

PushConstants create_push_constants(PipelineID pipeline)
{
    PushConstants c = {};

    switch (pipeline) {
    case Pipeline_font:
        c.offset = 0;
        c.size   = sizeof(Matrix4);
        c.data   = alloc(g_heap, c.size);
        break;
    case Pipeline_wireframe:
    case Pipeline_wireframe_lines:
        c.offset = 0;
        c.size   = sizeof(Matrix4) + sizeof(Vector3);
        c.data   = alloc(g_heap, c.size);
        break;
    case Pipeline_gui_basic:
        c.offset = 0;
        c.size   = sizeof(Matrix4);
        c.data   = alloc(g_heap, c.size);
        break;
    case Pipeline_basic2d:
        c.offset = 0;
        c.size   = sizeof(Matrix4);
        c.data   = alloc(g_heap, c.size);
        break;
    default:
        // TODO(jesper): error handling
        ASSERT(false);
        break;
    }

    return c;
}

template<typename T>
void set_push_constant(PushConstants *c, T t)
{
    // TODO(jesper): handle multiple push constants
    ASSERT(c->size == sizeof(T));
    memcpy(c->data, &t, sizeof(T));
}

GfxDescriptorSet gfx_create_descriptor(
    VkDescriptorType type,
    VkDescriptorSetLayout layout)
{
    Array<GfxDescriptorPool> *pools = nullptr;

    switch (type) {
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        pools = &g_vulkan->descriptor_pools[0];
        break;
    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        pools = &g_vulkan->descriptor_pools[1];
        break;
    case VK_DESCRIPTOR_TYPE_SAMPLER:
    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
    case VK_DESCRIPTOR_TYPE_RANGE_SIZE:
    case VK_DESCRIPTOR_TYPE_MAX_ENUM:
        LOG_ERROR("unimplemented descriptor type");
        return {};
        break;
    }

    GfxDescriptorSet desc = {};
    for (i32 i = 0; i < pools->count; i++) {
        GfxDescriptorPool &pool = (*pools)[i];
        if (pool.count < pool.capacity) {

            VkDescriptorSetAllocateInfo dai = {};
            dai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dai.descriptorSetCount = 1;
            dai.descriptorPool = pool.vk_pool;
            dai.pSetLayouts = &layout;

            VkResult result = vkAllocateDescriptorSets(g_vulkan->handle, &dai, &desc.vk_set);
            ASSERT(result == VK_SUCCESS);

            pool.sets[pool.count] = desc.vk_set;

            desc.id      = pool.count++;
            desc.pool_id = i;
        }
    }

    if (desc.id == -1) {
        // TODO(jesper): investigate a better pool size or potentially
        // automatically resizing existing pools in some way
        GfxDescriptorPool pool = {};
        pool.count    = 0;
        pool.capacity = 10;
        pool.sets = alloc_array(g_persistent, VkDescriptorSet, pool.capacity);

        VkDescriptorPoolSize dps = {};
        dps.type            = type;
        dps.descriptorCount = pool.capacity;

        VkDescriptorPoolCreateInfo pi = {};
        pi.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pi.poolSizeCount = 1;
        pi.pPoolSizes    = &dps;
        pi.maxSets       = pool.capacity;

        VkResult result = vkCreateDescriptorPool( g_vulkan->handle, &pi, nullptr, &pool.vk_pool);
        ASSERT(result == VK_SUCCESS);

        VkDescriptorSetAllocateInfo dai = {};
        dai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dai.descriptorSetCount = 1;
        dai.descriptorPool = pool.vk_pool;
        dai.pSetLayouts = &layout;

        result = vkAllocateDescriptorSets(g_vulkan->handle, &dai, &desc.vk_set);
        ASSERT(result == VK_SUCCESS);

        pool.sets[pool.count] = desc.vk_set;

        desc.id = pool.count++;
        desc.pool_id = array_add(pools, pool);
    }

    return desc;
}

void gfx_bind_descriptors(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    Array<GfxDescriptorSet> descriptors)
{
    auto vk_sets = create_array<VkDescriptorSet>(g_stack, descriptors.count);
    for (auto set : descriptors) {
        array_add(&vk_sets, set.vk_set);
    }

    vkCmdBindDescriptorSets(
        cmd,
        bind_point, layout,
        0, vk_sets.count, vk_sets.data,
        0, nullptr);
}

void gfx_bind_descriptor(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    GfxDescriptorSet descriptor)
{
    vkCmdBindDescriptorSets(
        cmd,
        bind_point, layout,
        0, 1, &descriptor.vk_set,
        0, nullptr);
}

GfxFrame gfx_begin_frame()
{
    VkResult result;
    GfxFrame& frame = g_vulkan->frames[g_vulkan->current_frame];

    if (frame.submitted) {
        PROFILE_SCOPE(vk_wait_frame);

        result = vkWaitForFences(g_vulkan->handle, 1, &frame.fence, VK_TRUE, UINT64_MAX);
        ASSERT(result == VK_SUCCESS);

        result = vkResetFences(g_vulkan->handle, 1, &frame.fence);
        ASSERT(result == VK_SUCCESS);

        frame.submitted = false;
    }

    if (frame.current_timestamp > 0) {
        u64 timestamps[GFX_NUM_TIMESTAMP_QUERIES];
        result = vkGetQueryPoolResults(
            g_vulkan->handle,
            frame.timestamps,
            0, frame.current_timestamp,
            sizeof timestamps,
            &timestamps[0],
            sizeof timestamps[0],
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        ASSERT(result == VK_SUCCESS);

        u64 start = timestamps[0];
        u64 end   = timestamps[1];

        f32 tick = (1000 * 1000 * 1000) / g_vulkan->physical_device.properties.limits.timestampPeriod;
        g_vulkan->gpu_time = ((end - start) * 1000) / tick;

        frame.current_timestamp = 0;
    }

    {
        PROFILE_SCOPE(vk_acquire_swapchain);

        result = vkAcquireNextImageKHR(
            g_vulkan->handle,
            g_vulkan->swapchain.handle,
            UINT64_MAX,
            frame.available,
            VK_NULL_HANDLE,
            &frame.swapchain_index);
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(frame.cmd, &begin_info);
    ASSERT(result == VK_SUCCESS);

    vkCmdWriteTimestamp(
        frame.cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        frame.timestamps,
        frame.current_timestamp++);

    VkClearValue clear_values[2];
    clear_values[0].color        = {{ 1.0f, 0.0f, 0.0f, 0.0f }};
    clear_values[1].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass        = g_vulkan->renderpass;
    info.framebuffer       = g_vulkan->framebuffers[frame.swapchain_index];
    info.renderArea.offset = { 0, 0 };
    info.renderArea.extent = g_vulkan->swapchain.extent;
    info.clearValueCount   = 2;
    info.pClearValues      = clear_values;

    vkCmdBeginRenderPass(frame.cmd, &info, VK_SUBPASS_CONTENTS_INLINE);

    return frame;
}

void gfx_end_frame()
{
    VkResult result;
    GfxFrame& frame = g_vulkan->frames[g_vulkan->current_frame];

    vkCmdEndRenderPass(frame.cmd);

    vkCmdWriteTimestamp(
        frame.cmd,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        frame.timestamps,
        frame.current_timestamp++);

    result = vkEndCommandBuffer(frame.cmd);
    ASSERT(result == VK_SUCCESS);

    submit_semaphore_wait(
        frame.available,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    submit_semaphore_signal(frame.complete);

    VkSubmitInfo sinfo = {};
    sinfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    sinfo.commandBufferCount   = 1;
    sinfo.pCommandBuffers      = &frame.cmd;

    sinfo.waitSemaphoreCount   = (u32)g_vulkan->semaphores_submit_wait.count;
    sinfo.pWaitSemaphores      = g_vulkan->semaphores_submit_wait.data;
    sinfo.pWaitDstStageMask    = g_vulkan->semaphores_submit_wait_stages.data;
    sinfo.signalSemaphoreCount = (u32)g_vulkan->semaphores_submit_signal.count;
    sinfo.pSignalSemaphores    = g_vulkan->semaphores_submit_signal.data;

    vkQueueSubmit(g_vulkan->queues[GFX_QUEUE_GRAPHICS].vk_queue, 1, &sinfo, frame.fence);

    present_semaphore(frame.complete);

    VkPresentInfoKHR pinfo = {};
    pinfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pinfo.waitSemaphoreCount = (u32)g_vulkan->present_semaphores.count;
    pinfo.pWaitSemaphores    = g_vulkan->present_semaphores.data;
    pinfo.swapchainCount     = 1;
    pinfo.pSwapchains        = &g_vulkan->swapchain.handle;
    pinfo.pImageIndices      = &frame.swapchain_index;

    result = vkQueuePresentKHR(g_vulkan->queues[GFX_QUEUE_GRAPHICS].vk_queue, &pinfo);
    ASSERT(result == VK_SUCCESS);

    g_vulkan->current_frame = (g_vulkan->current_frame + 1) % GFX_NUM_FRAMES;
    frame.submitted = true;

    g_vulkan->present_semaphores.count       = 0;
    g_vulkan->semaphores_submit_wait.count   = 0;
    g_vulkan->semaphores_submit_signal.count = 0;
}

void gfx_create_image(
    i32 width,
    i32 height,
    u32 mip_levels,
    VkFormat format,
    VkImageTiling tiling,
    VkImage *vk_image,
    VkDeviceMemory *vk_memory,
    u32 usage, // VkImageUsageFlagBits
    VkMemoryPropertyFlags properties)
{
    VkResult result;

    VkImageCreateInfo info = {};
    info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType     = VK_IMAGE_TYPE_2D;
    info.format        = format;
    info.extent.width  = width;
    info.extent.height = height;
    info.extent.depth  = 1;
    info.mipLevels     = mip_levels;
    info.arrayLayers   = 1;
    info.samples       = VK_SAMPLE_COUNT_1_BIT;
    // TODO(jesper): look into tiling options
    info.tiling        = tiling;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.usage         = usage;
    info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateImage(g_vulkan->handle, &info, nullptr, vk_image);
    ASSERT(result == VK_SUCCESS);

    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(g_vulkan->handle, *vk_image, &req);

    u32 memory_type = find_memory_type(
        g_vulkan->physical_device,
        req.memoryTypeBits,
        properties);
    ASSERT(memory_type != UINT32_MAX);

    VkMemoryAllocateInfo ainfo = {};
    ainfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ainfo.allocationSize  = req.size;
    ainfo.memoryTypeIndex = memory_type;

    // TODO(jesper): vulkan pool allocator
    result = vkAllocateMemory(g_vulkan->handle, &ainfo, nullptr, vk_memory);
    ASSERT(result == VK_SUCCESS);

    vkBindImageMemory(g_vulkan->handle, *vk_image, *vk_memory, 0);
}


GfxTexture gfx_create_texture(
    i32 width,
    i32 height,
    u32 mip_levels,
    VkFormat format,
    VkImageTiling tiling,
    VkComponentMapping components,
    u32 usage, // VkImageUsageFlagBits
    VkMemoryPropertyFlags properties)
{
    VkResult result;

    GfxTexture texture = {};
    texture.width      = width;
    texture.height     = height;
    texture.vk_format  = format;

    gfx_create_image(
        width,
        height,
        mip_levels,
        format,
        tiling,
        &texture.vk_image,
        &texture.vk_memory,
        usage,
        properties);

    VkImageViewCreateInfo vinfo = {};
    vinfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vinfo.image      = texture.vk_image;
    vinfo.viewType   = VK_IMAGE_VIEW_TYPE_2D;
    vinfo.format     = texture.vk_format;
    vinfo.components = components;
    vinfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    vinfo.subresourceRange.baseMipLevel   = 0;
    vinfo.subresourceRange.levelCount     = mip_levels;
    vinfo.subresourceRange.baseArrayLayer = 0;
    vinfo.subresourceRange.layerCount     = 1;

    result = vkCreateImageView(g_vulkan->handle, &vinfo, nullptr, &texture.vk_view);
    ASSERT(result == VK_SUCCESS);

    return texture;
}

GfxTexture gfx_create_texture(
    u32 width,
    u32 height,
    u32 mip_levels,
    VkFormat format,
    VkComponentMapping components,
    void *pixels)
{
    i32 num_channels;
    i32 bytes_per_channel;

    switch (format) {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_UINT:
        num_channels      = 1;
        bytes_per_channel = 1;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_B8G8R8A8_SRGB:
        num_channels      = 4;
        bytes_per_channel = 1;
        break;
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SNORM:
    case VK_FORMAT_R16_SFLOAT:
    case VK_FORMAT_R16_SINT:
        num_channels      = 1;
        bytes_per_channel = 2;
        break;
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
        num_channels      = 1;
        bytes_per_channel = 4;
        break;
    case VK_FORMAT_R32G32_SFLOAT:
        num_channels      = 2;
        bytes_per_channel = 4;
        break;
    case VK_FORMAT_R16G16_SFLOAT:
        num_channels      = 2;
        bytes_per_channel = 2;
        break;
    default:
        ASSERT(false);
        num_channels      = 1;
        bytes_per_channel = 2;
        break;
    }

    VkDeviceSize size = width * height * num_channels * bytes_per_channel;

    VulkanBuffer staging = create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *mapped = nullptr;
    vkMapMemory(g_vulkan->handle, staging.memory, 0, size, 0, &mapped);
    ASSERT(mapped != nullptr);

    memcpy(mapped, pixels, size);

    vkUnmapMemory(g_vulkan->handle, staging.memory);

    GfxTexture texture = gfx_create_texture(
        width,
        height,
        mip_levels,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        components,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

    gfx_transition_immediate(
        &texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    GfxCommandBuffer command = gfx_begin_command(GFX_QUEUE_TRANSFER);

    VkBufferImageCopy region = {};
    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset                     = { 0, 0, 0 };
    region.imageExtent                     = { width, height, 1 };

    vkCmdCopyBufferToImage(
        command.handle,
        staging.handle,
        texture.vk_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    gfx_end_command(command);
    gfx_flush_and_wait(GFX_QUEUE_TRANSFER);

    if (mip_levels > 1) {
        VkImageMemoryBarrier barrier = {};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = texture.vk_image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        command = gfx_begin_command(GFX_QUEUE_GRAPHICS);

        i32 mip_width  = width;
        i32 mip_height = height;

        for (u32 i = 1; i < mip_levels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                command.handle,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit = {};
            blit.srcOffsets[0]                 = { 0, 0, 0 };
            blit.srcOffsets[1]                 = { mip_width, mip_height, 1 };
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = 1;
            blit.dstOffsets[0]                 = { 0, 0, 0 };

            blit.dstOffsets[1] = {
                mip_width > 1 ? mip_width / 2 : 1,
                mip_height> 1 ? mip_height / 2 : 1,
                1
            };

            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = 1;

            // TODO(jesper) look into filter options here
            vkCmdBlitImage(
                command.handle,
                texture.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                texture.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                command.handle,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mip_width > 1) {
                mip_width /= 2;
            }

            if (mip_height > 1) {
                mip_height /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            command.handle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        gfx_end_command(command);
        gfx_flush_and_wait(GFX_QUEUE_GRAPHICS);
    } else {
        gfx_transition_immediate(
            &texture,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    destroy_buffer(staging);

    return texture;
}

void gfx_destroy_texture(GfxTexture texture)
{
    vkFreeMemory(g_vulkan->handle, texture.vk_memory, nullptr);
    vkDestroyImage(g_vulkan->handle, texture.vk_image, nullptr);
}

Vector2 screen_from_camera(Vector2 v)
{
    Vector2 r;
    r.x = g_vulkan->resolution.x / 2.0f * ( v.x + 1.0f ) + g_vulkan->offset.x;
    r.y = g_vulkan->resolution.y / 2.0f * ( v.y + 1.0f ) + g_vulkan->offset.y;
    return r;
}

Vector3 screen_from_camera(Vector3 v)
{
    Vector3 r;
    r.x = g_vulkan->resolution.x / 2.0f * ( v.x + 1.0f ) + g_vulkan->offset.x;
    r.y = g_vulkan->resolution.y / 2.0f * ( v.y + 1.0f ) + g_vulkan->offset.y;
    r.z = v.z;
    return r;
}

Vector2 camera_from_screen(Vector2 v)
{
    Vector2 r;
    r.x = 2.0f * v.x / g_vulkan->resolution.x - 1.0f;
    r.y = 2.0f * v.y / g_vulkan->resolution.y - 1.0f;
    return r;
}

Vector3 camera_from_screen(Vector3 v)
{
    Vector3 r;
    r.x = 2.0f * v.x / g_vulkan->resolution.x - 1.0f;
    r.y = 2.0f * v.y / g_vulkan->resolution.y - 1.0f;
    r.z = v.z;
    return r;
}

void gfx_copy_texture(
    GfxTexture *dst,
    GfxTexture *src)
{
    ASSERT(dst->mip_levels == src->mip_levels);
    ASSERT(dst->width == src->width);
    ASSERT(dst->height == src->height);

    VkPipelineStageFlagBits src_stage  = src->vk_stage;
    VkImageLayout           src_layout = src->vk_layout;

    VkPipelineStageFlagBits dst_stage  = dst->vk_stage;
    VkImageLayout           dst_layout = dst->vk_layout;

    gfx_transition_immediate(
        src,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    gfx_transition_immediate(
        dst,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT);

    GfxCommandBuffer command = gfx_begin_command(GFX_QUEUE_GRAPHICS);

    // TODO(jesper): support mip layers
    VkImageSubresourceLayers subresource = {};
    subresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.baseArrayLayer = 0;
    subresource.layerCount     = 1;


    i32 mip_width  = dst->width;
    i32 mip_height = dst->height;

    for (u32 i = 0; i < dst->mip_levels; i++) {
        subresource.mipLevel = i;

        VkImageCopy region = {};
        region.srcOffset = { 0, 0, 0 };
        region.dstOffset = { 0, 0, 0 };
        region.srcSubresource = subresource;
        region.dstSubresource = subresource;
        region.extent.depth = 1;

        region.extent.width  = mip_width;
        region.extent.height = mip_height;

        vkCmdCopyImage(
            command.handle,
            src->vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dst->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);

        if (mip_width > 1) {
            mip_width /= 2;
        }

        if (mip_height > 1) {
            mip_height /= 2;
        }
    }

    gfx_end_command(command);
    gfx_flush_and_wait(GFX_QUEUE_GRAPHICS);

    gfx_transition_immediate(dst, dst_layout, dst_stage);
    gfx_transition_immediate(src, src_layout, src_stage);
}

void gfx_flush_and_wait()
{
    vkQueueWaitIdle(g_vulkan->queues[GFX_QUEUE_GRAPHICS].vk_queue);
}

