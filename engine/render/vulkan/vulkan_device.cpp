#include "vulkan_device.h"

#include <vector>
#include <limits>
#include <fstream>

#include <SDL_syswm.h>
#include <X11/Xlib-xcb.h>


#include "util/debug.h"
#include "util/environment.h"

#include "core/settings.h"

#include "render/game_window.h"

namespace
{
	int32_t find_memory_type_index(const VkPhysicalDeviceMemoryProperties properties,
	                               const uint32_t                         type_bits,
	                               const VkMemoryPropertyFlags            req_properties);

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
debug_callback_func(VkFlags flags, 
               VkDebugReportObjectTypeEXT type, 
               uint64_t src, 
               size_t location,
               int32_t code, 
               const char* prefix, 
               const char* msg, 
               void* data)
{
	// NOTE: these might be useful?
	LEARY_UNUSED(type); 
	LEARY_UNUSED(src);
	LEARY_UNUSED(location);

	// NOTE: we never set any data when creating the callback, so useless for now
	LEARY_UNUSED(data);

	eLogType log_type;
	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		log_type = eLogType::Error;
	else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
		log_type = eLogType::Warning;
	else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ||
	         flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		log_type = eLogType::Info;
	else
		// NOTE: this would only happen if they extend the report callback flags
		log_type = eLogType::Info;
		
	LEARY_LOGF(log_type, "[VULKAN]: %s (%d) - %s", prefix, code, msg); 
	return false;
}

void VulkanDevice::create(const GameWindow& window)
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
	const VkApplicationInfo applicationInfo = {
	    VK_STRUCTURE_TYPE_APPLICATION_INFO,
	    nullptr,
	    window.getTitle().c_str(), 1,
	    window.getTitle().c_str(), 1,
	    VK_MAKE_VERSION(1, 0, 22)
	};

	const VkInstanceCreateInfo instanceCreateInfo = {
	    VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	    nullptr,
	    0,
	    &applicationInfo,
	    static_cast<uint32_t>(layerNamesToEnable.size()),     layerNamesToEnable.data(),
	    static_cast<uint32_t>(extensionNamesToEnable.size()), extensionNamesToEnable.data()
	};

	result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
	LEARY_ASSERT(result == VK_SUCCESS);


	/***********************************************************************************************
	 * Create debug callbacks
	 **********************************************************************************************/
	VkDebugReportCallbackEXT debug_callback;

	auto pfn_vkCreateDebugReportCallbackEXT = 
		(PFN_vkCreateDebugReportCallbackEXT)(vkGetInstanceProcAddr(m_instance, 
																   "vkCreateDebugReportCallbackEXT"));
	VkDebugReportCallbackCreateInfoEXT debug_create_info;
	debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debug_create_info.pNext = 0;
	debug_create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
	                          VK_DEBUG_REPORT_WARNING_BIT_EXT |
	                          VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
	                          VK_DEBUG_REPORT_DEBUG_BIT_EXT;
	debug_create_info.pfnCallback = &debug_callback_func;
	debug_create_info.pUserData   = nullptr;

	result = pfn_vkCreateDebugReportCallbackEXT(m_instance, 
	                                            &debug_create_info, 
	                                            nullptr, 
	                                            &debug_callback);
	LEARY_ASSERT(result == VK_SUCCESS);



