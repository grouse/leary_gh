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

#include "prefix.h"

#include <GL/glew.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include "core/settings.h"
#include "render/rendering.h"
#include "render/texture.h"

int main(int, char *[]) 
{
	int32_t result = SDL_Init(SDL_INIT_VIDEO);
	LEARY_UNUSED(result);
	LEARY_ASSERT_PRINTF(result == 0, "Failed to initialise SDL2 %s", SDL_GetError());

	Settings::create();
	Settings::get()->load("settings.ini");

	Rendering::create();
	Rendering* rendering = Rendering::get();

	TextureManager::create();
	TextureManager::get()->init();

	bool quit = false;
	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				quit = true;

			if (event.type == SDL_KEYUP) {
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					quit = true;
					break;
				}
			}
		}

		rendering->update();
	}

	TextureManager::destroy();

	Rendering::destroy();

	Settings::get()->save("settings.ini");
	Settings::destroy();
	
	SDL_Quit();
	return 0;
}
