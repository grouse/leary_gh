#include "vulkan_device.h"

#include <vector>
#include <limits>
#include <fstream>

#include <SDL_syswm.h>
#include <X11/Xlib-xcb.h>


#include "util/debug.h"
#include "platform/file.h"

#include "core/settings.h"

#include "render/game_window.h"

namespace
{
	int32_t 
	find_memory_type_index(const VkPhysicalDeviceMemoryProperties &properties, 
	                       uint32_t type_bits, 
	                       const VkMemoryPropertyFlags &req_properties); 
	constexpr int VERTEX_INPUT_BINDING = 0;	// The Vertex Input Binding for our vertex buffer.

	// Vertex data to draw.
	constexpr int NUM_DEMO_VERTICES = 3;

	const float vertices[NUM_DEMO_VERTICES * 6] =
	{
	    //      position             color
	    0.5f,  0.5f,  0.0f,  0.1f, 0.8f, 0.1f,
	    -0.5f,  0.5f,  0.0f,  0.8f, 0.1f, 0.1f,
	    0.0f, -0.5f,  0.0f,  0.1f, 0.1f, 0.8f,
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
	LEARY_UNUSED(type); 
	LEARY_UNUSED(src);
	LEARY_UNUSED(location);

	// NOTE: we never set any data when creating the callback, so useless for now
	LEARY_UNUSED(data);

	LogType log_type;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		log_type = LogType::error;
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		log_type = LogType::warning;
	else if (flags & (VK_DEBUG_REPORT_DEBUG_BIT_EXT | 
	                  VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
		log_type = LogType::info;
	else
		// NOTE: this would only happen if they extend the report callback flags
		log_type = LogType::info;
		
	LEARY_LOGF(log_type, "[VULKAN]: %s (%d) - %s", prefix, code, msg); 
	return false;
}

void 
VulkanDevice::create(const GameWindow& window)
{
	const Settings *settings = Settings::get();

	m_width  = settings->video.resolution.width;
	m_height = settings->video.resolution.height;

	/***********************************************************************************************
	 * Enumerate and validate supported extensions and layers
	 **********************************************************************************************/
	std::vector<const char*> layerNamesToEnable;
	layerNamesToEnable.push_back("VK_LAYER_LUNARG_standard_validation");

	std::vector<const char*> extensionNamesToEnable;
	extensionNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensionNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	extensionNamesToEnable.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);

	VkResult result;

	uint32_t supportedLayersCount = 0;
	result = vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkLayerProperties *supportedLayers = new VkLayerProperties[supportedLayersCount];
	result = vkEnumerateInstanceLayerProperties(&supportedLayersCount, supportedLayers);
	LEARY_ASSERT(result == VK_SUCCESS);

	uint32_t propertyCount = 0;

	// by passing a nullptr to the name parameter (first) we get the available supported extensions
	result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkExtensionProperties *supportedExtensions = new VkExtensionProperties[propertyCount];
	result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, supportedExtensions);
	LEARY_ASSERT(result == VK_SUCCESS);

	// Enumerate the layers we wanted to enable and the layers we've found supported and validate
	// that they are available
	for (const auto &layerName : layerNamesToEnable) {
		bool found = false;

		for (uint32_t i = 0; i < supportedLayersCount; ++i) {
			const auto &property = supportedLayers[i];

			if (strcmp(property.layerName, layerName) == 0) {
				found = true;
				break;
			}
		}

		LEARY_ASSERT(found);
	}

	// validate that they are available
	for (const auto &extensionName : extensionNamesToEnable) {
		bool found = false;

		for (uint32_t i = 0; i < propertyCount; ++i) {
			const auto &property = supportedExtensions[i];

			if (strcmp(property.extensionName, extensionName) == 0) {
				found = true;
				break;
			}
		}

		LEARY_ASSERT(found);
	}

	delete[] supportedLayers;
	delete[] supportedExtensions;

	/***********************************************************************************************
	 * Create VkInstance
	 * we've checked the available extensions and layers and can now create our instance
	 **********************************************************************************************/
	{
		VkApplicationInfo app_info = {
			.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pNext              = nullptr,
			.pApplicationName   = window.getTitle().c_str(),
			.applicationVersion = 1,
			.pEngineName        = window.getTitle().c_str(),
			.apiVersion         = VK_MAKE_VERSION(1, 0, 22),
		};

		VkInstanceCreateInfo create_info = {
			.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pNext                   = nullptr,
			.flags                   = 0,
			.pApplicationInfo        = &app_info,
			.enabledLayerCount       = (uint32_t) layerNamesToEnable.size(),
			.ppEnabledLayerNames     = layerNamesToEnable.data(),
			.enabledExtensionCount   = (uint32_t) extensionNamesToEnable.size(),
			.ppEnabledExtensionNames = extensionNamesToEnable.data(),
		};

		result = vkCreateInstance(&create_info, nullptr, &vk_instance);
		LEARY_ASSERT(result == VK_SUCCESS);
	}


	/***********************************************************************************************
	 * Create debug callbacks
	 **********************************************************************************************/
	{
		VkDebugReportCallbackEXT debug_callback;

		auto pfn_vkCreateDebugReportCallbackEXT = 
			(PFN_vkCreateDebugReportCallbackEXT) 
			vkGetInstanceProcAddr(vk_instance, "vkCreateDebugReportCallbackEXT");

		VkDebugReportCallbackCreateInfoEXT create_info = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
			.pNext = 0,

			.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | 
		                    VK_DEBUG_REPORT_WARNING_BIT_EXT | 
		                    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
		                    VK_DEBUG_REPORT_DEBUG_BIT_EXT,

			.pfnCallback = &debug_callback_func,
			.pUserData   = nullptr,
		};

		result = pfn_vkCreateDebugReportCallbackEXT(vk_instance, 
		                                            &create_info, 
		                                            nullptr, 
		                                            &debug_callback);
		LEARY_ASSERT(result == VK_SUCCESS);
	}