	/***********************************************************************************************
	 * Create and choose VkPhysicalDevice
	 **********************************************************************************************/
	uint32_t physicalDevicesCount = 0;
	result = vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, nullptr);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[physicalDevicesCount];
	result = vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, physicalDevices);
	LEARY_ASSERT(result == VK_SUCCESS);

	// @TODO: choose device based on device type (discrete > integrated > etc)
	m_physicalDevice = physicalDevices[0];

	delete[] physicalDevices;

	/***********************************************************************************************
	 * Create VkSurfaceKHR
	 **********************************************************************************************/
	SDL_SysWMinfo syswm;
	window.getSysWMInfo(&syswm);

	// @TODO: support other windowing systems, e.g. Windows, Wayland, Mir
	xcb_connection_t       *xcbConnection = XGetXCBConnection(syswm.info.x11.display);
	const xcb_window_t      xcbWindow     = syswm.info.x11.window;

	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
	    VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
	    nullptr,
	    0,
	    xcbConnection,
	    xcbWindow
	};

	result = vkCreateXcbSurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Create VkDevice
	 **********************************************************************************************/
	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice,
	                                         &queueFamilyCount, nullptr);

	VkQueueFamilyProperties *queueFamilies = new VkQueueFamilyProperties[queueFamilyCount];
	vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice,
	                                         &queueFamilyCount, queueFamilies);

	m_queueFamilyIndex = 0;
	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		const VkQueueFamilyProperties &property = queueFamilies[i];

		// figure out if the queue family supports present
		VkBool32 supportsPresent = VK_FALSE;
		result = vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_queueFamilyIndex,
		                                              m_surface, &supportsPresent);
		LEARY_ASSERT(result == VK_SUCCESS);

		// if it doesn't we keep on searching
		if (supportsPresent == VK_FALSE)
			continue;

		LEARY_LOGF(eLogType::Info, "queueCount                 : %u", property.queueCount);
		LEARY_LOGF(eLogType::Info, "timestampValidBits         : %u", property.timestampValidBits);
		LEARY_LOGF(eLogType::Info, "minImageTransferGranualrity: (%u, %u, %u)",
		           property.minImageTransferGranularity.depth,
		           property.minImageTransferGranularity.height,
		           property.minImageTransferGranularity.depth);
		LEARY_LOGF(eLogType::Info, "supportsPresent            : %d",
		           static_cast<int32_t>(supportsPresent));

		// we're just interested in getting a graphics queue going for now, so choose the first one
		// @TODO: @COMPUTE: find a compute queue, potentially asynchronous (separate from graphics
		//                  queue)
		// @TODO: get a separate queue for transfer if one exist to do buffer copy commands on while
		// graphics/compute queue is doing its own thing
		if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			m_queueFamilyIndex = i;
			break;
		}
	}

	// @TODO: when we have more than one queue we'll need to figure out how to handle this, for now
	// just set highest queue priroity for the 1 queue we create
	float queuePriority = 1.0f;
	const VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
	    VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
	    nullptr,
	    0,
	    m_queueFamilyIndex,
	    1, &queuePriority
	};

	// @TODO: look into VkPhysicalDeviceFeatures and how it relates to VkDeviceCreateInfo
	VkPhysicalDeviceFeatures physicalDeviceFeatures;
	vkGetPhysicalDeviceFeatures(m_physicalDevice, &physicalDeviceFeatures);

	const char* deviceExtensionNamesToEnable[1] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	const VkDeviceCreateInfo deviceCreateInfo = {
	    VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    nullptr,
	    0,
	    1, &deviceQueueCreateInfo,
	    static_cast<uint32_t>(layerNamesToEnable.size()),     layerNamesToEnable.data(),
	    1, deviceExtensionNamesToEnable,
	    &physicalDeviceFeatures
	};

	result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
	LEARY_ASSERT(result == VK_SUCCESS);

	uint32_t queueIndex = 0; // @TODO: does it matter which queue we choose?
	vkGetDeviceQueue(m_device, m_queueFamilyIndex, queueIndex, &m_queue);

	delete[] queueFamilies;

	/***********************************************************************************************
	 * Create VkSwapchainKHR
	 **********************************************************************************************/
	uint32_t surfaceFormatsCount;
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
	                                              &surfaceFormatsCount, nullptr);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkSurfaceFormatKHR *surfaceFormats = new VkSurfaceFormatKHR[surfaceFormatsCount];
	result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface,
	                                              &surfaceFormatsCount, surfaceFormats);
	LEARY_ASSERT(result == VK_SUCCESS);

	// if impl. reports only 1 surface format and that is undefined it has no preferred format, so
	// we choose BGRA8_UNORM
	if (surfaceFormatsCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
		m_surfaceFormat = VK_FORMAT_B8G8R8A8_UNORM;
	else
		m_surfaceFormat = surfaceFormats[0].format;

	const VkColorSpaceKHR                surfaceColorSpace  = surfaceFormats[0].colorSpace;

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface,
	                                                   &surfaceCapabilities);
	LEARY_ASSERT(result == VK_SUCCESS);


	const uint32_t                       imageArraylayers   = 1;
	const VkImageUsageFlags              imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	const VkSharingMode                  imageSharingMode   = VK_SHARING_MODE_EXCLUSIVE;
	const uint32_t                       queueFamilyIndexCount = 1;
	const uint32_t                      *queueFamilyIndices = &m_queueFamilyIndex;
	const VkCompositeAlphaFlagBitsKHR    compositeAlpha     = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	uint32_t presentModesCount;
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface,
	                                                   &presentModesCount, nullptr);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkPresentModeKHR *presentModes = new VkPresentModeKHR[presentModesCount];
	result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface,
	                                                   &presentModesCount, presentModes);
	LEARY_ASSERT(result == VK_SUCCESS);

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < presentModesCount; ++i) {
		const VkPresentModeKHR &mode = presentModes[i];

		if (settings->video.vsync && mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}

		if (!settings->video.vsync && mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
			break;
		}
	}

	VkExtent2D imageExtent = surfaceCapabilities.currentExtent;
	if (imageExtent.width == (uint32_t)(-1)) {
		imageExtent.width  = window.getWidth();
		imageExtent.height = window.getHeight();
	} else {
		LEARY_ASSERT(imageExtent.width  == window.getWidth());
		LEARY_ASSERT(imageExtent.height == window.getHeight());
	}

	VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;


	// determine the number of VkImages to use in the swapchain
	uint32_t desiredSwapchainImages = surfaceCapabilities.minImageCount + 1;
	if (surfaceCapabilities.maxImageCount > 0)
		desiredSwapchainImages = std::min(desiredSwapchainImages, surfaceCapabilities.maxImageCount);


	const VkBool32                       clipped            = VK_TRUE;


	const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
	    VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
	    nullptr,
	    0,
	    m_surface,
	    desiredSwapchainImages,
	    m_surfaceFormat,
	    surfaceColorSpace,
	    imageExtent,
	    imageArraylayers,
	    imageUsage,
	    imageSharingMode,
	    queueFamilyIndexCount,
	    queueFamilyIndices,
	    preTransform,
	    compositeAlpha,
	    presentMode,
	    clipped,
	    VK_NULL_HANDLE
	};

	result = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain);
	LEARY_ASSERT(result == VK_SUCCESS);

	delete[] presentModes;

	/***********************************************************************************************
	 * Create Swapchain images and views
	 **********************************************************************************************/
	result = vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImagesCount, nullptr);
	LEARY_ASSERT(result == VK_SUCCESS);

	m_swapchainImages = new VkImage[m_swapchainImagesCount];
	result = vkGetSwapchainImagesKHR(m_device, m_swapchain,
	                                 &m_swapchainImagesCount, m_swapchainImages);
	LEARY_ASSERT(result == VK_SUCCESS);

	m_swapchainImageViews = new VkImageView[m_swapchainImagesCount];
	for (uint32_t i = 0; i < m_swapchainImagesCount; ++i) {
		const VkComponentMapping components = {
		    VK_COMPONENT_SWIZZLE_R,
		    VK_COMPONENT_SWIZZLE_G,
		    VK_COMPONENT_SWIZZLE_B,
		    VK_COMPONENT_SWIZZLE_A,
		};

		const VkImageSubresourceRange subresourceRange = {
		    VK_IMAGE_ASPECT_COLOR_BIT,
		    0, 1,
		    0, 1
		};

		const VkImageViewCreateInfo createInfo = {
		    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		    nullptr,
		    0,
		    m_swapchainImages[i],
		    VK_IMAGE_VIEW_TYPE_2D,
		    m_surfaceFormat,
		    components,
		    subresourceRange
		};

		result = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]);
		LEARY_ASSERT(result == VK_SUCCESS);
	}

	/***********************************************************************************************
	 * Create VkCommandPool
	 **********************************************************************************************/
	const VkCommandPoolCreateInfo cmdPoolCreateInfo = {
	    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
	    nullptr,
	    0,
	    m_queueFamilyIndex
	};

	result = vkCreateCommandPool(m_device, &cmdPoolCreateInfo, nullptr, &m_commandPool);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Create VkCommandBuffer for initialisation and present
	 **********************************************************************************************/
	const VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
	    nullptr,
	    m_commandPool,
	    VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	    2
	};

	result = vkAllocateCommandBuffers(m_device, &commandBufferAllocateInfo, m_commandBuffers);
	LEARY_ASSERT(result == VK_SUCCESS);

	m_commandBufferInit    = m_commandBuffers[0];
	m_commandBufferPresent = m_commandBuffers[1];


	/***********************************************************************************************
	 * Create Depth buffer
	 **********************************************************************************************/
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memory_properties);

	// create depth buffer with same extent as the swapchain
	const VkExtent3D depthImageExtent = { m_width, m_height, 1 };

	const VkImageCreateInfo depthImageCreateInfo = {
	    VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
	    nullptr,
	    0,
	    VK_IMAGE_TYPE_2D,
	    VK_FORMAT_D16_UNORM,
	    depthImageExtent,
	    1,
	    1,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_IMAGE_TILING_OPTIMAL,
	    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
	    VK_SHARING_MODE_EXCLUSIVE,
	    0,
	    nullptr,
	    VK_IMAGE_LAYOUT_UNDEFINED
	};

	result = vkCreateImage(m_device, &depthImageCreateInfo, nullptr, &m_depthImage);
	LEARY_ASSERT(result == VK_SUCCESS);

	// create memory
	VkMemoryRequirements depthMemRequirements;
	vkGetImageMemoryRequirements(m_device, m_depthImage, &depthMemRequirements);

	const VkMemoryPropertyFlags depthMemPropertyFlags = 0;
	const int32_t memoryTypeIndex = find_memory_type_index(memory_properties,
	                                                        depthMemRequirements.memoryTypeBits,
	                                                        depthMemPropertyFlags);
	LEARY_ASSERT(memoryTypeIndex >= 0);

	const VkMemoryAllocateInfo depthMemoryAllocateInfo = {
	    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
	    nullptr,
	    depthMemRequirements.size,
	    (uint32_t) memoryTypeIndex
	};

	result = vkAllocateMemory(m_device, &depthMemoryAllocateInfo, nullptr, &m_depthMemory);
	LEARY_ASSERT(result == VK_SUCCESS);

	result = vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0);
	LEARY_ASSERT(result == VK_SUCCESS);


	// create image view
	const VkComponentMapping depthComponents = {
	    VK_COMPONENT_SWIZZLE_IDENTITY,
	    VK_COMPONENT_SWIZZLE_IDENTITY,
	    VK_COMPONENT_SWIZZLE_IDENTITY,
	    VK_COMPONENT_SWIZZLE_IDENTITY,
	};

	const VkImageSubresourceRange depthSubresourceRange = {
	    VK_IMAGE_ASPECT_DEPTH_BIT,
	    0, 1,
	    0, 1
	};

	const VkImageViewCreateInfo depthImageViewCreateInfo = {
	    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    nullptr,
	    0,
	    m_depthImage,
	    VK_IMAGE_VIEW_TYPE_2D,
	    VK_FORMAT_D16_UNORM,
	    depthComponents,
	    depthSubresourceRange
	};

	result = vkCreateImageView(m_device, &depthImageViewCreateInfo, nullptr, &m_depthImageView);
	LEARY_ASSERT(result == VK_SUCCESS);

	// transfer the depth buffer image layout to the correct type
	const VkImageMemoryBarrier imageMemoryBarrier = {
	    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	    nullptr,
	    0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
	    m_depthImage,
	    depthSubresourceRange

	};

	const VkCommandBufferBeginInfo commandBufferBeginInfo = {
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    nullptr,
	    0,
	    nullptr
	};

	result = vkBeginCommandBuffer(m_commandBufferInit, &commandBufferBeginInfo);
	LEARY_ASSERT(result == VK_SUCCESS);

	vkCmdPipelineBarrier(m_commandBufferInit,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0,
	                     0,
	                     nullptr,
	                     0,
	                     nullptr,
	                     1, &imageMemoryBarrier);

	result = vkEndCommandBuffer(m_commandBufferInit);
	LEARY_ASSERT(result == VK_SUCCESS);

	const VkSubmitInfo submitInfo = {
	    VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    nullptr,
	    0,
	    nullptr,
	    nullptr,
	    1,
	    &m_commandBufferInit,
	    0,
	    nullptr
	};

	result = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	LEARY_ASSERT(result == VK_SUCCESS);

	result = vkQueueWaitIdle(m_queue);
	LEARY_ASSERT(result == VK_SUCCESS);


	/***********************************************************************************************
	 * Create vkRenderPass
	 **********************************************************************************************/
	const VkAttachmentDescription attachmentDescriptions[2] = {
	    {  // color attachment description
	       0,
	       m_surfaceFormat,
	       VK_SAMPLE_COUNT_1_BIT,
	       VK_ATTACHMENT_LOAD_OP_CLEAR,
	       VK_ATTACHMENT_STORE_OP_STORE,
	       VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	       VK_ATTACHMENT_STORE_OP_DONT_CARE,
	       VK_IMAGE_LAYOUT_UNDEFINED,
	       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	    },
	    { // depth stencil attachment description
	      0,
	      VK_FORMAT_D16_UNORM,
	      VK_SAMPLE_COUNT_1_BIT,
	      VK_ATTACHMENT_LOAD_OP_CLEAR,
	      VK_ATTACHMENT_STORE_OP_DONT_CARE,
	      VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	      VK_ATTACHMENT_STORE_OP_DONT_CARE,
	      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	    }
	};

	const VkAttachmentReference colorAttachmentReference = {
	    0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentReference = {
	    1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkSubpassDescription subpassDescription = {
	    0,
	    VK_PIPELINE_BIND_POINT_GRAPHICS,
	    0, nullptr,
	    1, &colorAttachmentReference, nullptr, &depthAttachmentReference,
	    0, nullptr

	};

	const VkRenderPassCreateInfo renderPassCreateInfo = {
	    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    nullptr,
	    0,
	    2, attachmentDescriptions,
	    1, &subpassDescription,
	    0, nullptr
	};

	result = vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr, &m_renderPass);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Create Framebuffers
	 **********************************************************************************************/
	m_framebuffersCount = m_swapchainImagesCount;
	m_framebuffers      = new VkFramebuffer[m_framebuffersCount];

	for (uint32_t i = 0; i < m_framebuffersCount; ++i) {
		const VkImageView views[2] = { m_swapchainImageViews[i], m_depthImageView };

		const VkFramebufferCreateInfo createInfo = {
		    VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		    nullptr,
		    0,
		    m_renderPass,
		    2, views,
		    imageExtent.width, imageExtent.height,
		    1
		};

		result = vkCreateFramebuffer(m_device, &createInfo, nullptr, &m_framebuffers[i]);
		LEARY_ASSERT(result == VK_SUCCESS);
	}

	/***********************************************************************************************
	 * Create Vertex buffer
	 **********************************************************************************************/
	vertex_buffer = create_vertex_buffer(sizeof(float) * 6 * NUM_DEMO_VERTICES, 
	                                     (uint8_t*) vertices);

	/***********************************************************************************************
	 * Create Shader modules
	 **********************************************************************************************/
	std::string shaderFolder = Environment::resolvePath(eEnvironmentFolder::GameData,  "shaders/");

	std::ifstream shaderFile;
	shaderFile.open(shaderFolder + "vert.spv", std::ios_base::binary | std::ios_base::ate);


	size_t   vertexShaderSize = shaderFile.tellg();
	char *vertexShaderSource  = new char[vertexShaderSize];

	shaderFile.seekg(0, std::ios::beg);
	shaderFile.read(vertexShaderSource, vertexShaderSize);

	shaderFile.close();


	shaderFile.open(shaderFolder + "frag.spv", std::ios_base::binary | std::ios_base::ate);

	size_t   fragmentShaderSize = shaderFile.tellg();
	char *fragmentShaderSource  = new char[fragmentShaderSize];

	shaderFile.seekg(0, std::ios::beg);
	shaderFile.read(fragmentShaderSource, fragmentShaderSize);

	shaderFile.close();

	const VkShaderModuleCreateInfo vertexShaderCreateInfo = {
	    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    nullptr,
	    0,
	    vertexShaderSize, reinterpret_cast<uint32_t*>(vertexShaderSource)
	};

	result = vkCreateShaderModule(m_device, &vertexShaderCreateInfo, nullptr, &m_vertexShader);
	LEARY_ASSERT(result == VK_SUCCESS);

	const VkShaderModuleCreateInfo fragmentShaderCreateInfo = {
	    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    nullptr,
	    0,
	    fragmentShaderSize, reinterpret_cast<uint32_t*>(fragmentShaderSource)
	};

	result = vkCreateShaderModule(m_device, &fragmentShaderCreateInfo, nullptr, &m_fragmentShader);
	LEARY_ASSERT(result == VK_SUCCESS);

	delete[] fragmentShaderSource;
	delete[] vertexShaderSource;

	/***********************************************************************************************
	 * Create pipeline
	 **********************************************************************************************/
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
	    nullptr,
	    0,
	    0, nullptr,
	    0, nullptr
	};

	result = vkCreatePipelineLayout(m_device,  &pipelineLayoutCreateInfo,  nullptr,
	                                &m_pipelineLayout);
	LEARY_ASSERT(result == VK_SUCCESS);

	const VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {
	    { // vertex shader
	      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	      nullptr,
	      0,
	      VK_SHADER_STAGE_VERTEX_BIT,
	      m_vertexShader,
	      "main",
	      nullptr
	    },
	    { // fragment shader
	      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	      nullptr,
	      0,
	      VK_SHADER_STAGE_FRAGMENT_BIT,
	      m_fragmentShader,
	      "main",
	      nullptr
	    }
	};

	const VkVertexInputBindingDescription vertexInputBindingDescription = {
	    VERTEX_INPUT_BINDING,
	    sizeof(float)*6,
	    VK_VERTEX_INPUT_RATE_VERTEX
	};

	const VkVertexInputAttributeDescription vertexInputAttributeDescriptions[2] = {
	    {
	        0, VERTEX_INPUT_BINDING,
	        VK_FORMAT_R32G32B32_SFLOAT,
	        0
	    },
	    {
	        1, VERTEX_INPUT_BINDING,
	        VK_FORMAT_R32G32B32_SFLOAT,
	        sizeof(float)*3
	    }
	};


	const VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    1, &vertexInputBindingDescription,
	    2, vertexInputAttributeDescriptions
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	    VK_FALSE
	};

	const VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    2, dynamicStates
	};

	const VkPipelineViewportStateCreateInfo viewportCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    1, nullptr,
	    1, nullptr
	};

	const VkPipelineRasterizationStateCreateInfo rasterizationCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    VK_FALSE,
	    VK_FALSE,
	    VK_POLYGON_MODE_FILL,
	    VK_CULL_MODE_BACK_BIT,
	    VK_FRONT_FACE_CLOCKWISE,
	    VK_FALSE,
	    0.0f, 0.0f, 0.0f,
	    1.0f
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
	    VK_FALSE,
	    VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO,
	    VK_BLEND_OP_ADD,
	    VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO,
	    VK_BLEND_OP_ADD,
	    VK_COLOR_COMPONENT_R_BIT |
	    VK_COLOR_COMPONENT_G_BIT |
	    VK_COLOR_COMPONENT_B_BIT |
	    VK_COLOR_COMPONENT_A_BIT
	};

	const VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    VK_FALSE, VK_LOGIC_OP_CLEAR,
	    1, &colorBlendAttachment,
	    { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    VK_TRUE,
	    VK_TRUE,
	    VK_COMPARE_OP_LESS_OR_EQUAL,
	    VK_FALSE,
	    VK_FALSE,
	    {
	        VK_STENCIL_OP_KEEP,
	        VK_STENCIL_OP_KEEP,
	        VK_STENCIL_OP_KEEP,
	        VK_COMPARE_OP_ALWAYS,
	        0, 0, 0
	    },
	    {
	        VK_STENCIL_OP_KEEP,
	        VK_STENCIL_OP_KEEP,
	        VK_STENCIL_OP_KEEP,
	        VK_COMPARE_OP_ALWAYS,
	        0, 0, 0
	    },
	    0.0f, 0.0f
	};

	const VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {
	    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    nullptr,
	    0,
	    VK_SAMPLE_COUNT_1_BIT,
	    VK_FALSE,
	    0,
	    nullptr,
	    VK_FALSE,
	    VK_FALSE
	};

	const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
	    VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    nullptr,
	    0,
	    2,
	    shaderStageCreateInfo,
	    &vertexInputCreateInfo,
	    &inputAssemblyCreateInfo,
	    nullptr,
	    &viewportCreateInfo,
	    &rasterizationCreateInfo,
	    &multisampleCreateInfo,
	    &depthStencilCreateInfo,
	    &colorBlendCreateInfo,
	    &dynamicStateCreateInfo,
	    m_pipelineLayout,
	    m_renderPass,
	    0,
	    VK_NULL_HANDLE,
	    -1
	};

	result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
	                                   &m_pipeline);
	LEARY_ASSERT(result == VK_SUCCESS);

	vkDestroyShaderModule(m_device, m_vertexShader,   nullptr);
	vkDestroyShaderModule(m_device, m_fragmentShader, nullptr);
}

