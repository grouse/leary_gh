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
#include "core/array.h"

struct VulkanCode {
	PFN_vkCreateDebugReportCallbackEXT   CreateDebugReportCallbackEXT;
	PFN_vkDestroyDebugReportCallbackEXT  DestroyDebugReportCallbackEXT;
};

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

	// TODO(jesper): better name. The suffix implies the bind frequency. E.g.
	// _pipeline will contain descriptors bound once per pipeline, e.g.
	// projection matrix
	VkDescriptorSetLayout descriptor_layout_pipeline;
	VkDescriptorSetLayout descriptor_layout_material;

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

	VulkanCode               code;

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

	SARRAY(VkFramebuffer) framebuffers;
};

enum MaterialID {
	Material_font,
	Material_phong
};

enum ResourceSlot {
	ResourceSlot_mvp,
	ResourceSlot_font_atlas,
	ResourceSlot_texture
};

struct Material {
	MaterialID       id;
	// NOTE(jesper): does this make sense? we need to use the pipeline when
	// creating the material for its descriptor layout, but maybe the dependency
	// makes more sense if it goes the other way around?
	VulkanPipeline   *pipeline;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet  descriptor_set;
};


#endif /* VULKAN_DEVICE_H */