	/***********************************************************************************************
	 * Create and choose VkPhysicalDevice
	 **********************************************************************************************/
	{
		uint32_t count = 0;
		result = vkEnumeratePhysicalDevices(vk_instance, &count, nullptr);
		LEARY_ASSERT(result == VK_SUCCESS);

		VkPhysicalDevice *physical_devices = new VkPhysicalDevice[count];
		result = vkEnumeratePhysicalDevices(vk_instance, &count, physical_devices);
		LEARY_ASSERT(result == VK_SUCCESS);

		// TODO: choose device based on device type (discrete > integrated > etc)
		vk_physical_device = physical_devices[0];

		vkGetPhysicalDeviceMemoryProperties(vk_physical_device, &vk_physical_memory_properties);

		// NOTE: this works because VkPhysicalDevice is a handle to physical device, not an actual
		// data type, so we're just deleting the array of handles
		delete[] physical_devices;
	}

	/***********************************************************************************************
	 * Create VkSurfaceKHR
	 **********************************************************************************************/
	{
		SDL_SysWMinfo syswm;
		window.getSysWMInfo(&syswm);

		VkXcbSurfaceCreateInfoKHR create_info = {
			.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
			.pNext      = nullptr,
			.flags      = 0,

		// TODO: support wayland, mir and win32
			.connection = XGetXCBConnection(syswm.info.x11.display),
			.window     = (xcb_window_t) syswm.info.x11.window
		};

		result = vkCreateXcbSurfaceKHR(vk_instance, &create_info, nullptr, &vk_surface);
		LEARY_ASSERT(result == VK_SUCCESS);
	}

