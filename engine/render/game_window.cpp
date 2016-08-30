/**
 * @file:   window.h
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

#include "game_window.h"

#include <limits>

#include <SDL_opengl.h>

#include "util/debug.h"

void GameWindow::create(const char* title, uint32_t width, uint32_t height, bool fullscreen)
{
	m_width  = width;
	m_height = height;
	m_title  = title;

	LEARY_ASSERT(width  <= std::numeric_limits<int32_t>::max());
	LEARY_ASSERT(height <= std::numeric_limits<int32_t>::max());

	uint32_t flags = 0;

#if LEARY_OPENGL
	flags |= SDL_WINDOW_OPENGL;
#endif // LEARY_OPENGL

	LEARY_UNUSED(fullscreen);
#if 0
	if (fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
#endif

	// initialise SDL
	m_window = SDL_CreateWindow(title,
	                            SDL_WINDOWPOS_UNDEFINED,
	                            SDL_WINDOWPOS_UNDEFINED,
	                            static_cast<int32_t>(width),
	                            static_cast<int32_t>(height),
	                            flags);

	LEARY_ASSERT_PRINTF(m_window != nullptr,
	                    "Failed to create window: %s", SDL_GetError());
}

void GameWindow::destroy()
{
	SDL_DestroyWindow(m_window);
}

#if LEARY_OPENGL
void GameWindow::swap()
{
	// @TODO: move to opengl_swapchain (? possibly device, depends how vulkan ends up being
	// structured
	SDL_GL_SwapWindow(m_window);
}
#endif // LEARY_OPENGL

