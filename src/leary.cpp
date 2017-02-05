/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */


#include "render/vulkan_device.cpp"

#include "core/settings.cpp"
#include "core/tokenizer.cpp"
#include "core/serialize.cpp"

struct Camera {
	Matrix4f view;
	Matrix4f projection;
};

struct GameState {
	VulkanDevice vulkan;
	VulkanPipeline pipeline;

	VulkanTexture texture;
	Camera camera;
	Vector3f velocity = {};
	VulkanUniformBuffer camera_buffer;

	VulkanBuffer vertex_buffer;
};

enum InputAction {
	InputAction_move_vertical_start,
	InputAction_move_horizontal_start,
	InputAction_move_vertical_end,
	InputAction_move_horizontal_end,
};



const f32 vertices[] = {
	-16.0f,  -16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0,  0.0f,
	 16.0f,  -16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	 16.0f,  16.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,

	 16.0f,  16.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
	-16.0f,  16.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	-16.0f, -16.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

void game_load_settings(Settings *settings)
{
	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	SERIALIZE_LOAD_CONF(settings_path, Settings, settings);
	free(settings_path);
}

void game_init(Settings *settings, PlatformState *platform, GameState *game)
{
	VAR_UNUSED(platform);
	game->vulkan = create_device(settings, platform);

	Vector4f pixels[32 * 32] = {};
	pixels[0]     = { 1.0f, 0.0f, 0.0f, 1.0f };
	pixels[31]    = { 0.0f, 1.0f, 0.0f, 1.0f };
	pixels[1023]     = { 0.0f, 0.0f, 1.0f, 1.0f };

	game->texture = create_texture(&game->vulkan,
	                               32, 32, VK_FORMAT_R32G32B32A32_SFLOAT,
	                               pixels);

	game->camera.view = Matrix4f::identity();
	game->camera.view = translate(game->camera.view, Vector3f{0.0f, 0.0f, 0.0f});

	f32 left   = - (f32)settings->video.resolution.width / 2.0f;
	f32 right  =   (f32)settings->video.resolution.width / 2.0f;
	f32 bottom = - (f32)settings->video.resolution.height / 2.0f;
	f32 top    =   (f32)settings->video.resolution.height / 2.0f;
	game->camera.projection = Matrix4f::orthographic(left, right, top, bottom, 0.0f, 1.0f);

	game->camera_buffer = create_uniform_buffer(&game->vulkan, sizeof(Camera));
	update_uniform_data(&game->vulkan,
	                    game->camera_buffer,
	                    &game->camera,
	                    sizeof(Camera));

	game->vertex_buffer = create_vertex_buffer(&game->vulkan,
	                                           sizeof(vertices),
	                                           (u8*)vertices);

	game->pipeline = create_pipeline(&game->vulkan);
	update_descriptor_sets(&game->vulkan,
	                       game->pipeline,
	                       game->texture,
	                       game->camera_buffer);
}

void game_input(GameState *game, InputAction action, float axis)
{
	switch (action) {
	case InputAction_move_vertical_start:
		game->velocity.y += axis * 100.0f;
		break;
	case InputAction_move_horizontal_start:
		game->velocity.x += axis * 100.0f;
		break;
	default: break;
	}
}

void game_input(GameState *game, InputAction action)
{
	switch (action) {
	case InputAction_move_vertical_end:
		game->velocity.y = 0.0f;
		break;
	case InputAction_move_horizontal_end:
		game->velocity.x = 0.0f;
		break;
	default: break;
	}
}

void game_quit(Settings *settings, GameState *game)
{
	destroy(&game->vulkan, game->vertex_buffer);
	destroy(&game->vulkan, game->camera_buffer);
	destroy(&game->vulkan, game->texture);
	destroy(&game->vulkan, game->pipeline);
	destroy(&game->vulkan);


	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	SERIALIZE_SAVE_CONF(settings_path, Settings, settings);
	free(settings_path);

	platform_quit();
}

void game_update(GameState* game, f32 dt)
{
	game->camera.view = translate(game->camera.view, dt * game->velocity);
	update_uniform_data(&game->vulkan, game->camera_buffer, &game->camera, sizeof(Camera));
}

void game_render(GameState *game)
{
	VkResult result;
	u32 image_index = acquire_swapchain_image(&game->vulkan);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	result = vkBeginCommandBuffer(game->vulkan.cmd_present, &begin_info);
	DEBUG_ASSERT(result == VK_SUCCESS);

	std::array<VkClearValue, 1> clear_values = {};
	clear_values[0].color        = { {1.0f, 0.0f, 0.0f, 0.0f} };

	VkRenderPassBeginInfo render_info = {};
	render_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_info.renderPass        = game->vulkan.renderpass;
	render_info.framebuffer       = game->vulkan.framebuffers[image_index];
	render_info.renderArea.offset = { 0, 0 };
	render_info.renderArea.extent = game->vulkan.swapchain.extent;
	render_info.clearValueCount   = clear_values.size();
	render_info.pClearValues      = clear_values.data();

	vkCmdBeginRenderPass(game->vulkan.cmd_present,
		                 &render_info,
		                 VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(game->vulkan.cmd_present,
			              VK_PIPELINE_BIND_POINT_GRAPHICS,
			              game->pipeline.handle);

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(game->vulkan.cmd_present,
			                   0,
			                   1, &game->vertex_buffer.handle,
			                   offsets);

		vkCmdBindDescriptorSets(game->vulkan.cmd_present,
			                    VK_PIPELINE_BIND_POINT_GRAPHICS,
			                    game->pipeline.layout,
			                    0,
			                    1, &game->pipeline.descriptor_set,
			                    0, nullptr);

		vkCmdDraw(game->vulkan.cmd_present, 6, 1, 0, 0);
	vkCmdEndRenderPass(game->vulkan.cmd_present);

	result = vkEndCommandBuffer(game->vulkan.cmd_present);
	DEBUG_ASSERT(result == VK_SUCCESS);

	present(&game->vulkan, image_index, 1, &game->vulkan.cmd_present);
}