	/***********************************************************************************************
	 * Create VkDevice and get its queue
	 **********************************************************************************************/
	{
		uint32_t queue_family_count;
		vkGetPhysicalDeviceQueueFamilyProperties(vk_physical_device, &queue_family_count, nullptr); 

		VkQueueFamilyProperties *queue_families = new VkQueueFamilyProperties[queue_family_count];
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
			LEARY_ASSERT(result == VK_SUCCESS);

			// if it doesn't we keep on searching
			if (supports_present == VK_FALSE)
				continue;

			LEARY_LOGF(LogType::info, "queueCount                 : %u", property.queueCount);
			LEARY_LOGF(LogType::info, "timestampValidBits         : %u", property.timestampValidBits);
			LEARY_LOGF(LogType::info, "minImageTransferGranualrity: (%u, %u, %u)", 
			           property.minImageTransferGranularity.depth, 
			           property.minImageTransferGranularity.height, 
			           property.minImageTransferGranularity.depth);
			LEARY_LOGF(LogType::info, "supportsPresent            : %d",
			           static_cast<int32_t>(supports_present));

			// we're just interested in getting a graphics queue going for now, so choose the first
			// one
			// TODO: COMPUTE: find a compute queue, potentially asynchronous (separate from 
			// graphics queue)
			// TODO: get a separate queue for transfer if one exist to do buffer copy commands on 
			// while graphics/compute queue is doing its own thing
			if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				queue_family_index = i;
				break;
			}
		}

		// TODO: when we have more than one queue we'll need to figure out how to handle this, for 
		// now just set highest queue priroity for the 1 queue we create
		float priority = 1.0f;

		VkDeviceQueueCreateInfo queue_create_info = {
			.sType            = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
			.pNext            = nullptr,
			.flags            = 0,
			.queueFamilyIndex = queue_family_index,
			.queueCount       = 1,
			.pQueuePriorities = &priority,
		};

		// TODO: look into VkPhysicalDeviceFeatures and how it relates to VkDeviceCreateInfo
		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(vk_physical_device, &physical_device_features);

		// TODO: look into other extensions
		const char *device_extensions[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		VkDeviceCreateInfo device_create_info = {
			.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext                   = nullptr,
			.flags                   = 0,
			.queueCreateInfoCount    = 1,
			.pQueueCreateInfos       = &queue_create_info,
			.enabledLayerCount       = (uint32_t) layerNamesToEnable.size(),
			.ppEnabledLayerNames     = layerNamesToEnable.data(),
			.enabledExtensionCount   = 1,
			.ppEnabledExtensionNames = device_extensions,
			.pEnabledFeatures        = &physical_device_features,
		};

		result = vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_device);
		LEARY_ASSERT(result == VK_SUCCESS);

		// NOTE: does it matter which queue we choose?
		uint32_t queue_index = 0;
		vkGetDeviceQueue(vk_device, queue_family_index, queue_index, &vk_queue);

		delete[] queue_families;
	}

	/***********************************************************************************************
	 * Create VkSwapchainKHR
	 **********************************************************************************************/
	{
		// figure out the color space for the swapchain
		uint32_t formats_count; 
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, 
		                                              &formats_count, nullptr);
		LEARY_ASSERT(result == VK_SUCCESS);

		VkSurfaceFormatKHR *formats = new VkSurfaceFormatKHR[formats_count];
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(vk_physical_device, vk_surface, 
		                                              &formats_count, formats);
		LEARY_ASSERT(result == VK_SUCCESS);

		// NOTE: if impl. reports only 1 surface format and that is undefined it has no preferred 
		// format, so we choose BGRA8_UNORM
		if (formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			vk_surface_format = VK_FORMAT_B8G8R8A8_UNORM;
		else
			vk_surface_format = formats[0].format;

		// TODO: does the above note affect the color space at all?
		VkColorSpaceKHR surface_colorspace = formats[0].colorSpace;

		VkSurfaceCapabilitiesKHR surface_capabilities;
		result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device, vk_surface, 
		                                                   &surface_capabilities);
		LEARY_ASSERT(result == VK_SUCCESS);

		// figure out the present mode for the swapchain
		uint32_t present_modes_count;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface, 
		                                                   &present_modes_count, nullptr);
		LEARY_ASSERT(result == VK_SUCCESS);

		VkPresentModeKHR *present_modes = new VkPresentModeKHR[present_modes_count];
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device, vk_surface, 
		                                                   &present_modes_count, present_modes);
		LEARY_ASSERT(result == VK_SUCCESS);

		VkPresentModeKHR surface_present_mode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < present_modes_count; ++i) {
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

		vk_swapchain_extent = surface_capabilities.currentExtent;
		if (vk_swapchain_extent.width == (uint32_t) (-1)) {
			vk_swapchain_extent.width  = window.getWidth();
			vk_swapchain_extent.height = window.getHeight();
		} else {
			LEARY_ASSERT(vk_swapchain_extent.width  == window.getWidth());
			LEARY_ASSERT(vk_swapchain_extent.height == window.getHeight());
		}

		VkSurfaceTransformFlagBitsKHR pre_transform = surface_capabilities.currentTransform;
		if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

		// TODO: determine the number of VkImages to use in the swapchain
		uint32_t desired_swapchain_images = surface_capabilities.minImageCount + 1;
		if (surface_capabilities.maxImageCount > 0)
			desired_swapchain_images = std::min(desired_swapchain_images, 
			                                    surface_capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR create_info = {
			.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext                 = nullptr,
			.flags                 = 0,
			.surface               = vk_surface,
			.minImageCount         = desired_swapchain_images,
			.imageFormat           = vk_surface_format,
			.imageColorSpace       = surface_colorspace,
			.imageExtent           = vk_swapchain_extent,
			.imageArrayLayers      = 1,
			.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices   = &queue_family_index,
			.preTransform          = pre_transform,
			.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode           = surface_present_mode,
			.clipped               = VK_TRUE,
			.oldSwapchain          = VK_NULL_HANDLE
		};

		result = vkCreateSwapchainKHR(vk_device, &create_info, nullptr, &vk_swapchain);
		LEARY_ASSERT(result == VK_SUCCESS);

		delete[] present_modes;
		delete[] formats;
	}

	/***********************************************************************************************
	 * Create Swapchain images and views
	 **********************************************************************************************/
	{
		result = vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &swapchain_images_count, nullptr);
		LEARY_ASSERT(result == VK_SUCCESS);

		vk_swapchain_images = new VkImage[swapchain_images_count];
		result = vkGetSwapchainImagesKHR(vk_device, vk_swapchain, 
		                                 &swapchain_images_count, vk_swapchain_images);
		LEARY_ASSERT(result == VK_SUCCESS);

		vk_swapchain_imageviews = new VkImageView[swapchain_images_count];

		VkComponentMapping components = {
			.r = VK_COMPONENT_SWIZZLE_R,
			.g = VK_COMPONENT_SWIZZLE_G,
			.b = VK_COMPONENT_SWIZZLE_B,
			.a = VK_COMPONENT_SWIZZLE_A
		};

		VkImageSubresourceRange subresource_range = {
			.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		};

		VkImageViewCreateInfo create_info = {
			.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext            = nullptr,
			.flags            = 0,
			.viewType         = VK_IMAGE_VIEW_TYPE_2D,
			.format           = vk_surface_format,
			.components       = components,
			.subresourceRange = subresource_range
		};

		for (int32_t i = 0; i < (int32_t) swapchain_images_count; ++i) {
			create_info.image = vk_swapchain_images[i];

			result = vkCreateImageView(vk_device, &create_info, nullptr, 
			                           &vk_swapchain_imageviews[i]);
			LEARY_ASSERT(result == VK_SUCCESS);
		}
	}

	/***********************************************************************************************
	 * Create VkCommandPool
	 **********************************************************************************************/
	{
		VkCommandPoolCreateInfo create_info = {
			.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext            = nullptr,
			.flags            = 0,
			.queueFamilyIndex = queue_family_index
		};

		result = vkCreateCommandPool(vk_device, &create_info, nullptr, &vk_command_pool);
		LEARY_ASSERT(result == VK_SUCCESS);
	}

	/***********************************************************************************************
	 * Create VkCommandBuffer for initialisation and present
	 **********************************************************************************************/
	{
		// NOTE: we want to allocate all the command buffers we're going to need in the game at
		// once.
		VkCommandBufferAllocateInfo allocate_info = {
			.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext              = nullptr,
			.commandPool        = vk_command_pool,
			.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 2
		};

		result = vkAllocateCommandBuffers(vk_device, &allocate_info, vk_cmd_buffers);
		LEARY_ASSERT(result == VK_SUCCESS);

		vk_cmd_init    = vk_cmd_buffers[0];
		vk_cmd_present = vk_cmd_buffers[1];
	}


	/***********************************************************************************************
	 * Create Depth buffer
	 **********************************************************************************************/
	{
		VkExtent3D extent = { m_width, m_height, 1 };

		VkImageCreateInfo create_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_D16_UNORM,
			.extent = extent,
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,

			// NOTE: must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		result = vkCreateImage(vk_device, &create_info, nullptr, &vk_depth_image);
		LEARY_ASSERT(result == VK_SUCCESS);

		// create memory
		VkMemoryRequirements memory_requirements;
		vkGetImageMemoryRequirements(vk_device, vk_depth_image, &memory_requirements);

		// NOTE: look into memory property flags for depth buffer if/when we want to sample or
		// map it
		int32_t memory_type_index = find_memory_type_index(vk_physical_memory_properties, 
		                                                   memory_requirements.memoryTypeBits, 
		                                                   0);
		LEARY_ASSERT(memory_type_index >= 0);


		// TODO: move this allocation to allocate from large memory pool in VulkanDevice
		VkMemoryAllocateInfo allocate_info = {
			.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext           = nullptr,
			.allocationSize  = memory_requirements.size,
			.memoryTypeIndex = (uint32_t) memory_type_index
		};

		result = vkAllocateMemory(vk_device, &allocate_info, nullptr, &vk_depth_memory);
		LEARY_ASSERT(result == VK_SUCCESS);

		result = vkBindImageMemory(vk_device, vk_depth_image, vk_depth_memory, 0);
		LEARY_ASSERT(result == VK_SUCCESS);

		// create image view
		VkComponentMapping components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY
		};

		VkImageSubresourceRange subresource_range = {
			.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1
		};

		VkImageViewCreateInfo imageview_create_info = {
			.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext            = nullptr,
			.flags            = 0,
			.image            = vk_depth_image,
			.viewType    = VK_IMAGE_VIEW_TYPE_2D,
			.format           = VK_FORMAT_D16_UNORM,
			.components       = components,
			.subresourceRange = subresource_range
		};

		result = vkCreateImageView(vk_device, &imageview_create_info, nullptr, &vk_depth_imageview);
		LEARY_ASSERT(result == VK_SUCCESS);

		// transfer the depth buffer image layout to the correct type
		VkImageMemoryBarrier memory_barrier = {
			.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext               = nullptr,
			.srcAccessMask       = 0,
			.dstAccessMask       = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image               = vk_depth_image,
			.subresourceRange    = subresource_range
		};

		VkCommandBufferBeginInfo cmd_begin_info = {
			.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext            = nullptr,
			.flags            = 0,
			.pInheritanceInfo = nullptr
		};

		result = vkBeginCommandBuffer(vk_cmd_init, &cmd_begin_info);
		LEARY_ASSERT(result == VK_SUCCESS);

		vkCmdPipelineBarrier(vk_cmd_init, 
		                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
		                     0, 
		                     0, nullptr, 
		                     0, nullptr, 
		                     1, &memory_barrier);

		result = vkEndCommandBuffer(vk_cmd_init);
		LEARY_ASSERT(result == VK_SUCCESS);

		VkSubmitInfo submit_info = {
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &vk_cmd_init,

		// NOTE: we should probably add a semaphore here to wait on so that we don't begin
		// rendering until the initialisation command buffers are completed
			.signalSemaphoreCount = 0,
			.pSignalSemaphores = nullptr
		};

		result = vkQueueSubmit(vk_queue, 1, &submit_info, VK_NULL_HANDLE);
		LEARY_ASSERT(result == VK_SUCCESS);

		result = vkQueueWaitIdle(vk_queue);
		LEARY_ASSERT(result == VK_SUCCESS);
	}

	/***********************************************************************************************
	 * Create vkRenderPass
	 **********************************************************************************************/
	{
		VkImageLayout depth_image_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachment_descriptions[2] = {
			{
				// color attachment
				.flags          = 0,
				.format         = vk_surface_format,
				.samples        = VK_SAMPLE_COUNT_1_BIT,
				.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			},
			{
				// depth stencil attachment
				.flags          = 0,
				.format         = VK_FORMAT_D16_UNORM,
				.samples        = VK_SAMPLE_COUNT_1_BIT,
				.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout  = depth_image_layout,
				.finalLayout    = depth_image_layout
			}
		};

		VkAttachmentReference color_attachment_ref = {
			.attachment = 0,
			.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkAttachmentReference depth_attachment_ref = {
			.attachment = 1,
			.layout     = depth_image_layout
		};

		VkSubpassDescription subpass_description = {
			.flags                    = 0,
			.pipelineBindPoint        = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.inputAttachmentCount     = 0,
			.pInputAttachments        = nullptr,
			.colorAttachmentCount     = 1,
			.pColorAttachments        = &color_attachment_ref,
			.pResolveAttachments      = nullptr,
			.pDepthStencilAttachment  = &depth_attachment_ref,
			.preserveAttachmentCount  = 0,
			.pPreserveAttachments     = nullptr
		};
		
		VkRenderPassCreateInfo create_info = {
			.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext           = nullptr,
			.flags           = 0,
			.attachmentCount = 2,
			.pAttachments    = attachment_descriptions,
			.subpassCount    = 1,
			.pSubpasses      = &subpass_description,
			.dependencyCount = 0,
			.pDependencies   = nullptr
		};

		result = vkCreateRenderPass(vk_device, &create_info, nullptr, &vk_renderpass);
		LEARY_ASSERT(result == VK_SUCCESS);
	}

	/***********************************************************************************************
	 * Create Framebuffers
	 **********************************************************************************************/
	{
		framebuffers_count = swapchain_images_count;
		vk_framebuffers    = new VkFramebuffer[framebuffers_count];

		VkFramebufferCreateInfo create_info = {
			.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext           = nullptr,
			.flags           = 0,
			.renderPass      = vk_renderpass,
			.attachmentCount = 2,
			.width           = vk_swapchain_extent.width,
			.height          = vk_swapchain_extent.height,
			.layers          = 1
		};

		for (int32_t i = 0; i < framebuffers_count; ++i) {

			VkImageView views[2] = { 
				vk_swapchain_imageviews[i], 
				vk_depth_imageview 
			};

			create_info.pAttachments = views;

			result = vkCreateFramebuffer(vk_device, &create_info, nullptr, &vk_framebuffers[i]);
			LEARY_ASSERT(result == VK_SUCCESS);
		}
	}

	/***********************************************************************************************
	 * Create Vertex buffer
	 **********************************************************************************************/
	vertex_buffer = create_vertex_buffer(sizeof(float) * 6 * NUM_DEMO_VERTICES, 
	                                     (uint8_t*) vertices);

	/***********************************************************************************************
	 * Create Shader modules
	 **********************************************************************************************/
	{
		// TODO: nuke std::string, and look into nuking std::ifstream
		std::string folder = resolve_path(EnvironmentFolder::GameData, "shaders/");

		std::ifstream file;
		file.open(folder + "vert.spv", std::ios_base::binary | std::ios_base::ate);

		size_t vertex_size    = file.tellg();
		char   *vertex_source = new char[vertex_size];

		file.seekg(0, std::ios::beg);
		file.read(vertex_source, vertex_size);

		file.close();


		file.open(folder + "frag.spv", std::ios_base::binary | std::ios_base::ate);

		size_t fragment_size    = file.tellg();
		char   *fragment_source = new char[fragment_size];

		file.seekg(0, std::ios::beg);
		file.read(fragment_source, fragment_size);

		file.close();

		VkShaderModuleCreateInfo create_info = {
			.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext    = nullptr,
			.flags    = 0,
			.codeSize = vertex_size,
			.pCode    = (uint32_t*) vertex_source
		};

		result = vkCreateShaderModule(vk_device, &create_info, nullptr, &vk_vertex_shader);
		LEARY_ASSERT(result == VK_SUCCESS);
		
		create_info.codeSize = fragment_size;
		create_info.pCode    = (uint32_t*) fragment_source;

		result = vkCreateShaderModule(vk_device, &create_info, nullptr, &vk_fragment_shader);
		LEARY_ASSERT(result == VK_SUCCESS);

		delete[] fragment_source;
		delete[] vertex_source;
	}

	/***********************************************************************************************
	 * Create pipeline
	 **********************************************************************************************/
	{
		VkPipelineLayoutCreateInfo layout_create_info = {
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext                  = nullptr,
			.flags                  = 0,
			.setLayoutCount         = 0,
			.pSetLayouts            = nullptr,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges    = nullptr
		};

		result = vkCreatePipelineLayout(vk_device, 
		                                &layout_create_info, 
		                                nullptr, 
		                                &vk_pipeline_layout);
		LEARY_ASSERT(result == VK_SUCCESS);

		VkPipelineShaderStageCreateInfo shader_stage_create_info[2] = {
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = vk_vertex_shader,
				.pName  = "main",
				.pSpecializationInfo = nullptr
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = vk_fragment_shader,
				.pName  = "main",
				.pSpecializationInfo = nullptr
			}
		};

		VkVertexInputBindingDescription vertex_input_binding_desc = {
			.binding = VERTEX_INPUT_BINDING,
			.stride  = sizeof(float) * 6,
		};

		VkVertexInputAttributeDescription vertex_input_attribute_desc[2] = {
			{
				.location = 0,
				.binding  = VERTEX_INPUT_BINDING,
				.format   = VK_FORMAT_R32G32B32_SFLOAT,
				.offset   = 0
			},
			{
				.location = 1,
				.binding  = VERTEX_INPUT_BINDING,
				.format   = VK_FORMAT_R32G32B32_SFLOAT,
				.offset   = sizeof(float) * 3
			}
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertex_input_binding_desc,
			.vertexAttributeDescriptionCount = 2,
			.pVertexAttributeDescriptions = vertex_input_attribute_desc
		};

		VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		VkDynamicState dynamic_states[2] = { 
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamic_states
		};

		VkPipelineViewportStateCreateInfo viewport_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports    = nullptr,
			.scissorCount  = 1,
			.pScissors     = nullptr
		};

		VkPipelineRasterizationStateCreateInfo raster_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable        = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode             = VK_POLYGON_MODE_FILL,
			.cullMode                = VK_CULL_MODE_BACK_BIT,
			.frontFace               = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable         = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp          = 0.0f,
			.depthBiasSlopeFactor    = 0.0f,
			.lineWidth               = 1.0f
		};

		VkPipelineColorBlendAttachmentState color_blend_attachment = {
			.blendEnable         = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp        = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp        = VK_BLEND_OP_ADD,
			.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT |
			                       VK_COLOR_COMPONENT_G_BIT | 
			                       VK_COLOR_COMPONENT_B_BIT | 
			                       VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable   = VK_FALSE,
			.logicOp         = VK_LOGIC_OP_CLEAR,
			.attachmentCount = 1,
			.pAttachments    = &color_blend_attachment,
			.blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f }
		};

		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable       = VK_TRUE,
			.depthWriteEnable      = VK_TRUE,
			.depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable     = VK_FALSE,
			.front = { 
				.failOp      = VK_STENCIL_OP_KEEP, 
				.passOp      = VK_STENCIL_OP_KEEP, 
				.depthFailOp = VK_STENCIL_OP_KEEP, 
				.compareOp   = VK_COMPARE_OP_ALWAYS, 
				.compareMask = 0, 
				.writeMask   = 0, 
				.reference   = 0 
			},
			.back = { 
				.failOp      = VK_STENCIL_OP_KEEP, 
				.passOp      = VK_STENCIL_OP_KEEP, 
				.depthFailOp = VK_STENCIL_OP_KEEP, 
				.compareOp   = VK_COMPARE_OP_ALWAYS, 
				.compareMask = 0, 
				.writeMask   = 0, 
				.reference   = 0 
			},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 0.0f
		};
		
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable   = VK_FALSE,
			.minSampleShading      = 0,
			.pSampleMask           = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable      = VK_FALSE
		};

		VkGraphicsPipelineCreateInfo pipeline_create_info = {
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = 2,
			.pStages = shader_stage_create_info,
			.pVertexInputState = &vertex_input_create_info,
			.pInputAssemblyState = &input_assembly_create_info,
			.pTessellationState = nullptr,
			.pViewportState = &viewport_create_info,
			.pRasterizationState = &raster_create_info,
			.pMultisampleState = &multisample_create_info,
			.pDepthStencilState = &depth_stencil_create_info,
			.pColorBlendState = &color_blend_create_info,
			.pDynamicState = &dynamic_state_create_info,
			.layout = vk_pipeline_layout,
			.renderPass = vk_renderpass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};
			

		result = vkCreateGraphicsPipelines(vk_device, 
		                                   VK_NULL_HANDLE, 
		                                   1, 
		                                   &pipeline_create_info, 
		                                   nullptr, 
		                                   &vk_pipeline);
		LEARY_ASSERT(result == VK_SUCCESS);

		vkDestroyShaderModule(vk_device, vk_vertex_shader,   nullptr);
		vkDestroyShaderModule(vk_device, vk_fragment_shader, nullptr);
	}
}

