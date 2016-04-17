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

#include "prefix.h"
#include "window.h"

Window::Window(const char* title, uint32_t width, uint32_t height)
{
	// initialise SDL and OpenGL
	m_window = SDL_CreateWindow (
			title,
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			width, height,
			SDL_WINDOW_OPENGL
	);

	LEARY_ASSERT_PRINTF(m_window != nullptr,
	                    "Failed to create OpenGL window: %s", SDL_GetError());

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	m_context = SDL_GL_CreateContext(m_window);
	LEARY_ASSERT_PRINTF(m_context != nullptr,
	                    "Failed to create OpenGL context: %s", SDL_GetError());

	glewExperimental = GL_TRUE;
	glewInit();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

Window::~Window()
{
	SDL_GL_DeleteContext(m_context);
	SDL_DestroyWindow(m_window);
}

void Window::swap()
{
	SDL_GL_SwapWindow(m_window);
}


