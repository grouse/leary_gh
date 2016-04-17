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

#ifndef LEARY_WINDOW_H
#define LEARY_WINDOW_H

#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

class Window {
public:
	Window(const char* title, uint32_t width, uint32_t height);
	~Window();


	void swap();

private:
	SDL_Window* m_window;
	SDL_GLContext m_context;
};


#endif //LEARY_WINDOW_H