void 
VulkanDevice::destroy()
{
	// wait for pending operations
	VkResult result = vkQueueWaitIdle(vk_queue);
	LEARY_ASSERT(result == VK_SUCCESS);

	vkDestroyPipeline(vk_device, vk_pipeline, nullptr);
	vkDestroyPipelineLayout(vk_device, vk_pipeline_layout, nullptr);

	// TODO: move these calls out of VulkanDevice, they are meant to be used outside as an api
	destroy_vertex_buffer(&vertex_buffer);

	for (int32_t i = 0; i < framebuffers_count; ++i) {
		vkDestroyFramebuffer(vk_device, vk_framebuffers[i], nullptr);
	}

	delete[] vk_framebuffers;

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

	VkBufferCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size  = size,
		.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices   = nullptr
	};

	VkResult result = vkCreateBuffer(this->vk_device, &create_info, nullptr, &buffer.vk_buffer);
	LEARY_ASSERT(result == VK_SUCCESS);

	// TODO: allocate buffers from large memory pool in VulkanDevice
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(this->vk_device, buffer.vk_buffer, &memory_requirements);

	int32_t index = find_memory_type_index(this->vk_physical_memory_properties, 
	                                       memory_requirements.memoryTypeBits, 
	                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	LEARY_ASSERT(index >= 0); 

	VkMemoryAllocateInfo allocate_info = {
		.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext           = nullptr,
		.allocationSize  = memory_requirements.size,
		.memoryTypeIndex = (uint32_t) index
	};

	result = vkAllocateMemory(this->vk_device, &allocate_info, nullptr, &buffer.vk_memory);
	LEARY_ASSERT(result == VK_SUCCESS);

	result = vkBindBufferMemory(this->vk_device, buffer.vk_buffer, buffer.vk_memory, 0);
	LEARY_ASSERT(result == VK_SUCCESS);

	if (data != nullptr)
	{
		void* memptr;
		result = vkMapMemory(this->vk_device, buffer.vk_memory, 0, VK_WHOLE_SIZE, 0, &memptr);
		LEARY_ASSERT(result == VK_SUCCESS);

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

	VkSemaphoreCreateInfo sem_create_info = { 
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 
		.pNext = nullptr, 
		.flags = 0 
	};

	result = vkCreateSemaphore(vk_device, &sem_create_info, nullptr, &image_acquired);
	LEARY_ASSERT(result == VK_SUCCESS);

	result = vkCreateSemaphore(vk_device, &sem_create_info, nullptr, &render_completed);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Acquire next available swapchain image
	 **********************************************************************************************/
	uint32_t image_index = std::numeric_limits<uint32_t>::max();
	uint64_t timeout     = std::numeric_limits<uint64_t>::max();

	result = vkAcquireNextImageKHR(vk_device,
	                               vk_swapchain,
	                               timeout,
	                               image_acquired,
	                               VK_NULL_HANDLE,
	                               &image_index);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Fill present command buffer
	 **********************************************************************************************/
	VkCommandBufferBeginInfo present_begin_info = { 
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, 
		.pNext = nullptr, 
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, 
		.pInheritanceInfo = nullptr 
	};

	result = vkBeginCommandBuffer(vk_cmd_present, &present_begin_info);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkImageSubresourceRange subresource_range = { 
		.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, 
		.baseMipLevel   = 0, 
		.levelCount     = 1, 
		.baseArrayLayer = 0, 
		.layerCount     = 1 
	};

	VkImageMemoryBarrier memory_barrier = { 
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, 
		.pNext = nullptr, 
		.srcAccessMask = 0, 
		.dstAccessMask = 0, 
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
		.newLayout = VK_IMAGE_LAYOUT_UNDEFINED, 
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, 
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, 
		.image               = vk_swapchain_images[image_index], 
		.subresourceRange    = subresource_range
	};

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
		{ m_width, m_height } 
	};

	VkRenderPassBeginInfo render_pass_begin_info = { 
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, 
		.pNext = nullptr, 
		.renderPass  = vk_renderpass, 
		.framebuffer = vk_framebuffers[image_index], 
		.renderArea  = render_area, 
		.clearValueCount = 2, 
		.pClearValues    = clear_values
	};

	vkCmdBeginRenderPass(vk_cmd_present, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vk_cmd_present, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

	VkViewport viewport = { 
		.x = 0.0f,    
		.y = 0.0f, 
		.width  = static_cast<float>(m_width), 
		.height = static_cast<float>(m_height), 
		.minDepth = 0.0f,    
		.maxDepth = 1.0f 
	};

	vkCmdSetViewport(vk_cmd_present, 0, 1, &viewport);

	vkCmdSetScissor(vk_cmd_present, 0, 1, &render_area);

	VkDeviceSize buffer_offsets= 0;
	vkCmdBindVertexBuffers(vk_cmd_present, VERTEX_INPUT_BINDING,
	                       1, &vertex_buffer.vk_buffer, &buffer_offsets);

	vkCmdDraw(vk_cmd_present, 3, 1, 0, 0);

	vkCmdEndRenderPass(vk_cmd_present);

	result = vkEndCommandBuffer(vk_cmd_present);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Submit Present command buffer to queue
	 **********************************************************************************************/
	VkPipelineStageFlags pipeline_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submit_info = { 
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, 
		.pNext = nullptr, 
		.waitSemaphoreCount   = 1, 
		.pWaitSemaphores      = &image_acquired, 
		.pWaitDstStageMask    = &pipeline_stage_flags, 
		.commandBufferCount   = 1, 
		.pCommandBuffers      = &vk_cmd_present, 
		.signalSemaphoreCount = 1, 
		.pSignalSemaphores    = &render_completed
	};

	result = vkQueueSubmit(vk_queue, 1, &submit_info, VK_NULL_HANDLE);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Present the rendered image
	 **********************************************************************************************/
	VkPresentInfoKHR present_info = { 
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, 
		.pNext = nullptr, 
		.waitSemaphoreCount = 1, 
		.pWaitSemaphores    = &render_completed,
		.swapchainCount = 1, 
		.pSwapchains    = &vk_swapchain, 
		.pImageIndices  = &image_index, 
		.pResults       = nullptr 
	};

	result = vkQueuePresentKHR(vk_queue, &present_info);
	LEARY_ASSERT(result == VK_SUCCESS);

	// wait for idle operations
	result = vkQueueWaitIdle(vk_queue);
	LEARY_ASSERT(result == VK_SUCCESS);

	vkDestroySemaphore(vk_device, render_completed, nullptr);
	vkDestroySemaphore(vk_device, image_acquired,  nullptr);
}

namespace
{
	int32_t 
	find_memory_type_index(const VkPhysicalDeviceMemoryProperties &properties, 
	                       uint32_t type_bits, 
	                       const VkMemoryPropertyFlags &req_properties)
	{
		for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
			if ((type_bits & (1 << i)) &&
			    ((properties.memoryTypes[i].propertyFlags & req_properties) == req_properties))
				return i;
		}

		return -1;
	}
}
