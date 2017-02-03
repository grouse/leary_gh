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
#include "core/math.h"

struct VulkanBuffer {
	size_t         size;
	VkBuffer       handle;
	VkDeviceMemory memory;
};

struct VulkanUniformBuffer {
	VulkanBuffer staging;
	VulkanBuffer buffer;
};

struct VulkanTexture {
	u32       width;
	u32       height;
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

enum ShaderStage {
	ShaderStage_vertex,
	ShaderStage_fragment,
	ShaderStage_max
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

struct Camera {
	Matrix4f view;
	Matrix4f projection;
};


class VulkanDevice {
public:
	void create(Settings settings, PlatformState platform_state);
	void destroy();

	u32 acquire_swapchain_image();
	void draw(u32 image_index, VulkanPipeline pipeline);
	void present(u32 image_index);

	void update_descriptor_sets(VulkanPipeline pipeline,
	                            VulkanTexture texture,
	                            VulkanUniformBuffer ubo);

	VulkanPipeline create_pipeline();

	VkCommandBuffer begin_command_buffer();
	void            end_command_buffer(VkCommandBuffer buffer);

	void transition_image(VkCommandBuffer command,
	                      VkImage image,
	                      VkImageLayout src,
	                      VkImageLayout dst);

	void transition_image(VkImage image,
	                      VkImageLayout src,
	                      VkImageLayout dst);

	void copy_image(u32 width, u32 height, VkImage src, VkImage dst);

	u32 find_memory_type(u32 filter, VkMemoryPropertyFlags flags);

	VulkanBuffer create_buffer(size_t size,
	                           VkBufferUsageFlags usage,
	                           VkMemoryPropertyFlags memory_flags);
	void         destroy_buffer(VulkanBuffer *buffer);

	VulkanBuffer create_vertex_buffer(size_t size, u8 *data);
	VulkanUniformBuffer create_uniform_buffer(size_t size);
	void update_uniform_data(VulkanUniformBuffer buffer, void *data, size_t size);
	void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);


	VkImage create_image(VkFormat format,
	                     u32 width,
	                     u32 height,
	                     VkImageTiling tiling,
	                     VkImageUsageFlags usage,
	                     VkMemoryPropertyFlags properties,
	                     VkDeviceMemory *memory);

	VulkanTexture create_texture(u32 width,
	                             u32 height,
	                             VkFormat format,
	                             void *pixels);

	VulkanShader create_shader(u32 *source,
	                           size_t size,
	                           VkShaderStageFlagBits stage);


	VkInstance       vk_instance;

	// Device and its queue(s)
	VkDevice         vk_device;
	VkQueue          vk_queue;
	u32         queue_family_index;

	// Physical device
	VkPhysicalDevice                 vk_physical_device;
	VkPhysicalDeviceMemoryProperties vk_physical_memory_properties;


	// Swapchain
	VkSurfaceKHR     vk_surface;
	VkFormat         vk_surface_format;
	VkSwapchainKHR   vk_swapchain;
	VkExtent2D       vk_swapchain_extent;

	u32         swapchain_images_count;
	VkImage          *vk_swapchain_images;
	VkImageView      *vk_swapchain_imageviews;

	VkSemaphore swapchain_image_available;
	VkSemaphore render_completed;

	// Command pool and buffers
	VkCommandPool    vk_command_pool;

	VkCommandBuffer  vk_cmd_present;

	// Depth Buffer
	VkImage          vk_depth_image;
	VkImageView      vk_depth_imageview;
	VkDeviceMemory   vk_depth_memory;

	// Render pass
	VkRenderPass     vk_renderpass;

	// Framebuffer
	i32          framebuffers_count;
	VkFramebuffer    *vk_framebuffers;

	// Vertex buffer
	VulkanBuffer vertex_buffer;
};

#endif // LEARY_VULKAN_DEVICE_H
