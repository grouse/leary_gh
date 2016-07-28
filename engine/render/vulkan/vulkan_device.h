#ifndef LEARY_VULKAN_DEVICE_H
#define LEARY_VULKAN_DEVICE_H

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan.h>

class GameWindow;

class VulkanDevice {
public:
    void create(const GameWindow& window);
    void destroy();

    void present();

private:
    VkInstance       m_instance;

    // Device and its queue(s)
    VkDevice         m_device;
    VkQueue          m_queue;
    uint32_t         m_queueFamilyIndex;

    // Physical device
    VkPhysicalDevice  m_physicalDevice;

    // Swapchain
    VkSurfaceKHR     m_surface;
    VkFormat         m_surfaceFormat;
    VkSwapchainKHR   m_swapchain;

    uint32_t         m_swapchainImagesCount;
    VkImage         *m_swapchainImages;
    VkImageView     *m_swapchainImageViews;

    // Command pool and buffers
    VkCommandPool    m_commandPool;

    VkCommandBuffer  m_commandBuffers[2];
    VkCommandBuffer  m_commandBufferInit;
    VkCommandBuffer  m_commandBufferPresent;
};

#endif // LEARY_VULKAN_DEVICE_H
