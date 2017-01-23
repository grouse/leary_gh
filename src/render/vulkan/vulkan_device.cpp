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

#include "vulkan_device.h"

#include <limits>
#include <fstream>

#include "platform/debug.h"
#include "platform/file.h"

#include "core/math.h"


namespace
{
	constexpr int VERTEX_INPUT_BINDING = 0;	// The Vertex Input Binding for our vertex buffer.

	// Vertex data to draw.
	constexpr int NUM_DEMO_VERTICES = 3;

	struct ColoredVertex {
		Vector3f pos;
		Vector3f color;
	};

	const ColoredVertex vertices[NUM_DEMO_VERTICES * 3] =
	{
	    //      position             color
		{ { 0.5f,  0.5f,  0.0f},  {0.1f, 0.8f, 0.1f} },
		{ {-0.5f,  0.5f,  0.0f},  {0.8f, 0.1f, 0.1f} },
		{ { 0.0f, -0.5f,  0.0f},  {0.1f, 0.1f, 0.8f} }
	};
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback_func(VkFlags                    flags,
                    VkDebugReportObjectTypeEXT type,
                    uint64_t                   src,
                    size_t                     location,
                    int32_t                    code,
                    const char*                prefix,
                    const char*                msg,
                    void*                      data)
{
	// NOTE: these might be useful?
	VAR_UNUSED(type);
	VAR_UNUSED(src);
	VAR_UNUSED(location);

	// NOTE: we never set any data when creating the callback, so useless for
	// now
	VAR_UNUSED(data);

	LogType log_type;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		log_type = LogType::error;
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		log_type = LogType::warning;
	else if (flags & (VK_DEBUG_REPORT_DEBUG_BIT_EXT |
	                  VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
		log_type = LogType::info;
	else
		// NOTE: this would only happen if they extend the report callback
		// flags
		log_type = LogType::info;

	DEBUG_LOGF(log_type, "[VULKAN]: %s (%d) - %s", prefix, code, msg);
	return false;
}

void VulkanDevice::copy_image(uint32_t width, uint32_t height,
                              VkImage src, VkImage dst)
{
	VkCommandBuffer command = begin_command_buffer();

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

	end_command_buffer(command);
}

void VulkanDevice::transition_image(VkImage image,
                                    VkImageLayout src,
                                    VkImageLayout dst)
{
	VkCommandBuffer command = begin_command_buffer();

	VkImageMemoryBarrier barrier = {};
	barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout                       = src;
	barrier.newLayout                       = dst;
	barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	barrier.image                           = image;
	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	// TODO(jesper): support mip layers
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	switch (src) {
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

	end_command_buffer(command);
}

VkCommandBuffer VulkanDevice::begin_command_buffer()
{
	// TODO(jesper): don't allocate command buffers on demand; allocate a big
	// pool of them in the device init and keep a freelist if unused ones, or
	// ring buffer, or something
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool        = vk_command_pool;
	allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer buffer;
	VkResult result = vkAllocateCommandBuffers(vk_device, &allocate_info, &buffer);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	result = vkBeginCommandBuffer(buffer, &begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	return buffer;
}

void VulkanDevice::end_command_buffer(VkCommandBuffer buffer)
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
	vkQueueSubmit(vk_queue, 1, &info, VK_NULL_HANDLE);
	// TODO(jesper): this seems like a bad idea, a better idea is probably to be
	// using semaphores and barriers, or let the caller decide whether it needs
	// to wait for everything to finish
	vkQueueWaitIdle(vk_queue);

	vkFreeCommandBuffers(vk_device, vk_command_pool, 1, &buffer);
}

VkImage VulkanDevice::create_image(VkFormat format,
                                   uint32_t width,
                                   uint32_t height,
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

	VkResult result = vkCreateImage(vk_device, &info, nullptr, &image);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(vk_device, image, &mem_requirements);

	// TODO(jesper): look into host coherent
	uint32_t memory_type = find_memory_type(mem_requirements.memoryTypeBits,
	                                        properties);
	DEBUG_ASSERT(memory_type != -1);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize       = mem_requirements.size;
	alloc_info.memoryTypeIndex      = memory_type;

	result = vkAllocateMemory(vk_device, &alloc_info, nullptr, memory);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkBindImageMemory(vk_device, image, *memory, 0);

	return image;
}

VulkanTexture VulkanDevice::create_texture(uint32_t width,
                                           uint32_t height,
                                           VkFormat format,
                                           uint8_t *pixels)
{
	VkResult result;

	VulkanTexture texture = {};
	texture.format = format;
	texture.width  = width;
	texture.height = height;

	VkDeviceMemory staging_memory;
	VkImage staging_image = create_image(format, width, height,
	                                     VK_IMAGE_TILING_LINEAR,
	                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
	                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                     &staging_memory);

	void *data;
	// TODO(jesper): hardcoded 1 byte per channel and 4 channels, lookup based
	// on the receieved VkFormat
	VkDeviceSize size = width * height * 4;
	vkMapMemory(vk_device, staging_memory, 0, size, 0, &data);

	VkImageSubresource subresource = {};
	subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource.mipLevel   = 0;
	subresource.arrayLayer = 0;

	VkSubresourceLayout staging_image_layout;
	vkGetImageSubresourceLayout(vk_device, staging_image,
	                            &subresource, &staging_image_layout);

	if (staging_image_layout.rowPitch == width * 4) {
		memcpy(data, pixels, (size_t)size);
	} else {
		uint8_t *bytes = (uint8_t*)data;
		for (int32_t y = 0; y < (int32_t)height; y++) {
			memcpy(&bytes[y * staging_image_layout.rowPitch],
			       &pixels[y * width * 4],
			       width * 4);
		}
	}

	vkUnmapMemory(vk_device, staging_memory);

	texture.image = create_image(format, width, height,
	                             VK_IMAGE_TILING_OPTIMAL,
	                             VK_IMAGE_USAGE_TRANSFER_DST_BIT |
	                             VK_IMAGE_USAGE_SAMPLED_BIT,
	                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                             &texture.memory);

	transition_image(staging_image,
	                 VK_IMAGE_LAYOUT_PREINITIALIZED,
	                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transition_image(texture.image,
	                 VK_IMAGE_LAYOUT_PREINITIALIZED,
	                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_image(width, height, staging_image, texture.image);
	transition_image(texture.image,
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

	result = vkCreateImageView(vk_device, &view_info, nullptr,
	                           &texture.image_view);
	DEBUG_ASSERT(result == VK_SUCCESS);

	return texture;
}

void
VulkanDevice::create(Settings settings, PlatformState platform_state)
{
	VkResult result;
	/**************************************************************************
	 * Create VkInstance
	 *************************************************************************/
	{
		// NOTE(jesper): currently we don't assert about missing required
		// extensions or layers, and we don't store any internal state about
		// which ones we've enabled.
		uint32_t supported_layers_count = 0;
		result = vkEnumerateInstanceLayerProperties(&supported_layers_count,
		                                            nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkLayerProperties *supported_layers =
			(VkLayerProperties*) malloc(sizeof(VkLayerProperties) *
			                            supported_layers_count);
		result = vkEnumerateInstanceLayerProperties(&supported_layers_count,
		                                            supported_layers);
		DEBUG_ASSERT(result == VK_SUCCESS);

		uint32_t supported_extensions_count = 0;
		result =
			vkEnumerateInstanceExtensionProperties(nullptr,
			                                       &supported_extensions_count,
			                                       nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkExtensionProperties *supported_extensions =
			(VkExtensionProperties*) malloc(sizeof(VkExtensionProperties) *
			                                supported_extensions_count);
		result =
			vkEnumerateInstanceExtensionProperties(nullptr,
			                                       &supported_extensions_count,
			                                       supported_extensions);
		DEBUG_ASSERT(result == VK_SUCCESS);

		// NOTE(jesper): we might want to store these in the device for future
		// usage/debug information
		int32_t enabled_layers_count = 0;
		char **enabled_layers = (char**) malloc(sizeof(char*) *
		                                        supported_layers_count);

		int32_t enabled_extensions_count = 0;
		char **enabled_extensions = (char**) malloc(sizeof(char*) *
		                                            supported_extensions_count);

		for (int32_t i = 0; i < (int32_t)supported_layers_count; ++i)
		{
			VkLayerProperties &layer = supported_layers[i];

			if (platform_vulkan_enable_instance_layer(layer) ||
			    strcmp(layer.layerName,
			           "VK_LAYER_LUNARG_standard_validation") == 0)
			{
				enabled_layers[enabled_layers_count++] = layer.layerName;
			}
		}

		for (int32_t i = 0; i < (int32_t)supported_extensions_count; ++i)
		{
			VkExtensionProperties &extension = supported_extensions[i];

			if (platform_vulkan_enable_instance_extension(extension) ||
			    strcmp(extension.extensionName,
			           VK_KHR_SURFACE_EXTENSION_NAME) == 0 ||
			    strcmp(extension.extensionName,
			           VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
			{
				enabled_extensions[enabled_extensions_count++] =
					extension.extensionName;
			}
		}

		// Create the VkInstance
		VkApplicationInfo app_info = {};
		app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName   = "leary";
		app_info.applicationVersion = 1;
		app_info.pEngineName        = "leary";
		app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 22);

		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledLayerCount = (uint32_t) enabled_layers_count;
		create_info.ppEnabledLayerNames = enabled_layers;
		create_info.enabledExtensionCount = (uint32_t) enabled_extensions_count;
		create_info.ppEnabledExtensionNames = enabled_extensions;

		result = vkCreateInstance(&create_info, nullptr, &vk_instance);
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
		VkDebugReportCallbackEXT debug_callback;

		auto pfn_vkCreateDebugReportCallbackEXT =
			(PFN_vkCreateDebugReportCallbackEXT)
			vkGetInstanceProcAddr(vk_instance,
			                      "vkCreateDebugReportCallbackEXT");

		VkDebugReportCallbackCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

		create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
		                    VK_DEBUG_REPORT_WARNING_BIT_EXT |
		                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
		                    VK_DEBUG_REPORT_DEBUG_BIT_EXT;

		create_info.pfnCallback = &debug_callback_func;

		result = pfn_vkCreateDebugReportCallbackEXT(vk_instance,
		                                            &create_info,
		                                            nullptr,
		                                            &debug_callback);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}



	/**************************************************************************
	 * Create and choose VkPhysicalDevice
	 *************************************************************************/
	{
		uint32_t count = 0;
		result = vkEnumeratePhysicalDevices(vk_instance, &count, nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkPhysicalDevice *physical_devices = new VkPhysicalDevice[count];
		result = vkEnumeratePhysicalDevices(vk_instance,
		                                    &count, physical_devices);
		DEBUG_ASSERT(result == VK_SUCCESS);

		// TODO: choose device based on device type
		// (discrete > integrated > etc)
		vk_physical_device = physical_devices[0];

		vkGetPhysicalDeviceMemoryProperties(vk_physical_device,
		                                    &vk_physical_memory_properties);

		// NOTE: this works because VkPhysicalDevice is a handle to physical
		// device, not an actual data type, so we're just deleting the array
		// of handles
		delete[] physical_devices;
	}

	/**************************************************************************
	 * Create VkSurfaceKHR
	 *************************************************************************/
	{
		result = vulkan_create_surface(vk_instance, &vk_surface, platform_state);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	/**************************************************************************
	 * Create VkDevice and get its queue
	 *************************************************************************/
	{
		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device,
		                                         &queue_family_count,
		                                         nullptr);

		VkQueueFamilyProperties *queue_families =
			new VkQueueFamilyProperties[queue_family_count];
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device,
		                                         &queue_family_count,
		                                         queue_families);

		queue_family_index = 0;
		for (uint32_t i = 0; i < queue_family_count; ++i) {
			const VkQueueFamilyProperties &property = queue_families[i];

			// figure out if the queue family supports present
			VkBool32 supports_present = VK_FALSE;
			result = vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device,
			                                              queue_family_index,
			                                              vk_surface,
			                                              &supports_present);
			DEBUG_ASSERT(result == VK_SUCCESS);

			// if it doesn't we keep on searching
			if (supports_present == VK_FALSE)
				continue;

			DEBUG_LOGF(LogType::info,
			           "queueCount                 : %u",
			           property.queueCount);
			DEBUG_LOGF(LogType::info,
			           "timestampValidBits         : %u",
			           property.timestampValidBits);
			DEBUG_LOGF(LogType::info,
			           "minImageTransferGranualrity: (%u, %u, %u)",
			           property.minImageTransferGranularity.depth,
			           property.minImageTransferGranularity.height,
			           property.minImageTransferGranularity.depth);
			DEBUG_LOGF(LogType::info,
			           "supportsPresent            : %d",
			           static_cast<int32_t>(supports_present));

			// we're just interested in getting a graphics queue going for
			// now, so choose the first one
			// TODO: COMPUTE: find a compute queue, potentially asynchronous
			// (separate from graphics queue)
			// TODO: get a separate queue for transfer if one exist to do
			// buffer copy commands on while graphics/compute queue is doing
			// its own thing
			if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				queue_family_index = i;
				break;
			}
		}

		// TODO: when we have more than one queue we'll need to figure out how
		// to handle this, for now just set highest queue priroity for the 1
		// queue we create
		float priority = 1.0f;

		VkDeviceQueueCreateInfo queue_info = {};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = queue_family_index;
		queue_info.queueCount       = 1;
		queue_info.pQueuePriorities = &priority;

		// TODO: look into VkPhysicalDeviceFeatures and how it relates to
		// VkDeviceCreateInfo
		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(vk_physical_device,
		                            &physical_device_features);

		// TODO: look into other extensions
		const char *device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo device_info = {};
		device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_info.queueCreateInfoCount    = 1;
		device_info.pQueueCreateInfos       = &queue_info;
		device_info.enabledExtensionCount   = 1;
		device_info.ppEnabledExtensionNames = device_extensions;
		device_info.pEnabledFeatures        = &physical_device_features;

		result = vkCreateDevice(vk_physical_device,
		                        &device_info,
		                        nullptr,
		                        &vk_device);
		DEBUG_ASSERT(result == VK_SUCCESS);

		// NOTE: does it matter which queue we choose?
		uint32_t queue_index = 0;
		vkGetDeviceQueue(vk_device, queue_family_index, queue_index, &vk_queue);

		delete[] queue_families;
	}

	/**************************************************************************
	 * Create VkSwapchainKHR
	 *************************************************************************/
	{
		// figure out the color space for the swapchain
		uint32_t formats_count;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device,
		                                              vk_surface,
		                                              &formats_count,
		                                              nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkSurfaceFormatKHR *formats = new VkSurfaceFormatKHR[formats_count];
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device,
		                                              vk_surface,
		                                              &formats_count,
		                                              formats);
		DEBUG_ASSERT(result == VK_SUCCESS);

		// NOTE: if impl. reports only 1 surface format and that is undefined
		// it has no preferred format, so we choose BGRA8_UNORM
		if (formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			vk_surface_format = VK_FORMAT_B8G8R8A8_UNORM;
		else
			vk_surface_format = formats[0].format;

		// TODO: does the above note affect the color space at all?
		VkColorSpaceKHR surface_colorspace = formats[0].colorSpace;

		VkSurfaceCapabilitiesKHR surface_capabilities;
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device,
		                                                   vk_surface,
		                                                   &surface_capabilities);
		DEBUG_ASSERT(result == VK_SUCCESS);

		// figure out the present mode for the swapchain
		uint32_t present_modes_count;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device,
		                                                   vk_surface,
		                                                   &present_modes_count,
		                                                   nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkPresentModeKHR *present_modes =
			new VkPresentModeKHR[present_modes_count];
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device,
		                                                   vk_surface,
		                                                   &present_modes_count,
		                                                   present_modes);
		DEBUG_ASSERT(result == VK_SUCCESS);

		VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < present_modes_count; ++i) {
			const VkPresentModeKHR &mode = present_modes[i];

			if (settings.video.vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				surface_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}

			if (!settings.video.vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				surface_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				break;
			}
		}

		vk_swapchain_extent = surface_capabilities.currentExtent;
		if (vk_swapchain_extent.width == (uint32_t) (-1)) {
			// TODO(grouse): clean up usage of window dimensions
			DEBUG_ASSERT(settings.video.resolution.width  >= 0);
			DEBUG_ASSERT(settings.video.resolution.height >= 0);

			vk_swapchain_extent.width =
				(uint32_t)settings.video.resolution.width;
			vk_swapchain_extent.height =
				(uint32_t)settings.video.resolution.height;
		}

		VkSurfaceTransformFlagBitsKHR pre_transform =
			surface_capabilities.currentTransform;

		if (surface_capabilities.supportedTransforms &
		    VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}

		// TODO: determine the number of VkImages to use in the swapchain
		uint32_t desired_swapchain_images =
			surface_capabilities.minImageCount + 1;

		if (surface_capabilities.maxImageCount > 0)
		{
			desired_swapchain_images = MIN(desired_swapchain_images,
			                               surface_capabilities.maxImageCount);
		}

		VkSwapchainCreateInfoKHR create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface               = vk_surface;
		create_info.minImageCount         = desired_swapchain_images;
		create_info.imageFormat           = vk_surface_format;
		create_info.imageColorSpace       = surface_colorspace;
		create_info.imageExtent           = vk_swapchain_extent;
		create_info.imageArrayLayers      = 1;
		create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 1;
		create_info.pQueueFamilyIndices   = &queue_family_index;
		create_info.preTransform          = pre_transform;
		create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode           = surface_present_mode;
		create_info.clipped               = VK_TRUE;
		create_info.oldSwapchain          = VK_NULL_HANDLE;

		result = vkCreateSwapchainKHR(vk_device,
		                              &create_info,
		                              nullptr,
		                              &vk_swapchain);

		DEBUG_ASSERT(result == VK_SUCCESS);

		delete[] present_modes;
		delete[] formats;
	}

	/**************************************************************************
	 * Create Swapchain images and views
	 *************************************************************************/
	{
		result = vkGetSwapchainImagesKHR(vk_device,
		                                 vk_swapchain,
		                                 &swapchain_images_count,
		                                 nullptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		vk_swapchain_images = new VkImage[swapchain_images_count];
		result = vkGetSwapchainImagesKHR(vk_device,
		                                 vk_swapchain,
		                                 &swapchain_images_count,
		                                 vk_swapchain_images);
		DEBUG_ASSERT(result == VK_SUCCESS);

		vk_swapchain_imageviews = new VkImageView[swapchain_images_count];

		VkComponentMapping components = {};
		components.r = VK_COMPONENT_SWIZZLE_R;
		components.g = VK_COMPONENT_SWIZZLE_G;
		components.b = VK_COMPONENT_SWIZZLE_B;
		components.a = VK_COMPONENT_SWIZZLE_A;

		VkImageSubresourceRange subresource_range = {};
		subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource_range.baseMipLevel   = 0;
		subresource_range.levelCount     = 1;
		subresource_range.baseArrayLayer = 0;
		subresource_range.layerCount     = 1;

		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format           = vk_surface_format;
		create_info.components       = components;
		create_info.subresourceRange = subresource_range;

		for (int32_t i = 0; i < (int32_t) swapchain_images_count; ++i)
		{
			create_info.image = vk_swapchain_images[i];

			result = vkCreateImageView(vk_device, &create_info, nullptr,
			                           &vk_swapchain_imageviews[i]);
			DEBUG_ASSERT(result == VK_SUCCESS);
		}
	}

	/**************************************************************************
	 * Create VkCommandPool
	 *************************************************************************/
	{
		VkCommandPoolCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		create_info.queueFamilyIndex = queue_family_index;

		result = vkCreateCommandPool(vk_device,
		                             &create_info,
		                             nullptr,
		                             &vk_command_pool);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	/**************************************************************************
	 * Create VkCommandBuffer for initialisation and present
	 *************************************************************************/
	{
		// NOTE: we want to allocate all the command buffers we're going to need
		// in the game at once.
		VkCommandBufferAllocateInfo allocate_info = {};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandPool        = vk_command_pool;
		allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandBufferCount = 2;

		result = vkAllocateCommandBuffers(vk_device,
		                                  &allocate_info,
		                                  vk_cmd_buffers);
		DEBUG_ASSERT(result == VK_SUCCESS);

		vk_cmd_init    = vk_cmd_buffers[0];
		vk_cmd_present = vk_cmd_buffers[1];
	}


	/**************************************************************************
	 * Create Depth buffer
	 *************************************************************************/
	{
		vk_depth_image = create_image(VK_FORMAT_D16_UNORM,
		                              vk_swapchain_extent.width,
		                              vk_swapchain_extent.height,
		                              VK_IMAGE_TILING_OPTIMAL,
		                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		                              (VkMemoryPropertyFlags)0,
		                              &vk_depth_memory);

		transition_image(vk_depth_image,
		                 VK_IMAGE_LAYOUT_PREINITIALIZED,
		                 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// create image view
		VkComponentMapping components = {};
		components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		VkImageSubresourceRange subresource_range = {};
		subresource_range.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
		subresource_range.baseMipLevel   = 0;
		subresource_range.levelCount     = 1;
		subresource_range.baseArrayLayer = 0;
		subresource_range.layerCount     = 1;

		VkImageViewCreateInfo imageview_info = {};
		imageview_info.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageview_info.image            = vk_depth_image;
		imageview_info.viewType         = VK_IMAGE_VIEW_TYPE_2D;
		imageview_info.format           = VK_FORMAT_D16_UNORM;
		imageview_info.components       = components;
		imageview_info.subresourceRange = subresource_range;

		result = vkCreateImageView(vk_device,
		                           &imageview_info,
		                           nullptr,
		                           &vk_depth_imageview);
		DEBUG_ASSERT(result == VK_SUCCESS);

	}

	/**************************************************************************
	 * Create vkRenderPass
	 *************************************************************************/
	{
		VkImageLayout depth_image_layout =
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription color_attachment = {};
		color_attachment.format         = vk_surface_format;
		color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depth_stencil_attachment = {};
		depth_stencil_attachment.format  = VK_FORMAT_D16_UNORM;
		depth_stencil_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_stencil_attachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_stencil_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		depth_stencil_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_stencil_attachment.stencilStoreOp =
			VK_ATTACHMENT_STORE_OP_DONT_CARE;

		depth_stencil_attachment.initialLayout  = depth_image_layout;
		depth_stencil_attachment.finalLayout    = depth_image_layout;

		VkAttachmentDescription attachment_descriptions[2] = {
			color_attachment,
			depth_stencil_attachment
		};

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref = {};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout     = depth_image_layout;

		VkSubpassDescription subpass_description = {};
		subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass_description.inputAttachmentCount    = 0;
		subpass_description.pInputAttachments       = nullptr;
		subpass_description.colorAttachmentCount    = 1;
		subpass_description.pColorAttachments       = &color_attachment_ref;
		subpass_description.pResolveAttachments     = nullptr;
		subpass_description.pDepthStencilAttachment = &depth_attachment_ref;
		subpass_description.preserveAttachmentCount = 0;
		subpass_description.pPreserveAttachments    = nullptr;

		VkRenderPassCreateInfo create_info = {};
		create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		create_info.attachmentCount = 2;
		create_info.pAttachments    = attachment_descriptions;
		create_info.subpassCount    = 1;
		create_info.pSubpasses      = &subpass_description;
		create_info.dependencyCount = 0;
		create_info.pDependencies   = nullptr;

		result = vkCreateRenderPass(vk_device,
		                            &create_info,
		                            nullptr,
		                            &vk_renderpass);
		DEBUG_ASSERT(result == VK_SUCCESS);
	}

	/**************************************************************************
	 * Create Framebuffers
	 *************************************************************************/
	{
		framebuffers_count = (int32_t)swapchain_images_count;
		vk_framebuffers = (VkFramebuffer*) malloc(sizeof(VkFramebuffer) *
		                                          framebuffers_count);

		VkFramebufferCreateInfo create_info = {};
		create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		create_info.renderPass      = vk_renderpass;
		create_info.attachmentCount = 2;
		create_info.width           = vk_swapchain_extent.width;
		create_info.height          = vk_swapchain_extent.height;
		create_info.layers          = 1;

		for (int32_t i = 0; i < framebuffers_count; ++i)
		{
			VkImageView views[2] =
			{
				vk_swapchain_imageviews[i],
				vk_depth_imageview
			};

			create_info.pAttachments = views;

			result = vkCreateFramebuffer(vk_device,
			                             &create_info,
			                             nullptr,
			                             &vk_framebuffers[i]);
			DEBUG_ASSERT(result == VK_SUCCESS);
		}
	}

	/**************************************************************************
	 * Create Vertex buffer
	 *************************************************************************/
	vertex_buffer = create_vertex_buffer(sizeof(float) * 6 * NUM_DEMO_VERTICES,
	                                     (uint8_t*) vertices);

	/**************************************************************************
	 * Create Shader modules
	 *************************************************************************/
	{
		const char *vertex_file = FILE_SEP "shaders" FILE_SEP "triangle_vert.spv";
		size_t vertex_path_length = platform_state.folders.game_data_length +
		                            strlen(vertex_file) + 1;
		char *vertex_path = (char*) malloc(vertex_path_length);
		strcpy(vertex_path, platform_state.folders.game_data);
		strcat(vertex_path, vertex_file);

		size_t vertex_size;
		void *vertex_source = file_read(vertex_path, &vertex_size);
		DEBUG_ASSERT(vertex_source != nullptr);
		free(vertex_path);

		const char *fragment_file = FILE_SEP "shaders" FILE_SEP "triangle_frag.spv";
		size_t fragment_path_length = platform_state.folders.game_data_length +
		                              strlen(fragment_file) + 1;
		char *fragment_path = (char*) malloc(fragment_path_length);
		strcpy(fragment_path, platform_state.folders.game_data);
		strcat(fragment_path, fragment_file);

		size_t fragment_size;
		void *fragment_source = file_read(fragment_path, &fragment_size);
		DEBUG_ASSERT(fragment_source != nullptr);
		free(fragment_path);

		VkShaderModuleCreateInfo create_info = {};
		create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = vertex_size;
		create_info.pCode    = (uint32_t*)vertex_source;

		result = vkCreateShaderModule(vk_device,
		                              &create_info,
		                              nullptr,
		                              &vk_vertex_shader);
		DEBUG_ASSERT(result == VK_SUCCESS);

		create_info.codeSize = fragment_size;
		create_info.pCode    = (uint32_t*) fragment_source;

		result = vkCreateShaderModule(vk_device,
		                              &create_info,
		                              nullptr,
		                              &vk_fragment_shader);
		DEBUG_ASSERT(result == VK_SUCCESS);

		free(fragment_source);
		free(vertex_source);
	}

	/**************************************************************************
	 * Create pipeline
	 *************************************************************************/
	{
		VkPipelineLayoutCreateInfo layout_info = {};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount         = 0;
		layout_info.pSetLayouts            = nullptr;
		layout_info.pushConstantRangeCount = 0;
		layout_info.pPushConstantRanges    = nullptr;

		result = vkCreatePipelineLayout(vk_device,
		                                &layout_info,
		                                nullptr,
		                                &vk_pipeline_layout);
		DEBUG_ASSERT(result == VK_SUCCESS);


		VkPipelineShaderStageCreateInfo vertex_shader_stage = {};
		vertex_shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertex_shader_stage.stage               = VK_SHADER_STAGE_VERTEX_BIT;
		vertex_shader_stage.module              = vk_vertex_shader;
		vertex_shader_stage.pName               = "main";
		vertex_shader_stage.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragment_shader_stage = {};
		fragment_shader_stage.sType =
			VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragment_shader_stage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragment_shader_stage.module = vk_fragment_shader;
		fragment_shader_stage.pName = "main";
		fragment_shader_stage.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo shader_stage_info[2] = {
			vertex_shader_stage,
			fragment_shader_stage
		};

		VkVertexInputBindingDescription vertex_input_binding_desc = {};
		vertex_input_binding_desc.binding = VERTEX_INPUT_BINDING;
		vertex_input_binding_desc.stride  = sizeof(float) * 6;


		VkVertexInputAttributeDescription vertex_input_position = {};
		vertex_input_position.location = 0;
		vertex_input_position.binding  = VERTEX_INPUT_BINDING;
		vertex_input_position.format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_input_position.offset   = 0;

		VkVertexInputAttributeDescription vertex_input_color = {};
		vertex_input_color.location = 1;
		vertex_input_color.binding  = VERTEX_INPUT_BINDING;
		vertex_input_color.format   = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_input_color.offset   = sizeof(float) * 3;

		VkVertexInputAttributeDescription vertex_input_attributes[2] = {
			vertex_input_position,
			vertex_input_color
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType                           =
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions =
			&vertex_input_binding_desc;

		vertex_input_info.vertexAttributeDescriptionCount = 2;
		vertex_input_info.pVertexAttributeDescriptions =
			vertex_input_attributes;

		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
		input_assembly_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly_info.primitiveRestartEnable = VK_FALSE;

		VkDynamicState dynamic_states[2] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
		dynamic_state_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state_info.dynamicStateCount = 2;
		dynamic_state_info.pDynamicStates    = dynamic_states;

		VkPipelineViewportStateCreateInfo viewport_info = {};
		viewport_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = 1;
		viewport_info.pViewports    = nullptr;
		viewport_info.scissorCount  = 1;
		viewport_info.pScissors     = nullptr;

		VkPipelineRasterizationStateCreateInfo raster_info = {};
		raster_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		raster_info.depthClampEnable        = VK_FALSE;
		raster_info.rasterizerDiscardEnable = VK_FALSE;
		raster_info.polygonMode             = VK_POLYGON_MODE_FILL;
		raster_info.cullMode                = VK_CULL_MODE_BACK_BIT;
		raster_info.frontFace               = VK_FRONT_FACE_CLOCKWISE;
		raster_info.depthBiasEnable         = VK_FALSE;
		raster_info.depthBiasConstantFactor = 0.0f;
		raster_info.depthBiasClamp          = 0.0f;
		raster_info.depthBiasSlopeFactor    = 0.0f;
		raster_info.lineWidth               = 1.0;

		VkPipelineColorBlendAttachmentState color_blend_attachment = {};
		color_blend_attachment.blendEnable         = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
		color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
		                                             VK_COLOR_COMPONENT_G_BIT |
		                                             VK_COLOR_COMPONENT_B_BIT |
		                                             VK_COLOR_COMPONENT_A_BIT;

		float blend_constants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		VkPipelineColorBlendStateCreateInfo color_blend_info = {};
		color_blend_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_info.logicOpEnable   = VK_FALSE;
		color_blend_info.logicOp         = VK_LOGIC_OP_CLEAR;
		color_blend_info.attachmentCount = 1;
		color_blend_info.pAttachments    = &color_blend_attachment;

		memcpy(color_blend_info.blendConstants,
		       blend_constants,
		       sizeof(blend_constants));

		VkStencilOpState stencil_state = {};
		stencil_state.failOp      = VK_STENCIL_OP_KEEP;
		stencil_state.passOp      = VK_STENCIL_OP_KEEP;
		stencil_state.depthFailOp = VK_STENCIL_OP_KEEP;
		stencil_state.compareOp   = VK_COMPARE_OP_ALWAYS;
		stencil_state.compareMask = 0;
		stencil_state.writeMask   = 0;
		stencil_state.reference   = 0;

		VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
		depth_stencil_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_info.depthTestEnable = VK_TRUE;
		depth_stencil_info.depthWriteEnable = VK_TRUE;
		depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depth_stencil_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_info.stencilTestEnable = VK_FALSE;
		depth_stencil_info.front = stencil_state;
		depth_stencil_info.back = stencil_state;
		depth_stencil_info.minDepthBounds = 0.0f;
		depth_stencil_info.maxDepthBounds = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisample_info = {};
		multisample_info.sType =
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
		multisample_info.sampleShadingEnable   = VK_FALSE;
		multisample_info.minSampleShading      = 0;
		multisample_info.pSampleMask           = nullptr;
		multisample_info.alphaToCoverageEnable = VK_FALSE;
		multisample_info.alphaToOneEnable      = VK_FALSE;

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType =
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount          = 2;
		pipeline_info.pStages             = shader_stage_info;
		pipeline_info.pVertexInputState   = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly_info;
		pipeline_info.pViewportState      = &viewport_info;
		pipeline_info.pRasterizationState = &raster_info;
		pipeline_info.pMultisampleState   = &multisample_info;
		pipeline_info.pDepthStencilState  = &depth_stencil_info;
		pipeline_info.pColorBlendState    = &color_blend_info;
		pipeline_info.pDynamicState       = &dynamic_state_info;
		pipeline_info.layout              = vk_pipeline_layout;
		pipeline_info.renderPass          = vk_renderpass;
		pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex   = -1;

		result = vkCreateGraphicsPipelines(vk_device,
		                                   VK_NULL_HANDLE,
		                                   1,
		                                   &pipeline_info,
		                                   nullptr,
		                                   &vk_pipeline);
		DEBUG_ASSERT(result == VK_SUCCESS);

		vkDestroyShaderModule(vk_device, vk_vertex_shader,   nullptr);
		vkDestroyShaderModule(vk_device, vk_fragment_shader, nullptr);
	}
}

void
VulkanDevice::destroy()
{
	VkResult result;
	VAR_UNUSED(result);

	// wait for pending operations
	result = vkQueueWaitIdle(vk_queue);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkDestroyPipeline(vk_device, vk_pipeline, nullptr);
	vkDestroyPipelineLayout(vk_device, vk_pipeline_layout, nullptr);

	// TODO: move these calls out of VulkanDevice, they are meant to be
	// used outside as an api
	destroy_vertex_buffer(&vertex_buffer);

	for (int32_t i = 0; i < framebuffers_count; ++i) {
		vkDestroyFramebuffer(vk_device, vk_framebuffers[i], nullptr);
	}


	free(vk_framebuffers);

	vkDestroyRenderPass(vk_device, vk_renderpass, nullptr);

	vkDestroyImageView(vk_device, vk_depth_imageview, nullptr);
	vkFreeMemory(vk_device, vk_depth_memory, nullptr);
	vkDestroyImage(vk_device, vk_depth_image, nullptr);

	vkFreeCommandBuffers(vk_device, vk_command_pool, 2, vk_cmd_buffers);
	vkDestroyCommandPool(vk_device, vk_command_pool, nullptr);

	vkDestroySwapchainKHR(vk_device, vk_swapchain, nullptr);

	delete[] vk_swapchain_imageviews;
	delete[] vk_swapchain_images;

	vkDestroyDevice(vk_device,     nullptr);
	vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
	vkDestroyInstance(vk_instance, nullptr);
}

VulkanVertexBuffer
VulkanDevice::create_vertex_buffer(size_t size, uint8_t* data)
{
	VulkanVertexBuffer buffer;
	buffer.size = size;

	VkBufferCreateInfo create_info = {};
	create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.size                  = size;
	create_info.usage                 = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices   = nullptr;

	VkResult result = vkCreateBuffer(this->vk_device,
	                                 &create_info,
	                                 nullptr,
	                                 &buffer.vk_buffer);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// TODO: allocate buffers from large memory pool in VulkanDevice
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(this->vk_device,
	                              buffer.vk_buffer,
	                              &memory_requirements);

	uint32_t index = find_memory_type(memory_requirements.memoryTypeBits,
	                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	DEBUG_ASSERT(index != -1);

	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize  = memory_requirements.size;
	allocate_info.memoryTypeIndex = (uint32_t) index;

	result = vkAllocateMemory(this->vk_device,
	                          &allocate_info,
	                          nullptr,
	                          &buffer.vk_memory);
	DEBUG_ASSERT(result == VK_SUCCESS);

	result = vkBindBufferMemory(this->vk_device,
	                            buffer.vk_buffer,
	                            buffer.vk_memory, 0);
	DEBUG_ASSERT(result == VK_SUCCESS);

	if (data != nullptr)
	{
		void* memptr;
		result = vkMapMemory(this->vk_device,
		                     buffer.vk_memory,
		                     0,
		                     VK_WHOLE_SIZE,
		                     0,
		                     &memptr);
		DEBUG_ASSERT(result == VK_SUCCESS);

		memcpy(memptr, data, size);

		vkUnmapMemory(this->vk_device, buffer.vk_memory);
	}

	return buffer;
}

void
VulkanDevice::destroy_vertex_buffer(VulkanVertexBuffer* buffer)
{
	// TODO: free memory from large memory pool in VulkanDevice
	vkFreeMemory(this->vk_device,    buffer->vk_memory, nullptr);
	vkDestroyBuffer(this->vk_device, buffer->vk_buffer, nullptr);
}

void
VulkanDevice::present()
{
	VkResult result;

	VkSemaphore image_acquired, render_completed;

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result = vkCreateSemaphore(vk_device,
	                           &semaphore_info,
	                           nullptr,
	                           &image_acquired);
	DEBUG_ASSERT(result == VK_SUCCESS);

	result = vkCreateSemaphore(vk_device,
	                           &semaphore_info,
	                           nullptr,
	                           &render_completed);
	DEBUG_ASSERT(result == VK_SUCCESS);

	/**************************************************************************
	 * Acquire next available swapchain image
	 *************************************************************************/
	uint32_t image_index = std::numeric_limits<uint32_t>::max();
	uint64_t timeout     = std::numeric_limits<uint64_t>::max();

	result = vkAcquireNextImageKHR(vk_device,
	                               vk_swapchain,
	                               timeout,
	                               image_acquired,
	                               VK_NULL_HANDLE,
	                               &image_index);
	DEBUG_ASSERT(result == VK_SUCCESS);

	/**************************************************************************
	 * Fill present command buffer
	 *************************************************************************/
	VkCommandBufferBeginInfo present_begin_info = {};
	present_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	present_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	present_begin_info.pInheritanceInfo = nullptr;

	result = vkBeginCommandBuffer(vk_cmd_present, &present_begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	VkImageSubresourceRange subresource_range = {};
	subresource_range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresource_range.baseMipLevel   = 0;
	subresource_range.levelCount     = 1;
	subresource_range.baseArrayLayer = 0;
	subresource_range.layerCount     = 1;

	VkImageMemoryBarrier memory_barrier = {};
	memory_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	memory_barrier.srcAccessMask       = 0;
	memory_barrier.dstAccessMask       = 0;
	memory_barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	memory_barrier.newLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
	memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memory_barrier.image               = vk_swapchain_images[image_index];
	memory_barrier.subresourceRange    = subresource_range;

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	memory_barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	vkCmdPipelineBarrier(vk_cmd_present,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &memory_barrier);

	VkClearValue clear_values[2];
	clear_values[0].color.float32[0] = 0.3f;
	clear_values[0].color.float32[1] = 0.5f;
	clear_values[0].color.float32[2] = 0.9f;
	clear_values[0].color.float32[3] = 1.0f;

	clear_values[1].depthStencil  = { 1, 0 };

	VkRect2D render_area = {
		{ 0,       0        },
		{ vk_swapchain_extent.width, vk_swapchain_extent.height }
	};

	VkRenderPassBeginInfo render_pass_begin_info = {};
	render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.renderPass      = vk_renderpass;
	render_pass_begin_info.framebuffer     = vk_framebuffers[image_index];
	render_pass_begin_info.renderArea      = render_area;
	render_pass_begin_info.clearValueCount = 2;
	render_pass_begin_info.pClearValues    = clear_values;

	vkCmdBeginRenderPass(vk_cmd_present,
	                     &render_pass_begin_info,
	                     VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vk_cmd_present,
	                  VK_PIPELINE_BIND_POINT_GRAPHICS,
	                  vk_pipeline);

	VkViewport viewport = {};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = (float)vk_swapchain_extent.width;
	viewport.height   = (float)vk_swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(vk_cmd_present, 0, 1, &viewport);
	vkCmdSetScissor(vk_cmd_present, 0, 1, &render_area);

	VkDeviceSize buffer_offsets= 0;
	vkCmdBindVertexBuffers(vk_cmd_present, VERTEX_INPUT_BINDING,
	                       1, &vertex_buffer.vk_buffer, &buffer_offsets);

	vkCmdDraw(vk_cmd_present, 3, 1, 0, 0);

	vkCmdEndRenderPass(vk_cmd_present);

	result = vkEndCommandBuffer(vk_cmd_present);
	DEBUG_ASSERT(result == VK_SUCCESS);

	/**************************************************************************
	 * Submit Present command buffer to queue
	 *************************************************************************/
	VkPipelineStageFlags pipeline_stage_flags =
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = {};
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount   = 1;
	submit_info.pWaitSemaphores      = &image_acquired;
	submit_info.pWaitDstStageMask    = &pipeline_stage_flags;
	submit_info.commandBufferCount   = 1;
	submit_info.pCommandBuffers      = &vk_cmd_present;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores    = &render_completed;

	result = vkQueueSubmit(vk_queue, 1, &submit_info, VK_NULL_HANDLE);
	DEBUG_ASSERT(result == VK_SUCCESS);

	/**************************************************************************
	 * Present the rendered image
	 *************************************************************************/
	VkPresentInfoKHR present_info = {};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = &render_completed;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &vk_swapchain;
	present_info.pImageIndices      = &image_index;
	present_info.pResults           = nullptr;

	result = vkQueuePresentKHR(vk_queue, &present_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	// wait for idle operations
	result = vkQueueWaitIdle(vk_queue);
	DEBUG_ASSERT(result == VK_SUCCESS);

	vkDestroySemaphore(vk_device, render_completed, nullptr);
	vkDestroySemaphore(vk_device, image_acquired,  nullptr);
}

uint32_t VulkanDevice::find_memory_type(uint32_t filter,
                                        VkMemoryPropertyFlags req_flags)
{
	// TODO(jesper): do I need to query this or can I cache it?
	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &properties);

	for (uint32_t i = 0; i < properties.memoryTypeCount; i++) {
		VkMemoryPropertyFlags flags = properties.memoryTypes[i].propertyFlags;
		if ((filter & (1 << i)) &&
		    (flags & req_flags) == req_flags)
		{
			return i;
		}
	}

	return UINT32_MAX;
}
