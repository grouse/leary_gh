/**
 * @file:   main.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
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

#include <SDL.h>

#include "core/settings.h"

#include "render/game_window.h"

#include "util/macros.h"
#include "util/debug.h"

#if LEARY_OPENGL
    #include "render/opengl/opengl_device.h"
    #include "render/texture.h"
#endif // LEARY_OPENGL

#if LEARY_VULKAN
    #include "render/vulkan/vulkan_device.h"
#endif // LEARY_VULKAN

int main(int, char *[]) 
{
	int32_t result = SDL_Init(SDL_INIT_VIDEO);
	LEARY_UNUSED(result);
	LEARY_ASSERT_PRINTF(result == 0, "Failed to initialise SDL2 %s", SDL_GetError());

    Settings::create();

    Settings *settings = Settings::get();
    settings->load("settings.ini");

    GameWindow game_window;
    game_window.create("leary",
                       settings->video.resolution.width,
                       settings->video.resolution.height,
                       settings->video.fullscreen);

#if LEARY_OPENGL
    OpenGLDevice opengl_device;
    opengl_device.create(game_window);

    TextureManager::create();
    TextureManager::get()->init();
#endif // LEARY_OPENGL

#if LEARY_VULKAN
    VulkanDevice vulkan_device;
    vulkan_device.create(game_window);
#endif // LEARY_VULKAN

	uint32_t lastTime = SDL_GetTicks();
	uint32_t nbFrames = 0;

	bool logFrameTime = true;

	bool quit = false;
	while (!quit) {
		nbFrames++;
        const uint32_t currentTime = SDL_GetTicks();
        const uint32_t frameTime   = lastTime - currentTime;

        if (frameTime >= 1000) {
            if (logFrameTime) {
                LEARY_LOGF(eLogType::Info, "%d fps, %f ms/frame", nbFrames, 1000.0 / nbFrames);
            }

            nbFrames = 0;
            lastTime += 1000;
        }

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				quit = true;

			if (event.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				case SDLK_F1:
					logFrameTime = !logFrameTime;
					if (logFrameTime)
						LEARY_LOG(eLogType::Info, "Turned frametime logging: on");
					else
						LEARY_LOG(eLogType::Info, "Turned frametime logging: off");
					break;
				}
            }
        }

#if LEARY_VULKAN
        vulkan_device.present();
#endif

#if LEARY_OPENGL
        opengl_device.present();
#endif

	}

#if LEARY_OPENGL
    TextureManager::destroy();
    opengl_device.destroy();
#endif // LEARY_OPENGL

#if LEARY_VULKAN
    vulkan_device.destroy();
#endif

    game_window.destroy();

	Settings::get()->save("settings.ini");
	Settings::destroy();
	
	SDL_Quit();
	return 0;
}
