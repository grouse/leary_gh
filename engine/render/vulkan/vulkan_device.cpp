#include "vulkan_device.h"

#include <vector>
#include <limits>

#include <SDL_syswm.h>
#include <X11/Xlib-xcb.h>


#include "util/debug.h"
#include "core/settings.h"
#include "render/game_window.h"

void VulkanDevice::create(const GameWindow& window)
{
    const Settings *settings = Settings::get();
    /***********************************************************************************************
     * Enumerate and validate supported extensions and layers
     **********************************************************************************************/
    std::vector<const char*> layerNamesToEnable;
    //layerNamesToEnable.push_back("VK_LAYER_LUNARG_standard_validation");

    std::vector<const char*> extensionNamesToEnable;
    extensionNamesToEnable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    //extensionNamesToEnable.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
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

    // Enumerate the extensions we wanted to enable and the layers we've found supported and
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
}

void VulkanDevice::destroy()
{
    // wait for pending operations
    VkResult result = vkQueueWaitIdle(m_queue);
    LEARY_ASSERT(result == VK_SUCCESS);

    vkFreeCommandBuffers(m_device, m_commandPool, 2, m_commandBuffers);
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    delete[] m_swapchainImageViews;
    delete[] m_swapchainImages;

    vkDestroyDevice(m_device,     nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);
}

void VulkanDevice::present()
{
    float red   = 1.0f;
    float green = 0.0f;
    float blue  = 1.0f;

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

    VkClearColorValue clearColor;
    clearColor.float32[0] = red;
    clearColor.float32[1] = green;
    clearColor.float32[2] = blue;
    clearColor.float32[3] = 1.0f; // alpha

    vkCmdClearColorImage(m_commandBufferPresent,
                         m_swapchainImages[imageIndex],
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &clearColor,
                         1, &subresourceRange);

    // transition swapchain image to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    imageMemoryBarrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout     = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vkCmdPipelineBarrier(m_commandBufferPresent,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0, nullptr,
                         0, nullptr,
                         1, &imageMemoryBarrier);

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
