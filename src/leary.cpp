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

struct GameState {
	VulkanDevice vulkan;
	VulkanPipeline pipeline;

	VulkanTexture texture;
	Camera camera;
	Vector3f velocity = {};
	VulkanUniformBuffer camera_buffer;
};

enum InputAction {
	InputAction_move_vertical_start,
	InputAction_move_horizontal_start,
	InputAction_move_vertical_end,
	InputAction_move_horizontal_end,
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
	game->vulkan.create(*settings, *platform);

	Vector4f pixels[32 * 32] = {};
	pixels[0]     = { 1.0f, 0.0f, 0.0f, 1.0f };
	pixels[31]    = { 0.0f, 1.0f, 0.0f, 1.0f };
	pixels[1023]     = { 0.0f, 0.0f, 1.0f, 1.0f };

	game->texture = game->vulkan.create_texture(32, 32, VK_FORMAT_R32G32B32A32_SFLOAT, pixels);

	game->camera.view = Matrix4f::identity();
	game->camera.view = translate(game->camera.view, Vector3f{0.0f, 0.0f, 0.0f});

	f32 left   = - (f32)settings->video.resolution.width / 2.0f;
	f32 right  =   (f32)settings->video.resolution.width / 2.0f;
	f32 bottom = - (f32)settings->video.resolution.height / 2.0f;
	f32 top    =   (f32)settings->video.resolution.height / 2.0f;
	game->camera.projection = Matrix4f::orthographic(left, right, top, bottom, 0.0f, 1.0f);

	game->camera_buffer = game->vulkan.create_uniform_buffer(sizeof(Camera));
	game->vulkan.update_uniform_data(game->camera_buffer,
	                                 &game->camera,
	                                 sizeof(Camera));

	game->pipeline = game->vulkan.create_pipeline();
	game->vulkan.update_descriptor_sets(game->pipeline, game->texture, game->camera_buffer);
}

void game_input(GameState *game, InputAction action, float axis)
{
	switch (action) {
	case InputAction_move_vertical_start:
		game->velocity.y = axis * 50.0f;
		break;
	case InputAction_move_horizontal_start:
		game->velocity.x = axis * 50.0f;
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
	game->vulkan.destroy(game->camera_buffer);
	game->vulkan.destroy(game->texture);
	game->vulkan.destroy(game->pipeline);

	game->vulkan.destroy();


	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	SERIALIZE_SAVE_CONF(settings_path, Settings, settings);
	free(settings_path);

	platform_quit();
}

void game_update(GameState* game, f32 dt)
{
	game->camera.view = translate(game->camera.view, dt * game->velocity);
	game->vulkan.update_uniform_data(game->camera_buffer,
	                                 &game->camera,
	                                 sizeof(Camera));
}

void game_render(GameState *game)
{
	u32 image_index = game->vulkan.acquire_swapchain_image();
	game->vulkan.draw(image_index, game->pipeline);
	game->vulkan.present(image_index);
}
