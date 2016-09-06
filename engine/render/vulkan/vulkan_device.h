/**
 * @file:   vulkan_device.h
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

#ifndef LEARY_VULKAN_DEVICE_H
#define LEARY_VULKAN_DEVICE_H

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

class GameWindow;
class VulkanDevice;

struct VulkanVertexBuffer {
	size_t               size;

	VkBuffer             vk_buffer;
	VkDeviceMemory       vk_memory;
};

class VulkanDevice {
public:
	void create(const GameWindow& window);
	void destroy();

	void present();

	VulkanVertexBuffer create_vertex_buffer(size_t size, uint8_t *data);
	void               destroy_vertex_buffer(VulkanVertexBuffer *buffer);

	uint32_t         m_width;
	uint32_t         m_height;

	VkInstance       vk_instance;

	// Device and its queue(s)
	VkDevice         vk_device;
	VkQueue          vk_queue;
	uint32_t         queue_family_index;

	// Physical device
	VkPhysicalDevice                 vk_physical_device;
	VkPhysicalDeviceMemoryProperties vk_physical_memory_properties; 


	// Swapchain
	VkSurfaceKHR     vk_surface;
	VkFormat         vk_surface_format;
	VkSwapchainKHR   vk_swapchain;
	VkExtent2D       vk_swapchain_extent;

	uint32_t         swapchain_images_count;
	VkImage          *vk_swapchain_images;
	VkImageView      *vk_swapchain_imageviews;

	// Command pool and buffers
	VkCommandPool    vk_command_pool;

	VkCommandBuffer  vk_cmd_buffers[2];
	VkCommandBuffer  vk_cmd_init;
	VkCommandBuffer  vk_cmd_present;

	// Depth Buffer
	VkImage          vk_depth_image;
	VkImageView      vk_depth_imageview;
	VkDeviceMemory   vk_depth_memory;

	// Render pass
	VkRenderPass     vk_renderpass;

	// Framebuffer
	int32_t          framebuffers_count;
	VkFramebuffer    *vk_framebuffers;

	// Vertex buffer
	VulkanVertexBuffer vertex_buffer;

	// Pipeline
	VkPipeline       vk_pipeline;
	VkPipelineLayout vk_pipeline_layout;

	VkShaderModule   vk_vertex_shader;
	VkShaderModule   vk_fragment_shader;
};

#endif // LEARY_VULKAN_DEVICE_H
