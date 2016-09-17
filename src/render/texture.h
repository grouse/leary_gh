/**
 * @file:   texture.h
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

#ifndef LEARY_TEXTURE_H
#define LEARY_TEXTURE_H

#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <SDL_opengl.h>

struct Texture {
	GLuint id;
	unsigned int refcount;
};

class TextureManager {
public:
	void init();

	Texture load(std::string);
	void unload(std::string); 

	static void create();
	static void destroy();
	static TextureManager* get();

private:
	TextureManager() {}
	~TextureManager() {}
	static TextureManager* m_instance;


	std::unordered_map<std::string, Texture> textures;
};

#endif // LEARY_TEXTURE_H
