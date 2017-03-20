/**
 * file:    vulkan_device.h
 * created: 2017-03-13
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef VULKAN_DEVICE_H
#define VULKAN_DEVICE_H

#include "platform/platform.h"

enum ShaderStage {
	ShaderStage_vertex,
	ShaderStage_fragment,
	ShaderStage_max
};

enum ShaderID {
	ShaderID_generic_vert,
	ShaderID_generic_frag,
	ShaderID_mesh_vert,
	ShaderID_mesh_frag,
	ShaderID_font_vert,
	ShaderID_font_frag,
	ShaderID_terrain_vert,
	ShaderID_terrain_frag
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

	i32                   sampler_count;
	VkSampler             *samplers;
};

struct VulkanDepthBuffer {
	VkFormat       format;
	VkImage        image;
	VkImageView    imageview;
	VkDeviceMemory memory;
};

struct VulkanSwapchain {
	VkSurfaceKHR      surface;
	VkFormat          format;
	VkSwapchainKHR    handle;
	VkExtent2D        extent;

	u32               images_count;
	VkImage           *images;
	VkImageView       *imageviews;

	VulkanDepthBuffer depth;

	VkSemaphore       available;
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

	StaticArray<VkFramebuffer> framebuffers;
};



#endif /* VULKAN_DEVICE_H */

