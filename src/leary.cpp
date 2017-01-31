/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */


#include "render/vulkan/vulkan_device.cpp"

#include "core/settings.cpp"
#include "core/serialize.cpp"

struct GameState {
	VulkanDevice vulkan_device;
};


void init_game(Settings *settings, PlatformState *platform, GameState *game)
{
	VAR_UNUSED(platform);
	game->vulkan_device.create(*settings, *platform);
}

void update_game()
{
}

void render_game(GameState *state)
{
	state->vulkan_device.present();
}