void VulkanDevice::destroy()
{
	// wait for pending operations
	VkResult result = vkQueueWaitIdle(m_queue);
	LEARY_ASSERT(result == VK_SUCCESS);

	vkDestroyPipeline(m_device, m_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

	// TODO: move these calls out of VulkanDevice, they are meant to be used outside as an api
	destroy_vertex_buffer(&vertex_buffer);

	for (uint32_t i = 0; i < m_framebuffersCount; ++i) {
		vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
	}

	delete[] m_framebuffers;

	vkDestroyRenderPass(m_device, m_renderPass, nullptr);

	vkDestroyImageView(m_device, m_depthImageView, nullptr);
	vkFreeMemory(m_device, m_depthMemory, nullptr);
	vkDestroyImage(m_device, m_depthImage, nullptr);

	vkFreeCommandBuffers(m_device, m_commandPool, 2, m_commandBuffers);
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

	delete[] m_swapchainImageViews;
	delete[] m_swapchainImages;

	vkDestroyDevice(m_device,     nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
}

VulkanVertexBuffer VulkanDevice::create_vertex_buffer(size_t size, uint8_t* data) 
{
	VulkanVertexBuffer buffer;
	buffer.size = size;

	VkBufferCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.size  = size;
	create_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices   = nullptr;

	VkResult result = vkCreateBuffer(this->m_device, &create_info, nullptr, &buffer.vk_buffer);
	LEARY_ASSERT(result == VK_SUCCESS);

	// TODO: allocate buffers from large memory pool in VulkanDevice
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(this->m_device, buffer.vk_buffer, &memory_requirements);

	int32_t index = find_memory_type_index(this->memory_properties, 
	                                       memory_requirements.memoryTypeBits, 
	                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	LEARY_ASSERT(index >= 0); 

	VkMemoryAllocateInfo allocate_info;
	allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.pNext           = nullptr;
	allocate_info.allocationSize  = memory_requirements.size;
	allocate_info.memoryTypeIndex = (uint32_t) index;

	result = vkAllocateMemory(this->m_device, &allocate_info, nullptr, &buffer.vk_memory);
	LEARY_ASSERT(result == VK_SUCCESS);

	result = vkBindBufferMemory(this->m_device, buffer.vk_buffer, buffer.vk_memory, 0);
	LEARY_ASSERT(result == VK_SUCCESS);

	if (data != nullptr)
	{
		void* memptr;
		result = vkMapMemory(this->m_device, buffer.vk_memory, 0, VK_WHOLE_SIZE, 0, &memptr);
		LEARY_ASSERT(result == VK_SUCCESS);

		memcpy(memptr, data, size);

		vkUnmapMemory(this->m_device, buffer.vk_memory);
	}

	return buffer;
}

void VulkanDevice::destroy_vertex_buffer(VulkanVertexBuffer* buffer) 
{
	// TODO: free memory from large memory pool in VulkanDevice 
	vkFreeMemory(this->m_device,    buffer->vk_memory, nullptr);
	vkDestroyBuffer(this->m_device, buffer->vk_buffer, nullptr);
}

void VulkanDevice::present()
{
	VkResult result;

	VkSemaphore imageAcquired, renderComplete;

	const VkSemaphoreCreateInfo semaphoreCreateInfo = {
	    VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	    nullptr,
	    0
	};

	result = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &imageAcquired);
	LEARY_ASSERT(result == VK_SUCCESS);

	result = vkCreateSemaphore(m_device, &semaphoreCreateInfo, nullptr, &renderComplete);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Acquire next available swapchain image
	 **********************************************************************************************/
	uint32_t imageIndex    = std::numeric_limits<uint32_t>::max();
	const uint64_t timeout = std::numeric_limits<uint64_t>::max();

	result = vkAcquireNextImageKHR(m_device,
	                               m_swapchain,
	                               timeout,
	                               imageAcquired,
	                               VK_NULL_HANDLE,
	                               &imageIndex);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Fill present command buffer
	 **********************************************************************************************/
	const VkCommandBufferBeginInfo presentBeginInfo = {
	    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    nullptr,
	    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
	    nullptr
	};

	result = vkBeginCommandBuffer(m_commandBufferPresent, &presentBeginInfo);
	LEARY_ASSERT(result == VK_SUCCESS);

	const VkImageSubresourceRange subresourceRange = {
	    VK_IMAGE_ASPECT_COLOR_BIT,
	    0, 1,
	    0, 1
	};

	VkImageMemoryBarrier imageMemoryBarrier = {
	    VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
	    nullptr,
	    0, 0,
	    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED,
	    VK_QUEUE_FAMILY_IGNORED,
	    VK_QUEUE_FAMILY_IGNORED,
	    m_swapchainImages[imageIndex],
	    subresourceRange
	};

	// transition swapchain image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	vkCmdPipelineBarrier(m_commandBufferPresent,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
	                     0,
	                     0, nullptr,
	                     0, nullptr,
	                     1, &imageMemoryBarrier);

	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.3f;
	clearValues[0].color.float32[1] = 0.5f;
	clearValues[0].color.float32[2] = 0.9f;
	clearValues[0].color.float32[3] = 1.0f;

	clearValues[1].depthStencil  = { 1, 0 };

	const VkRect2D renderArea = {
	    { 0,       0        },
	    { m_width, m_height }
	};

	const VkRenderPassBeginInfo renderPassBeginInfo = {
	    VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    nullptr,
	    m_renderPass,
	    m_framebuffers[imageIndex],
	    renderArea,
	    2, clearValues
	};

	vkCmdBeginRenderPass(m_commandBufferPresent, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(m_commandBufferPresent, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	VkViewport viewport = {
	    0.0f,    0.0f,
	    static_cast<float>(m_width), static_cast<float>(m_height),
	    0.0f,    1.0f
	};

	vkCmdSetViewport(m_commandBufferPresent, 0, 1, &viewport);

	vkCmdSetScissor(m_commandBufferPresent, 0, 1, &renderArea);

	VkDeviceSize buffersOffsets = 0;
	vkCmdBindVertexBuffers(m_commandBufferPresent, VERTEX_INPUT_BINDING,
	                       1, &vertex_buffer.vk_buffer, &buffersOffsets);

	vkCmdDraw(m_commandBufferPresent, 3, 1, 0, 0);

	vkCmdEndRenderPass(m_commandBufferPresent);

	result = vkEndCommandBuffer(m_commandBufferPresent);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Submit Present command buffer to queue
	 **********************************************************************************************/
	const VkPipelineStageFlags piplineStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	const VkSubmitInfo submitInfo = {
	    VK_STRUCTURE_TYPE_SUBMIT_INFO,
	    nullptr,
	    1, &imageAcquired,
	    &piplineStageFlags,
	    1, &m_commandBufferPresent,
	    1, &renderComplete
	};

	result = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	LEARY_ASSERT(result == VK_SUCCESS);

	/***********************************************************************************************
	 * Present the rendered image
	 **********************************************************************************************/
	const VkPresentInfoKHR presentInfo = {
	    VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
	    nullptr,
	    1, &renderComplete,
	    1, &m_swapchain,
	    &imageIndex,
	    nullptr
	};

	result = vkQueuePresentKHR(m_queue, &presentInfo);
	LEARY_ASSERT(result == VK_SUCCESS);

	// wait for idle operations
	result = vkQueueWaitIdle(m_queue);
	LEARY_ASSERT(result == VK_SUCCESS);

	vkDestroySemaphore(m_device, renderComplete, nullptr);
	vkDestroySemaphore(m_device, imageAcquired,  nullptr);
}

namespace
{
	int32_t find_memory_type_index(const VkPhysicalDeviceMemoryProperties properties,
	                               const uint32_t                         type_bits,
	                               const VkMemoryPropertyFlags            req_properties)
	{
		for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
			if ((type_bits & (1 << i)) &&
			    ((properties.memoryTypes[i].propertyFlags & req_properties) == req_properties))
				return i;
		}

		return -1;

	}
}
