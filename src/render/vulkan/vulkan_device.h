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

#include "core/settings.h"
#include "platform/platform_vulkan.h"

struct VulkanVertexBuffer {
	size_t               size;

	VkBuffer             vk_buffer;
	VkDeviceMemory       vk_memory;
};

struct VulkanTexture {
	uint32_t       width;
	uint32_t       height;
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

class VulkanDevice {
public:
	void create(Settings settings, PlatformState platform_state);
	void destroy();

	void present();

	VkCommandBuffer begin_command_buffer();
	void            end_command_buffer(VkCommandBuffer buffer);

	void transition_image(VkCommandBuffer command,
	                      VkImage image,
	                      VkImageLayout src,
	                      VkImageLayout dst);

	void transition_image(VkImage image,
	                      VkImageLayout src,
	                      VkImageLayout dst);

	void copy_image(uint32_t width, uint32_t height, VkImage src, VkImage dst);

	uint32_t find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags);

	VulkanVertexBuffer create_vertex_buffer(size_t size, uint8_t *data);
	void               destroy_vertex_buffer(VulkanVertexBuffer *buffer);


	VkImage create_image(VkFormat format,
	                     uint32_t width,
	                     uint32_t height,
	                     VkImageTiling tiling,
	                     VkImageUsageFlags usage,
	                     VkMemoryPropertyFlags properties,
	                     VkDeviceMemory *memory);

	VulkanTexture create_texture(uint32_t width,
	                             uint32_t height,
	                             VkFormat format,
	                             void *pixels);

	VulkanShader create_shader(uint32_t *source,
	                           size_t size,
	                           VkShaderStageFlagBits stage);

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

	VkDescriptorSet       descriptor_set;
	VkDescriptorPool      descriptor_pool;
	VkDescriptorSetLayout descriptor_layout;

	VulkanShader vertex_shader;
	VulkanShader fragment_shader;
	VkSampler texture_sampler;
};

#endif // LEARY_VULKAN_DEVICE_H
