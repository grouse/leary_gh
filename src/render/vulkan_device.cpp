/**
 * @file:   vulkan_device.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <limits>
#include <fstream>
#include <array>

enum ShaderStage {
	ShaderStage_vertex,
	ShaderStage_fragment,
	ShaderStage_max
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

struct VulkanTexture {
	u32            width;
	u32            height;
	VkFormat       format;
	VkImage        image;
	VkImageView    image_view;
	VkDeviceMemory memory;
};

struct VulkanShader {
	VkShaderModule        module;
	VkShaderStageFlagBits stage;
	const char            *name;
};

struct VulkanPipeline {
	VkPipeline            handle;
	VkPipelineLayout      layout;
	VkDescriptorSet       descriptor_set;
	VkDescriptorPool      descriptor_pool;
	VkDescriptorSetLayout descriptor_layout;
	VulkanShader          shaders[ShaderStage_max];
	VkSampler             texture_sampler;
};

struct VulkanSwapchain {
	VkSurfaceKHR   surface;
	VkFormat       format;
	VkSwapchainKHR handle;
	VkExtent2D     extent;

	u32            images_count;
	VkImage        *images;
	VkImageView    *imageviews;

	VkSemaphore    available;
};

struct VulkanPhysicalDevice {
	VkPhysicalDevice                 handle;
	VkPhysicalDeviceProperties       properties;
	VkPhysicalDeviceMemoryProperties memory;
	VkPhysicalDeviceFeatures         features;
};

struct VulkanDevice {
	VkDevice                 handle;

	VkInstance               instance;
	VkDebugReportCallbackEXT debug_callback;

	VkQueue                  queue;
	u32                      queue_family_index;


	VulkanSwapchain          swapchain;
	VulkanPhysicalDevice     physical_device;

	VkSemaphore              render_completed;

	VkCommandPool            command_pool;
	VkCommandBuffer          cmd_present;

	VkRenderPass             renderpass;

	i32                      framebuffers_count;
	VkFramebuffer            *framebuffers;
};


PFN_vkCreateDebugReportCallbackEXT   CreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT  DestroyDebugReportCallbackEXT;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback_func(VkFlags flags,
                    VkDebugReportObjectTypeEXT object_type,
                    u64 object,
                    usize location,
                    i32 message_code,
                    const char* layer,
                    const char* message,
                    void *user_data)
{
	// NOTE: these might be useful?
	VAR_UNUSED(object);
	VAR_UNUSED(location);
	VAR_UNUSED(user_data);

	// TODO(jesper): multiple flags can be set at the same time, I don't really
	// have a way to express this in my current logging system so I let the log
	// type be decided by a severity precedence
	LogChannel channel;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
		channel = Log_error;
	} else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
		channel = Log_warning;
	} else if (flags & (VK_DEBUG_REPORT_DEBUG_BIT_EXT |
	                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
	{
		channel = Log_info;
	} else {
		// NOTE: this would only happen if they extend the report callback
		// flags
		channel = Log_info;
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
		break;
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

	DEBUG_LOG(channel, "[Vulkan:%s] [%s:%d] - %s",
	          layer, object_str, message_code, message);
	DEBUG_ASSERT(channel != Log_error);
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

u32 find_memory_type(VulkanPhysicalDevice physical_device, u32 filter,
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

VkCommandBuffer begin_command_buffer(VulkanDevice *device)
{
	// TODO(jesper): don't allocate command buffers on demand; allocate a big
	// pool of them in the device init and keep a freelist if unused ones, or
	// ring buffer, or something
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool        = device->command_pool;
	allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer buffer;
	VkResult result = vkAllocateCommandBuffers(device->handle, &allocate_info, &buffer);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	result = vkBeginCommandBuffer(buffer, &begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	return buffer;
}

void end_command_buffer(VulkanDevice *device, VkCommandBuffer buffer)
{
	VkResult result = vkEndCommandBuffer(buffer);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &buffer;

	// TODO(jesper): we just submit to the graphics queue right now, good enough
	// for the forseable future but eventually there'll be compute
	// TODO(jesper): look into pooling up ready-to-submit command buffers and
	// submit them in a big batch, might be faster?
	vkQueueSubmit(device->queue, 1, &info, VK_NULL_HANDLE);
	// TODO(jesper): this seems like a bad idea, a better idea is probably to be
	// using semaphores and barriers, or let the caller decide whether it needs
	// to wait for everything to finish
	vkQueueWaitIdle(device->queue);

	vkFreeCommandBuffers(device->handle, device->command_pool, 1, &buffer);
}

VulkanSwapchain create_swapchain(VulkanDevice *device,
                                 VulkanPhysicalDevice *physical_device,
                                 VkSurfaceKHR surface,
                                 Settings *settings)

{
	VkResult result;
	VulkanSwapchain swapchain = {};
	swapchain.surface = surface;

	// figure out the color space for the swapchain
	u32 formats_count;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle,
	                                              swapchain.surface,
	                                              &formats_count,
	                                              nullptr);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkSurfaceFormatKHR *formats = new VkSurfaceFormatKHR[formats_count];
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device->handle,
	                                              swapchain.surface,
	                                              &formats_count,
	                                              formats);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// NOTE: if impl. reports only 1 surface format and that is undefined
	// it has no preferred format, so we choose BGRA8_UNORM
	if (formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		swapchain.format = VK_FORMAT_B8G8R8A8_UNORM;
	} else {
		swapchain.format = formats[0].format;
	}

	// TODO: does the above note affect the color space at all?
	VkColorSpaceKHR surface_colorspace = formats[0].colorSpace;

	VkSurfaceCapabilitiesKHR surface_capabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle,
	                                                   swapchain.surface,
	                                                   &surface_capabilities);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// figure out the present mode for the swapchain
	u32 present_modes_count;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->handle,
	                                                   swapchain.surface,
	                                                   &present_modes_count,
	                                                   nullptr);
	DEBUG_ASSERT(result == VK_SUCCESS);

	auto present_modes = new VkPresentModeKHR[present_modes_count];
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device->handle,
	                                                   swapchain.surface,
	                                                   &present_modes_count,
	                                                   present_modes);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (u32 i = 0; i < present_modes_count; ++i) {
		const VkPresentModeKHR &mode = present_modes[i];

		if (settings->video.vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			surface_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}

		if (!settings->video.vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			surface_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			break;
		}
	}

	swapchain.extent = surface_capabilities.currentExtent;
	if (swapchain.extent.width == (u32) (-1)) {
		// TODO(grouse): clean up usage of window dimensions
		DEBUG_ASSERT(settings->video.resolution.width  >= 0);
		DEBUG_ASSERT(settings->video.resolution.height >= 0);

		swapchain.extent.width  = (u32)settings->video.resolution.width;
		swapchain.extent.height = (u32)settings->video.resolution.height;
	}

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
	create_info.pQueueFamilyIndices   = &device->queue_family_index;
	create_info.preTransform          = surface_capabilities.currentTransform;
	create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode           = surface_present_mode;
	create_info.clipped               = VK_TRUE;
	create_info.oldSwapchain          = VK_NULL_HANDLE;

	result = vkCreateSwapchainKHR(device->handle,
	                              &create_info,
	                              nullptr,
	                              &swapchain.handle);

	DEBUG_ASSERT(result == VK_SUCCESS);

	delete[] present_modes;
	delete[] formats;

	result = vkGetSwapchainImagesKHR(device->handle,
	                                 swapchain.handle,
	                                 &swapchain.images_count,
	                                 nullptr);
	DEBUG_ASSERT(result == VK_SUCCESS);

	swapchain.images = new VkImage[swapchain.images_count];
	result = vkGetSwapchainImagesKHR(device->handle,
	                                 swapchain.handle,
	                                 &swapchain.images_count,
	                                 swapchain.images);
	DEBUG_ASSERT(result == VK_SUCCESS);

	swapchain.imageviews = new VkImageView[swapchain.images_count];

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

		result = vkCreateImageView(device->handle, &imageview_create_info, nullptr,
		                           &swapchain.imageviews[i]);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	return swapchain;
}

VkSampler create_sampler(VulkanDevice *device)
{
	VkSampler sampler;

	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter               = VK_FILTER_LINEAR;
	sampler_info.minFilter               = VK_FILTER_LINEAR;
	sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable           = VK_FALSE;
	sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VkResult result = vkCreateSampler(device->handle,
	                                  &sampler_info,
	                                  nullptr,
	                                  &sampler);
	DEBUG_ASSERT(result == VK_SUCCESS);

	return sampler;
}

VulkanShader create_shader(VulkanDevice *device,
                           VkShaderStageFlagBits stage,
                           const char *file)
{

	char *path = platform_resolve_path(GamePath_shaders, file);

	usize size;
	u32 *source = (u32*)platform_file_read(path, &size);
	DEBUG_ASSERT(source != nullptr);

	VkResult result;
	VulkanShader shader = {};
	// NOTE(jesper): this is the name of the entry point function in the shader,
	// is it at all worth supporting other entry point names? maybe if we start
	// using HLSL
	shader.name         = "main";
	shader.stage        = stage;

	VkShaderModuleCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	info.codeSize = size;
	info.pCode = source;

	result = vkCreateShaderModule(device->handle, &info, nullptr, &shader.module);
	DEBUG_ASSERT(result == VK_SUCCESS);

	free(source);
	free(path);

	return shader;
}

VulkanPipeline create_font_pipeline(VulkanDevice *device)
{
	VkResult result;
	VulkanPipeline pipeline = {};

	pipeline.shaders[ShaderStage_vertex] =
		create_shader(device, VK_SHADER_STAGE_VERTEX_BIT, "font.vert.spv");

	pipeline.shaders[ShaderStage_fragment] =
		create_shader(device, VK_SHADER_STAGE_FRAGMENT_BIT, "font.frag.spv");

	pipeline.texture_sampler = create_sampler(device);

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {};
	// texture sampler
	bindings[0].binding         = 1;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
	descriptor_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout_info.bindingCount = (u32)bindings.size();
	descriptor_layout_info.pBindings    = bindings.data();

	result = vkCreateDescriptorSetLayout(device->handle,
	                                     &descriptor_layout_info,
	                                     nullptr,
	                                     &pipeline.descriptor_layout);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// NOTE(jesper): create a pool size descriptor for each type of
	// descriptor this shader program uses
	std::array<VkDescriptorPoolSize, 1> pool_sizes = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = (u32)pool_sizes.size();
	pool_info.pPoolSizes    = pool_sizes.data();
	pool_info.maxSets       = 1;

	result = vkCreateDescriptorPool(device->handle,
	                                &pool_info,
	                                nullptr,
	                                &pipeline.descriptor_pool);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkDescriptorSetAllocateInfo descriptor_alloc_info = {};
	descriptor_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_alloc_info.descriptorPool     = pipeline.descriptor_pool;
	descriptor_alloc_info.descriptorSetCount = 1;
	descriptor_alloc_info.pSetLayouts        = &pipeline.descriptor_layout;

	result = vkAllocateDescriptorSets(device->handle,
	                                  &descriptor_alloc_info,
	                                  &pipeline.descriptor_set);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount         = 1;
	layout_info.pSetLayouts            = &pipeline.descriptor_layout;

	result = vkCreatePipelineLayout(device->handle,
	                                &layout_info,
	                                nullptr,
	                                &pipeline.layout);
	DEBUG_ASSERT(result == VK_SUCCESS);

	std::array<VkVertexInputBindingDescription, 1> vertex_bindings = {};
	vertex_bindings[0].binding   = 0;
	vertex_bindings[0].stride    = sizeof(f32) * 5;
	vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> vertex_descriptions = {};
	// vertices
	vertex_descriptions[0].location = 0;
	vertex_descriptions[0].binding  = 0;
	vertex_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_descriptions[0].offset   = 0;

	// texture coordinates
	vertex_descriptions[1].location = 1;
	vertex_descriptions[1].binding  = 0;
	vertex_descriptions[1].format   = VK_FORMAT_R32G32_SFLOAT;
	vertex_descriptions[1].offset   = sizeof(f32) * 3;

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount   = (u32)vertex_bindings.size();
	vertex_input_info.pVertexBindingDescriptions      = vertex_bindings.data();
	vertex_input_info.vertexAttributeDescriptionCount = (u32)vertex_descriptions.size();
	vertex_input_info.pVertexAttributeDescriptions    = vertex_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (f32) device->swapchain.extent.width;
	viewport.height   = (f32) device->swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = device->swapchain.extent;

	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.pViewports    = &viewport;
	viewport_info.scissorCount  = 1;
	viewport_info.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo raster_info = {};
	raster_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.depthClampEnable        = VK_FALSE;
	raster_info.rasterizerDiscardEnable = VK_FALSE;
	raster_info.polygonMode             = VK_POLYGON_MODE_FILL;
	raster_info.cullMode                = VK_CULL_MODE_NONE;
	raster_info.depthBiasEnable         = VK_FALSE;
	raster_info.lineWidth               = 1.0;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.blendEnable         = VK_TRUE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
	color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
	                                             VK_COLOR_COMPONENT_G_BIT |
	                                             VK_COLOR_COMPONENT_B_BIT |
	                                             VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
	color_blend_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.logicOpEnable     = VK_FALSE;
	color_blend_info.logicOp           = VK_LOGIC_OP_CLEAR;
	color_blend_info.attachmentCount   = 1;
	color_blend_info.pAttachments      = &color_blend_attachment;
	color_blend_info.blendConstants[0] = 0.0f;
	color_blend_info.blendConstants[1] = 0.0f;
	color_blend_info.blendConstants[2] = 0.0f;
	color_blend_info.blendConstants[3] = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisample_info = {};
	multisample_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisample_info.sampleShadingEnable   = VK_FALSE;
	multisample_info.minSampleShading      = 0;
	multisample_info.pSampleMask           = nullptr;
	multisample_info.alphaToCoverageEnable = VK_FALSE;
	multisample_info.alphaToOneEnable      = VK_FALSE;

	// NOTE(jesper): it seems like it'd be worth creating and caching this
	// inside the VulkanShader objects
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};
	shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage  = pipeline.shaders[ShaderStage_vertex].stage;
	shader_stages[0].module = pipeline.shaders[ShaderStage_vertex].module;
	shader_stages[0].pName  = pipeline.shaders[ShaderStage_vertex].name;

	shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage  = pipeline.shaders[ShaderStage_fragment].stage;
	shader_stages[1].module = pipeline.shaders[ShaderStage_fragment].module;
	shader_stages[1].pName  = pipeline.shaders[ShaderStage_fragment].name;

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount          = (u32)shader_stages.size();
	pipeline_info.pStages             = shader_stages.data();
	pipeline_info.pVertexInputState   = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState      = &viewport_info;
	pipeline_info.pRasterizationState = &raster_info;
	pipeline_info.pMultisampleState   = &multisample_info;
	pipeline_info.pColorBlendState    = &color_blend_info;
	pipeline_info.layout              = pipeline.layout;
	pipeline_info.renderPass          = device->renderpass;
	pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex   = -1;

	result = vkCreateGraphicsPipelines(device->handle,
	                                   VK_NULL_HANDLE,
	                                   1,
	                                   &pipeline_info,
	                                   nullptr,
	                                   &pipeline.handle);
	DEBUG_ASSERT(result == VK_SUCCESS);
	return pipeline;
}

VulkanPipeline create_pipeline(VulkanDevice *device)
{
	VkResult result;
	VulkanPipeline pipeline = {};

	pipeline.shaders[ShaderStage_vertex] =
		create_shader(device, VK_SHADER_STAGE_VERTEX_BIT, "generic.vert.spv");

	pipeline.shaders[ShaderStage_fragment] =
		create_shader(device, VK_SHADER_STAGE_FRAGMENT_BIT, "generic.frag.spv");

	pipeline.texture_sampler = create_sampler(device);

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
	// camera ubo
	bindings[0].binding         = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

	// texture sampler
	bindings[1].binding         = 1;
	bindings[1].descriptorCount = 1;
	bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
	descriptor_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout_info.bindingCount = (u32)bindings.size();
	descriptor_layout_info.pBindings    = bindings.data();

	result = vkCreateDescriptorSetLayout(device->handle,
	                                     &descriptor_layout_info,
	                                     nullptr,
	                                     &pipeline.descriptor_layout);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// NOTE(jesper): create a pool size descriptor for each type of
	// descriptor this shader program uses
	std::array<VkDescriptorPoolSize, 2> pool_sizes = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;

	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = (u32)pool_sizes.size();
	pool_info.pPoolSizes    = pool_sizes.data();
	pool_info.maxSets       = 1;

	result = vkCreateDescriptorPool(device->handle,
	                                &pool_info,
	                                nullptr,
	                                &pipeline.descriptor_pool);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkDescriptorSetAllocateInfo descriptor_alloc_info = {};
	descriptor_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_alloc_info.descriptorPool     = pipeline.descriptor_pool;
	descriptor_alloc_info.descriptorSetCount = 1;
	descriptor_alloc_info.pSetLayouts        = &pipeline.descriptor_layout;

	result = vkAllocateDescriptorSets(device->handle,
	                                  &descriptor_alloc_info,
	                                  &pipeline.descriptor_set);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkPushConstantRange push_constants = {};
	push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constants.offset = 0;
	push_constants.size = sizeof(Matrix4);

	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount         = 1;
	layout_info.pSetLayouts            = &pipeline.descriptor_layout;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges    = &push_constants;

	result = vkCreatePipelineLayout(device->handle,
	                                &layout_info,
	                                nullptr,
	                                &pipeline.layout);
	DEBUG_ASSERT(result == VK_SUCCESS);

	std::array<VkVertexInputBindingDescription, 1> vertex_bindings = {};
	vertex_bindings[0].binding   = 0;
	vertex_bindings[0].stride    = sizeof(f32) * 9;
	vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> vertex_descriptions = {};
	// vertices
	vertex_descriptions[0].location = 0;
	vertex_descriptions[0].binding  = 0;
	vertex_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_descriptions[0].offset   = 0;

	// color
	vertex_descriptions[1].location = 1;
	vertex_descriptions[1].binding  = 0;
	vertex_descriptions[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertex_descriptions[1].offset   = sizeof(f32) * 3;

	// texture coordinates
	vertex_descriptions[2].location = 2;
	vertex_descriptions[2].binding  = 0;
	vertex_descriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
	vertex_descriptions[2].offset   = sizeof(f32) * 7;

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount   = (u32)vertex_bindings.size();
	vertex_input_info.pVertexBindingDescriptions      = vertex_bindings.data();
	vertex_input_info.vertexAttributeDescriptionCount = (u32)vertex_descriptions.size();
	vertex_input_info.pVertexAttributeDescriptions    = vertex_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (f32) device->swapchain.extent.width;
	viewport.height   = (f32) device->swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = device->swapchain.extent;

	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.pViewports    = &viewport;
	viewport_info.scissorCount  = 1;
	viewport_info.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo raster_info = {};
	raster_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.depthClampEnable        = VK_FALSE;
	raster_info.rasterizerDiscardEnable = VK_FALSE;
	raster_info.polygonMode             = VK_POLYGON_MODE_FILL;
	raster_info.cullMode                = VK_CULL_MODE_NONE;
	raster_info.depthBiasEnable         = VK_FALSE;
	raster_info.lineWidth               = 1.0;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.blendEnable    = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
	                                        VK_COLOR_COMPONENT_G_BIT |
	                                        VK_COLOR_COMPONENT_B_BIT |
	                                        VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
	color_blend_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.logicOpEnable     = VK_FALSE;
	color_blend_info.logicOp           = VK_LOGIC_OP_CLEAR;
	color_blend_info.attachmentCount   = 1;
	color_blend_info.pAttachments      = &color_blend_attachment;
	color_blend_info.blendConstants[0] = 0.0f;
	color_blend_info.blendConstants[1] = 0.0f;
	color_blend_info.blendConstants[2] = 0.0f;
	color_blend_info.blendConstants[3] = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisample_info = {};
	multisample_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisample_info.sampleShadingEnable   = VK_FALSE;
	multisample_info.minSampleShading      = 0;
	multisample_info.pSampleMask           = nullptr;
	multisample_info.alphaToCoverageEnable = VK_FALSE;
	multisample_info.alphaToOneEnable      = VK_FALSE;

	// NOTE(jesper): it seems like it'd be worth creating and caching this
	// inside the VulkanShader objects
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};
	shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage  = pipeline.shaders[ShaderStage_vertex].stage;
	shader_stages[0].module = pipeline.shaders[ShaderStage_vertex].module;
	shader_stages[0].pName  = pipeline.shaders[ShaderStage_vertex].name;

	shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage  = pipeline.shaders[ShaderStage_fragment].stage;
	shader_stages[1].module = pipeline.shaders[ShaderStage_fragment].module;
	shader_stages[1].pName  = pipeline.shaders[ShaderStage_fragment].name;

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount          = (u32)shader_stages.size();
	pipeline_info.pStages             = shader_stages.data();
	pipeline_info.pVertexInputState   = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState      = &viewport_info;
	pipeline_info.pRasterizationState = &raster_info;
	pipeline_info.pMultisampleState   = &multisample_info;
	pipeline_info.pColorBlendState    = &color_blend_info;
	pipeline_info.layout              = pipeline.layout;
	pipeline_info.renderPass          = device->renderpass;
	pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex   = -1;

	result = vkCreateGraphicsPipelines(device->handle,
	                                   VK_NULL_HANDLE,
	                                   1,
	                                   &pipeline_info,
	                                   nullptr,
	                                   &pipeline.handle);
	DEBUG_ASSERT(result == VK_SUCCESS);
	return pipeline;
}

VulkanPipeline create_mesh_pipeline(VulkanDevice *device)
{
	VkResult result;
	VulkanPipeline pipeline = {};

	pipeline.shaders[ShaderStage_vertex] =
		create_shader(device, VK_SHADER_STAGE_VERTEX_BIT, "mesh.vert.spv");

	pipeline.shaders[ShaderStage_fragment] =
		create_shader(device, VK_SHADER_STAGE_FRAGMENT_BIT, "mesh.frag.spv");

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {};
	// camera ubo
	bindings[0].binding         = 0;
	bindings[0].descriptorCount = 1;
	bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].stageFlags      = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo descriptor_layout_info = {};
	descriptor_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout_info.bindingCount = (u32)bindings.size();
	descriptor_layout_info.pBindings    = bindings.data();

	result = vkCreateDescriptorSetLayout(device->handle,
	                                     &descriptor_layout_info,
	                                     nullptr,
	                                     &pipeline.descriptor_layout);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// NOTE(jesper): create a pool size descriptor for each type of
	// descriptor this shader program uses
	std::array<VkDescriptorPoolSize, 1> pool_sizes = {};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = (u32)pool_sizes.size();
	pool_info.pPoolSizes    = pool_sizes.data();
	pool_info.maxSets       = 1;

	result = vkCreateDescriptorPool(device->handle,
	                                &pool_info,
	                                nullptr,
	                                &pipeline.descriptor_pool);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkDescriptorSetAllocateInfo descriptor_alloc_info = {};
	descriptor_alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_alloc_info.descriptorPool     = pipeline.descriptor_pool;
	descriptor_alloc_info.descriptorSetCount = 1;
	descriptor_alloc_info.pSetLayouts        = &pipeline.descriptor_layout;

	result = vkAllocateDescriptorSets(device->handle,
	                                  &descriptor_alloc_info,
	                                  &pipeline.descriptor_set);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkPushConstantRange push_constants = {};
	push_constants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	push_constants.offset = 0;
	push_constants.size = sizeof(Matrix4);

	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount         = 1;
	layout_info.pSetLayouts            = &pipeline.descriptor_layout;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges    = &push_constants;

	result = vkCreatePipelineLayout(device->handle,
	                                &layout_info,
	                                nullptr,
	                                &pipeline.layout);
	DEBUG_ASSERT(result == VK_SUCCESS);

	std::array<VkVertexInputBindingDescription, 1> vertex_bindings = {};
	vertex_bindings[0].binding   = 0;
	vertex_bindings[0].stride    = sizeof(f32) * (3 + 3 + 2);
	vertex_bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> vertex_descriptions = {};
	// vertices
	vertex_descriptions[0].location = 0;
	vertex_descriptions[0].binding  = 0;
	vertex_descriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_descriptions[0].offset   = 0;

	// normals
	vertex_descriptions[1].location = 1;
	vertex_descriptions[1].binding  = 0;
	vertex_descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_descriptions[1].offset   = sizeof(f32) * 3;

	// uvs
	vertex_descriptions[2].location = 2;
	vertex_descriptions[2].binding  = 0;
	vertex_descriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
	vertex_descriptions[2].offset   = sizeof(f32) * (3 + 3);

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount   = (u32)vertex_bindings.size();
	vertex_input_info.pVertexBindingDescriptions      = vertex_bindings.data();
	vertex_input_info.vertexAttributeDescriptionCount = (u32)vertex_descriptions.size();
	vertex_input_info.pVertexAttributeDescriptions    = vertex_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
	input_assembly_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (f32) device->swapchain.extent.width;
	viewport.height   = (f32) device->swapchain.extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = device->swapchain.extent;

	VkPipelineViewportStateCreateInfo viewport_info = {};
	viewport_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount = 1;
	viewport_info.pViewports    = &viewport;
	viewport_info.scissorCount  = 1;
	viewport_info.pScissors     = &scissor;

	VkPipelineRasterizationStateCreateInfo raster_info = {};
	raster_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster_info.depthClampEnable        = VK_FALSE;
	raster_info.rasterizerDiscardEnable = VK_FALSE;
	raster_info.polygonMode             = VK_POLYGON_MODE_FILL;
	raster_info.cullMode                = VK_CULL_MODE_NONE;
	raster_info.depthBiasEnable         = VK_FALSE;
	raster_info.lineWidth               = 1.0;

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.blendEnable    = VK_FALSE;
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
	                                        VK_COLOR_COMPONENT_G_BIT |
	                                        VK_COLOR_COMPONENT_B_BIT |
	                                        VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend_info = {};
	color_blend_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_info.logicOpEnable     = VK_FALSE;
	color_blend_info.logicOp           = VK_LOGIC_OP_CLEAR;
	color_blend_info.attachmentCount   = 1;
	color_blend_info.pAttachments      = &color_blend_attachment;
	color_blend_info.blendConstants[0] = 0.0f;
	color_blend_info.blendConstants[1] = 0.0f;
	color_blend_info.blendConstants[2] = 0.0f;
	color_blend_info.blendConstants[3] = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisample_info = {};
	multisample_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisample_info.sampleShadingEnable   = VK_FALSE;
	multisample_info.minSampleShading      = 0;
	multisample_info.pSampleMask           = nullptr;
	multisample_info.alphaToCoverageEnable = VK_FALSE;
	multisample_info.alphaToOneEnable      = VK_FALSE;

	// NOTE(jesper): it seems like it'd be worth creating and caching this
	// inside the VulkanShader objects
	std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {};
	shader_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage  = pipeline.shaders[ShaderStage_vertex].stage;
	shader_stages[0].module = pipeline.shaders[ShaderStage_vertex].module;
	shader_stages[0].pName  = pipeline.shaders[ShaderStage_vertex].name;

	shader_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage  = pipeline.shaders[ShaderStage_fragment].stage;
	shader_stages[1].module = pipeline.shaders[ShaderStage_fragment].module;
	shader_stages[1].pName  = pipeline.shaders[ShaderStage_fragment].name;

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount          = (u32)shader_stages.size();
	pipeline_info.pStages             = shader_stages.data();
	pipeline_info.pVertexInputState   = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState      = &viewport_info;
	pipeline_info.pRasterizationState = &raster_info;
	pipeline_info.pMultisampleState   = &multisample_info;
	pipeline_info.pColorBlendState    = &color_blend_info;
	pipeline_info.layout              = pipeline.layout;
	pipeline_info.renderPass          = device->renderpass;
	pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex   = -1;

	result = vkCreateGraphicsPipelines(device->handle,
	                                   VK_NULL_HANDLE,
	                                   1,
	                                   &pipeline_info,
	                                   nullptr,
	                                   &pipeline.handle);
	DEBUG_ASSERT(result == VK_SUCCESS);
	return pipeline;
}

void copy_image(VulkanDevice *device,
                u32 width, u32 height,
                VkImage src, VkImage dst)
{
	VkCommandBuffer command = begin_command_buffer(device);

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

	vkCmdCopyImage(command,
	               src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
	               dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	               1, &region);

	end_command_buffer(device, command);
}

void transition_image(VkCommandBuffer command,
                      VkImage image,
                      VkImageLayout src,
                      VkImageLayout dst)
{
	VkImageAspectFlags aspect_mask = 0;
	switch (dst) {
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
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
	barrier.subresourceRange.levelCount     = 1;
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
	default:
		// TODO(jesper): unimplemented transfer
		DEBUG_ASSERT(false);
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
	default:
		// TODO(jesper): unimplemented transfer
		DEBUG_ASSERT(false);
		barrier.dstAccessMask = 0;
		break;
	}

	vkCmdPipelineBarrier(command,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &barrier);
}

void transition_image(VulkanDevice *device,
                      VkImage image,
                      VkImageLayout src,
                      VkImageLayout dst)
{
	VkCommandBuffer command = begin_command_buffer(device);
	transition_image(command, image, src, dst);
	end_command_buffer(device, command);
}

VkImage create_image(VulkanDevice *device,
                     VkFormat format,
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
	info.initialLayout     = VK_IMAGE_LAYOUT_PREINITIALIZED;
	info.usage             = usage;
	info.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateImage(device->handle, &info, nullptr, &image);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device->handle, image, &mem_requirements);

	// TODO(jesper): look into host coherent
	u32 memory_type = find_memory_type(device->physical_device,
	                                   mem_requirements.memoryTypeBits,
	                                   properties);
	DEBUG_ASSERT(memory_type != UINT32_MAX);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize       = mem_requirements.size;
	alloc_info.memoryTypeIndex      = memory_type;

	result = vkAllocateMemory(device->handle, &alloc_info, nullptr, memory);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkBindImageMemory(device->handle, image, *memory, 0);

	return image;
}

VulkanTexture create_texture(VulkanDevice *device, u32 width, u32 height,
                             VkFormat format, void *pixels,
                             VkComponentMapping components)
{
	VkResult result;

	VulkanTexture texture = {};
	texture.format = format;
	texture.width  = width;
	texture.height = height;

	VkDeviceMemory staging_memory;
	VkImage staging_image = create_image(device,
	                                     format, width, height,
	                                     VK_IMAGE_TILING_LINEAR,
	                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
	                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                     &staging_memory);


	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel   = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout staging_image_layout;
	vkGetImageSubresourceLayout(device->handle, staging_image,
	                            &subresource, &staging_image_layout);

	void *data;
	// TODO(jesper): hardcoded 1 byte per channel and 4 channels, lookup based
	// on the receieved VkFormat

	i32 num_channels      = 4;
	i32 bytes_per_channel = 4;
	switch (format) {
	case VK_FORMAT_R8_UNORM:
	case VK_FORMAT_R8_UINT:
		num_channels = 1;
		bytes_per_channel = 1;
		break;
	default:
		DEBUG_LOG(Log_warning, "unhandled format in determining number of bytes "
		          "per channel and number of channels");
		break;
	}

	VkDeviceSize size = width * height * num_channels * bytes_per_channel;
	vkMapMemory(device->handle, staging_memory, 0, size, 0, &data);

	u32 row_pitch = width * num_channels * bytes_per_channel;
	if (staging_image_layout.rowPitch == row_pitch) {
		memcpy(data, pixels, (usize)size);
	} else {
		u8 *bytes = (u8*)data;
		u8 *pixel_bytes = (u8*)pixels;
		for (i32 y = 0; y < (i32)height; y++) {
			memcpy(&bytes[y * num_channels * staging_image_layout.rowPitch],
			       &pixel_bytes[y * width * num_channels * bytes_per_channel],
			       width * num_channels * bytes_per_channel);
		}
	}

	vkUnmapMemory(device->handle, staging_memory);

	texture.image = create_image(device,
	                             format, width, height,
	                             VK_IMAGE_TILING_OPTIMAL,
	                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	                             VK_IMAGE_USAGE_SAMPLED_BIT,
	                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                             &texture.memory);

	transition_image(device,
	                 staging_image,
	                 VK_IMAGE_LAYOUT_PREINITIALIZED,
	                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transition_image(device,
	                 texture.image,
	                 VK_IMAGE_LAYOUT_PREINITIALIZED,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_image(device, width, height, staging_image, texture.image);
	transition_image(device,
	                 texture.image,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
	                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageViewCreateInfo view_info = {};
	view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image                           = texture.image;
	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format                          = texture.format;
	view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;
	view_info.components = components;

	result = vkCreateImageView(device->handle, &view_info, nullptr,
	                           &texture.image_view);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkFreeMemory(device->handle, staging_memory, nullptr);
	vkDestroyImage(device->handle, staging_image, nullptr);

	return texture;
}


VulkanDevice create_device(Settings *settings, PlatformState *platform)
{
	VulkanDevice device = {};

	VkResult result;
	/**************************************************************************
	 * Create VkInstance
	 *************************************************************************/
	{
		// NOTE(jesper): currently we don't assert about missing required
		// extensions or layers, and we don't store any internal state about
		// which ones we've enabled.
		u32 supported_layers_count = 0;
		result = vkEnumerateInstanceLayerProperties(&supported_layers_count,
		                                            nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		auto supported_layers = (VkLayerProperties*)malloc(sizeof(VkLayerProperties) *
			                                               supported_layers_count);
		result = vkEnumerateInstanceLayerProperties(&supported_layers_count,
		                                            supported_layers);
		DEBUG_ASSERT(result == VK_SUCCESS);

		for (u32 i = 0; i < supported_layers_count; i++) {
			DEBUG_LOG("VkLayerProperties[%u]", i);
			DEBUG_LOG("  layerName            : %s",
			          supported_layers[i].layerName);
			DEBUG_LOG("  specVersion          : %u.%u.%u",
			          VK_VERSION_MAJOR(supported_layers[i].specVersion),
			          VK_VERSION_MINOR(supported_layers[i].specVersion),
			          VK_VERSION_PATCH(supported_layers[i].specVersion));
			DEBUG_LOG("  implementationVersion: %u",
			          supported_layers[i].implementationVersion);
			DEBUG_LOG("  description          : %s",
			          supported_layers[i].description);
		}

		u32 supported_extensions_count = 0;
		result = vkEnumerateInstanceExtensionProperties(nullptr,
		                                                &supported_extensions_count,
		                                                nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		auto supported_extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) *
			                                                       supported_extensions_count);
		result = vkEnumerateInstanceExtensionProperties(nullptr,
		                                                &supported_extensions_count,
		                                                supported_extensions);
		DEBUG_ASSERT(result == VK_SUCCESS);

		for (u32 i = 0; i < supported_extensions_count; i++) {
			DEBUG_LOG("vkExtensionProperties[%u]", i);
			DEBUG_LOG("  extensionName: %s", supported_extensions[i].extensionName);
			DEBUG_LOG("  specVersion  : %u", supported_extensions[i].specVersion);
		}


		// NOTE(jesper): we might want to store these in the device for future
		// usage/debug information
		i32 enabled_layers_count = 0;
		char **enabled_layers = (char**) malloc(sizeof(char*) *
		                                        supported_layers_count);

		i32 enabled_extensions_count = 0;
		char **enabled_extensions = (char**) malloc(sizeof(char*) *
		                                            supported_extensions_count);

		for (i32 i = 0; i < (i32)supported_layers_count; ++i) {
			VkLayerProperties &layer = supported_layers[i];

			if (platform_vulkan_enable_instance_layer(layer) ||
			    strcmp(layer.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
			{
				enabled_layers[enabled_layers_count++] = layer.layerName;
			}
		}

		for (i32 i = 0; i < (i32)supported_extensions_count; ++i) {
			VkExtensionProperties &extension = supported_extensions[i];

			if (platform_vulkan_enable_instance_extension(extension) ||
			    strcmp(extension.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0 ||
			    strcmp(extension.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
			{
				enabled_extensions[enabled_extensions_count++] = extension.extensionName;
			}
		}

		// Create the VkInstance
		VkApplicationInfo app_info = {};
		app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName   = "leary";
		app_info.applicationVersion = 1;
		app_info.pEngineName        = "leary";
		app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 37);

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledLayerCount = (u32) enabled_layers_count;
		create_info.ppEnabledLayerNames = enabled_layers;
		create_info.enabledExtensionCount = (u32) enabled_extensions_count;
		create_info.ppEnabledExtensionNames = enabled_extensions;

		result = vkCreateInstance(&create_info, nullptr, &device.instance);
		DEBUG_ASSERT(result == VK_SUCCESS);

		free(enabled_layers);
		free(enabled_extensions);

		free(supported_layers);
		free(supported_extensions);
	}

	/**************************************************************************
	 * Create debug callbacks
	 *************************************************************************/
	{

		CreateDebugReportCallbackEXT =
			(PFN_vkCreateDebugReportCallbackEXT)
			vkGetInstanceProcAddr(device.instance,
			                      "vkCreateDebugReportCallbackEXT");

		DestroyDebugReportCallbackEXT =
			(PFN_vkDestroyDebugReportCallbackEXT)
			vkGetInstanceProcAddr(device.instance,
			                      "vkDestroyDebugReportCallbackEXT");

		VkDebugReportCallbackCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

		create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
		    VK_DEBUG_REPORT_WARNING_BIT_EXT |
		    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		    VK_DEBUG_REPORT_DEBUG_BIT_EXT;

		create_info.pfnCallback = &debug_callback_func;

		result = CreateDebugReportCallbackEXT(device.instance,
		                                      &create_info,
		                                      nullptr,
		                                      &device.debug_callback);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	/**************************************************************************
	 * Create and choose VkPhysicalDevice
	 *************************************************************************/
	{
		u32 count = 0;
		result = vkEnumeratePhysicalDevices(device.instance, &count, nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkPhysicalDevice *physical_devices = new VkPhysicalDevice[count];
		result = vkEnumeratePhysicalDevices(device.instance,
		                                    &count, physical_devices);
		DEBUG_ASSERT(result == VK_SUCCESS);

		bool found_device = false;
		for (u32 i = 0; i < count; i++) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physical_devices[i], &properties);

			DEBUG_LOG("VkPhysicalDeviceProperties[%u]", i);
			DEBUG_LOG("  apiVersion    : %d.%d.%d",
			          VK_VERSION_MAJOR(properties.apiVersion),
			          VK_VERSION_MINOR(properties.apiVersion),
			          VK_VERSION_PATCH(properties.apiVersion));
			// NOTE(jesper): only confirmed to be accurate, by experimentation,
			// on nvidia
			DEBUG_LOG("  driverVersion : %u.%u",
			          (properties.driverVersion >> 22),
			          (properties.driverVersion >> 14) & 0xFF);
			DEBUG_LOG("  vendorID      : 0x%X %s",
			          properties.vendorID,
			          vendor_string(properties.vendorID));
			DEBUG_LOG("  deviceID      : 0x%X", properties.deviceID);
			switch (properties.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				DEBUG_LOG("  deviceType: Integrated GPU");
				if (!found_device) {
					device.physical_device.handle     = physical_devices[i];
					device.physical_device.properties = properties;
				}
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				DEBUG_LOG("  deviceType    : Discrete GPU");
				device.physical_device.handle     = physical_devices[i];
				device.physical_device.properties = properties;
				found_device = true;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				DEBUG_LOG("  deviceType    : Virtual GPU");
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				DEBUG_LOG("  deviceType    : CPU");
				break;
			default:
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				DEBUG_LOG("  deviceType    : Unknown");
				break;
			}
			DEBUG_LOG("  deviceName    : %s", properties.deviceName);
		}

		vkGetPhysicalDeviceMemoryProperties(device.physical_device.handle,
		                                    &device.physical_device.memory);

		vkGetPhysicalDeviceFeatures(device.physical_device.handle,
		                            &device.physical_device.features);
		delete[] physical_devices;
	}

	// NOTE(jesper): this is so annoying. The surface belongs with the swapchain
	// (imo), but to create the swapchain we need the device, and to create the
	// device we need the surface
	VkSurfaceKHR surface;
	result = vulkan_create_surface(device.instance, &surface, *platform);
	DEBUG_ASSERT(result == VK_SUCCESS);


	/**************************************************************************
	 * Create VkDevice and get its queue
	 *************************************************************************/
	{
		u32 queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device.handle,
		                                         &queue_family_count,
		                                         nullptr);

		auto queue_families = new VkQueueFamilyProperties[queue_family_count];
		vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device.handle,
		                                         &queue_family_count,
		                                         queue_families);

		device.queue_family_index = 0;
		for (u32 i = 0; i < queue_family_count; ++i) {
			const VkQueueFamilyProperties &property = queue_families[i];

			// figure out if the queue family supports present
			VkBool32 supports_present = VK_FALSE;
			result = vkGetPhysicalDeviceSurfaceSupportKHR(device.physical_device.handle,
			                                              device.queue_family_index,
			                                              surface,
			                                              &supports_present);
			DEBUG_ASSERT(result == VK_SUCCESS);


			DEBUG_LOG("VkQueueFamilyProperties[%u]", i);
			DEBUG_LOG("  queueCount                 : %u",
			          property.queueCount);
			DEBUG_LOG("  timestampValidBits         : %u",
			          property.timestampValidBits);
			DEBUG_LOG("  minImageTransferGranualrity: (%u, %u, %u)",
			          property.minImageTransferGranularity.depth,
			          property.minImageTransferGranularity.height,
			          property.minImageTransferGranularity.depth);
			DEBUG_LOG("  supportsPresent            : %d",
			          (i32)supports_present);

			// if it doesn't we keep on searching
			if (supports_present == VK_FALSE)
				continue;

			// we're just interested in getting a graphics queue going for
			// now, so choose the first one
			// TODO: COMPUTE: find a compute queue, potentially asynchronous
			// (separate from graphics queue)
			// TODO: get a separate queue for transfer if one exist to do
			// buffer copy commands on while graphics/compute queue is doing
			// its own thing
			if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				device.queue_family_index = i;
				break;
			}
		}

		// TODO: when we have more than one queue we'll need to figure out how
		// to handle this, for now just set highest queue priroity for the 1
		// queue we create
		f32 priority = 1.0f;

		VkDeviceQueueCreateInfo queue_info = {};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = device.queue_family_index;
		queue_info.queueCount       = 1;
		queue_info.pQueuePriorities = &priority;

		// TODO: look into other extensions
		const char *device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount    = 1;
		device_info.pQueueCreateInfos       = &queue_info;
		device_info.enabledExtensionCount   = 1;
		device_info.ppEnabledExtensionNames = device_extensions;
		device_info.pEnabledFeatures        = &device.physical_device.features;

		result = vkCreateDevice(device.physical_device.handle,
		                        &device_info,
		                        nullptr,
		                        &device.handle);
		DEBUG_ASSERT(result == VK_SUCCESS);

		// NOTE: does it matter which queue we choose?
		u32 queue_index = 0;
		vkGetDeviceQueue(device.handle,
		                 device.queue_family_index,
		                 queue_index,
		                 &device.queue);

		delete[] queue_families;
	}

	/**************************************************************************
	 * Create VkSwapchainKHR
	 *************************************************************************/
	// NOTE(jesper): the swapchain is stuck in the VulkanDevice creation right
	// now because we're still hardcoding the depth buffer creation, among other
	// things, in the VulkanDevice, which requires the created swapchain.
	// Really I think it mostly/only need the extent, but same difference
	device.swapchain = create_swapchain(&device, &device.physical_device,
	                                    surface, settings);

	/**************************************************************************
	 * Create VkCommandPool
	 *************************************************************************/
	{
		VkCommandPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		create_info.queueFamilyIndex = device.queue_family_index;

		result = vkCreateCommandPool(device.handle,
		                             &create_info,
		                             nullptr,
		                             &device.command_pool);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	/**************************************************************************
	 * Create vkRenderPass
	 *************************************************************************/
	{
		std::array<VkAttachmentDescription, 1> attachment_descriptions = {};
		attachment_descriptions[0].format         = device.swapchain.format;
		attachment_descriptions[0].samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment_descriptions[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachment_descriptions[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment_descriptions[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment_descriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment_descriptions[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment_descriptions[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		std::array<VkAttachmentReference, 1> attachment_references = {};
		attachment_references[0].attachment = 0;
		attachment_references[0].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass_description = {};
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.inputAttachmentCount    = 0;
		subpass_description.pInputAttachments       = nullptr;
		subpass_description.colorAttachmentCount    = (u32)attachment_references.size();
		subpass_description.pColorAttachments       = attachment_references.data();
		subpass_description.pResolveAttachments     = nullptr;
		subpass_description.preserveAttachmentCount = 0;
		subpass_description.pPreserveAttachments    = nullptr;

		VkRenderPassCreateInfo create_info = {};
		create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		create_info.attachmentCount = (u32)attachment_descriptions.size();
		create_info.pAttachments    = attachment_descriptions.data();
		create_info.subpassCount    = 1;
		create_info.pSubpasses      = &subpass_description;
		create_info.dependencyCount = 0;
		create_info.pDependencies   = nullptr;

		result = vkCreateRenderPass(device.handle,
		                            &create_info,
		                            nullptr,
		                            &device.renderpass);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	/**************************************************************************
	 * Create Framebuffers
	 *************************************************************************/
	{
		device.framebuffers_count = (i32)device.swapchain.images_count;
		device.framebuffers = (VkFramebuffer*) malloc(sizeof(VkFramebuffer) *
		                                              device.framebuffers_count);

		VkFramebufferCreateInfo create_info = {};
		create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		create_info.renderPass      = device.renderpass;
		create_info.attachmentCount = 1;
		create_info.width           = device.swapchain.extent.width;
		create_info.height          = device.swapchain.extent.height;
		create_info.layers          = 1;

		for (i32 i = 0; i < device.framebuffers_count; ++i)
		{
			create_info.pAttachments = &device.swapchain.imageviews[i];

			result = vkCreateFramebuffer(device.handle,
			                             &create_info,
			                             nullptr,
			                             &device.framebuffers[i]);
			DEBUG_ASSERT(result == VK_SUCCESS);
		}
	}

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result = vkCreateSemaphore(device.handle,
	                           &semaphore_info,
	                           nullptr,
	                           &device.swapchain.available);
	DEBUG_ASSERT(result == VK_SUCCESS);

	result = vkCreateSemaphore(device.handle,
	                           &semaphore_info,
	                           nullptr,
	                           &device.render_completed);
	DEBUG_ASSERT(result == VK_SUCCESS);

	/**************************************************************************
	 * Create VkCommandBuffer for frame
	 *************************************************************************/
	{
		// NOTE: we want to allocate all the command buffers we're going to need
		// in the game at once.
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandPool        = device.command_pool;
		allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandBufferCount = 1;

		result = vkAllocateCommandBuffers(device.handle, &allocate_info, &device.cmd_present);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	return device;
}

void destroy(VulkanDevice *device, VulkanPipeline pipeline)
{
	// TODO(jesper): find a better way to clean these up; not every pipeline
	// will have every shader stage and we'll probably want to keep shader
	// stages in a map of some sort of in the device to reuse
	vkDestroyShaderModule(device->handle,
	                      pipeline.shaders[ShaderStage_vertex].module,
	                      nullptr);
	vkDestroyShaderModule(device->handle,
	                      pipeline.shaders[ShaderStage_fragment].module,
	                      nullptr);

	vkDestroySampler(device->handle, pipeline.texture_sampler, nullptr);
	vkDestroyDescriptorPool(device->handle, pipeline.descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(device->handle, pipeline.descriptor_layout, nullptr);
	vkDestroyPipelineLayout(device->handle, pipeline.layout, nullptr);
	vkDestroyPipeline(device->handle, pipeline.handle, nullptr);
}

void destroy(VulkanDevice *device, VulkanSwapchain swapchain)
{
	for (i32 i = 0; i < (i32)device->swapchain.images_count; i++) {
		vkDestroyImageView(device->handle, swapchain.imageviews[i], nullptr);
	}

	vkDestroySwapchainKHR(device->handle, swapchain.handle, nullptr);
	vkDestroySurfaceKHR(device->instance, swapchain.surface, nullptr);

	delete[] swapchain.imageviews;
	delete[] swapchain.images;
}

void destroy(VulkanDevice *device, VulkanTexture texture)
{
	vkDestroyImageView(device->handle, texture.image_view, nullptr);
	vkDestroyImage(device->handle, texture.image, nullptr);
	vkFreeMemory(device->handle, texture.memory, nullptr);
}

void destroy(VulkanDevice *device, VulkanBuffer buffer)
{
	vkFreeMemory(device->handle, buffer.memory, nullptr);
	vkDestroyBuffer(device->handle, buffer.handle, nullptr);
}

void destroy(VulkanDevice *device, VulkanUniformBuffer ubo)
{
	destroy(device, ubo.staging);
	destroy(device, ubo.buffer);
}

void destroy(VulkanDevice *device)
{
	VkResult result;
	VAR_UNUSED(result);

	for (i32 i = 0; i < device->framebuffers_count; ++i) {
		vkDestroyFramebuffer(device->handle, device->framebuffers[i], nullptr);
	}
	free(device->framebuffers);


	vkDestroyRenderPass(device->handle, device->renderpass, nullptr);


	vkFreeCommandBuffers(device->handle, device->command_pool, 1, &device->cmd_present);
	vkDestroyCommandPool(device->handle, device->command_pool, nullptr);


	vkDestroySemaphore(device->handle, device->swapchain.available, nullptr);
	vkDestroySemaphore(device->handle, device->render_completed, nullptr);


	// TODO(jesper): move out of here when the swapchain<->device dependency is
	// fixed
	destroy(device, device->swapchain);


	vkDestroyDevice(device->handle,     nullptr);

	DestroyDebugReportCallbackEXT(device->instance, device->debug_callback, nullptr);

	vkDestroyInstance(device->instance, nullptr);
}

VulkanBuffer create_buffer(VulkanDevice *device,
                           usize size,
                           VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags memory_flags)
{
	VulkanBuffer buffer = {};
	buffer.size = size;

	VkBufferCreateInfo create_info = {};
	create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size                  = size;
	create_info.usage                 = usage;
	create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices   = nullptr;

	VkResult result = vkCreateBuffer(device->handle,
	                                 &create_info,
	                                 nullptr,
	                                 &buffer.handle);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// TODO: allocate buffers from large memory pool in VulkanDevice
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(device->handle, buffer.handle, &memory_requirements);

	u32 index = find_memory_type(device->physical_device,
	                             memory_requirements.memoryTypeBits,
	                             memory_flags);
	DEBUG_ASSERT(index != UINT32_MAX);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize  = memory_requirements.size;
	allocate_info.memoryTypeIndex = index;

	result = vkAllocateMemory(device->handle,
	                          &allocate_info,
	                          nullptr,
	                          &buffer.memory);
	DEBUG_ASSERT(result == VK_SUCCESS);

	result = vkBindBufferMemory(device->handle, buffer.handle, buffer.memory, 0);
	DEBUG_ASSERT(result == VK_SUCCESS);
	return buffer;
}

void copy_buffer(VulkanDevice *device, VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBuffer command = begin_command_buffer(device);

	VkBufferCopy region = {};
	region.srcOffset    = 0;
	region.dstOffset    = 0;
	region.size         = size;

	vkCmdCopyBuffer(command, src, dst, 1, &region);

	end_command_buffer(device, command);
}

VulkanBuffer create_vertex_buffer(VulkanDevice *device, usize size, void *data)
{
#if 0 // upload to staging buffer then copy to device local
	VulkanBuffer staging = create_buffer(device, size,
	                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void *mapped;
	vkMapMemory(device->handle, staging.memory, 0, size, 0, &mapped);
	memcpy(mapped, data, size);
	vkUnmapMemory(device->handle, staging.memory);

	VulkanBuffer vbo = create_buffer(device, size,
	                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
	                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copy_buffer(device, staging.handle, vbo.handle, size);
	destroy(device, staging);
#else
	VulkanBuffer vbo = create_buffer(device, size,
	                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	void *mapped;
	vkMapMemory(device->handle, vbo.memory, 0, size, 0, &mapped);
	memcpy(mapped, data, size);
	vkUnmapMemory(device->handle, vbo.memory);
#endif

	return vbo;
}

VulkanBuffer create_vertex_buffer(VulkanDevice *device, usize size)
{
	VulkanBuffer vbo = create_buffer(device, size,
	                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	return vbo;
}

VulkanBuffer create_index_buffer(VulkanDevice *device,
                                 u32 *indices, usize size)
{
	VulkanBuffer staging = create_buffer(device, size,
	                                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void *data;
	vkMapMemory(device->handle, staging.memory, 0, size, 0, &data);
	memcpy(data, indices, size);
	vkUnmapMemory(device->handle, staging.memory);

	VulkanBuffer ib = create_buffer(device, size,
	                                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
	                                VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
	                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	copy_buffer(device, staging.handle, ib.handle, size);

	destroy(device, staging);

	return ib;
}

VulkanUniformBuffer create_uniform_buffer(VulkanDevice *device, usize size)
{
	VulkanUniformBuffer ubo;
	ubo.staging = create_buffer(device,
	                            size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	ubo.buffer = create_buffer(device,
	                           size,
	                           VK_BUFFER_USAGE_TRANSFER_DST_BIT |
	                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
	                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	return ubo;
}


void update_uniform_data(VulkanDevice *device,
                         VulkanUniformBuffer ubo,
                         void *data,
                         usize offset, usize size)
{
	void *mapped;
	vkMapMemory(device->handle,
	            ubo.staging.memory,
	            offset, size,
	            0,
	            &mapped);
	memcpy(mapped, data, size);
	vkUnmapMemory(device->handle, ubo.staging.memory);

	copy_buffer(device, ubo.staging.handle, ubo.buffer.handle, size);
}

void update_descriptor_sets(VulkanDevice *device,
                            VulkanPipeline pipeline,
                            VulkanTexture texture,
                            VulkanUniformBuffer ubo)
{
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = ubo.buffer.handle;
	buffer_info.offset = 0;
	buffer_info.range  = ubo.buffer.size;

	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView   = texture.image_view;
	image_info.sampler     = pipeline.texture_sampler;

	std::array<VkWriteDescriptorSet, 2> descriptor_writes = {};
	descriptor_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet          = pipeline.descriptor_set;
	descriptor_writes[0].dstBinding      = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo     = &buffer_info;

	descriptor_writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet          = pipeline.descriptor_set;
	descriptor_writes[1].dstBinding      = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pImageInfo      = &image_info;

	vkUpdateDescriptorSets(device->handle,
	                       (u32)descriptor_writes.size(), descriptor_writes.data(),
	                       0, nullptr);
}

void update_descriptor_sets(VulkanDevice *device,
                            VulkanPipeline pipeline,
                            VulkanUniformBuffer ubo)
{
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = ubo.buffer.handle;
	buffer_info.offset = 0;
	buffer_info.range  = ubo.buffer.size;

	std::array<VkWriteDescriptorSet, 1> descriptor_writes = {};
	descriptor_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet          = pipeline.descriptor_set;
	descriptor_writes[0].dstBinding      = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo     = &buffer_info;

	vkUpdateDescriptorSets(device->handle,
	                       (u32)descriptor_writes.size(), descriptor_writes.data(),
	                       0, nullptr);
}

void update_descriptor_sets(VulkanDevice *device,
                            VulkanPipeline pipeline,
                            VulkanTexture texture)
{
	VkDescriptorImageInfo image_info = {};
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView   = texture.image_view;
	image_info.sampler     = pipeline.texture_sampler;

	std::array<VkWriteDescriptorSet, 1> descriptor_writes = {};
	descriptor_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet          = pipeline.descriptor_set;
	descriptor_writes[0].dstBinding      = 1;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pImageInfo      = &image_info;

	vkUpdateDescriptorSets(device->handle,
	                       (u32)descriptor_writes.size(), descriptor_writes.data(),
	                       0, nullptr);
}

u32 acquire_swapchain_image(VulkanDevice *device)
{
	VkResult result;
	u32 image_index;
	result = vkAcquireNextImageKHR(device->handle,
	                               device->swapchain.handle,
	                               UINT64_MAX,
	                               device->swapchain.available,
	                               VK_NULL_HANDLE,
	                               &image_index);
	DEBUG_ASSERT(result == VK_SUCCESS);
	return image_index;
}

