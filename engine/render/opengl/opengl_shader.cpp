/**
 * @file:   rendering.cpp
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

#include "opengl_shader.h"

#include <string>
#include <fstream>

#include "GL/glew.h"
#include "SDL_opengl.h"

#include "util/environment.h"
#include "util/debug.h"

bool OpenGLProgram::create(OpenGLShader* shaders, size_t num_shaders)
{
	id = glCreateProgram();
	
	for (uint32_t i = 0; i < num_shaders; i++)
		glAttachShader(id, shaders[i].id);

	glLinkProgram(id);
	
	int32_t result;
	glGetProgramiv(id, GL_LINK_STATUS, &result);

	if (result == GL_TRUE) {
		mvp_location = glGetUniformLocation(id, "MVP");
		LEARY_LOGF(eLogType::Info, "Linked OpenGL shader program %d", id);

		return true;
	} else {
		int32_t result_log_length;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &result_log_length);

		char *result_log = new char[static_cast<size_t>(result_log_length + 1)];
		glGetProgramInfoLog(id, result_log_length, NULL, result_log);

		LEARY_LOGF(eLogType::Error,
				  "Failed to link OpenGL shader program %d: %s",
				  id, result_log);

		delete[] result_log;
	}
		
	return false;
}

void OpenGLProgram::destroy()
{
	glDeleteProgram(id);
}

bool OpenGLShader::create(uint32_t type, const char* const filename)
{
	std::string file_path = Environment::resolvePath(eEnvironmentFolder::GameData, "shaders/") +
		                    filename;

	std::string   shader_source;
	std::ifstream shader_source_stream(file_path, std::ios::in);

	if (shader_source_stream.is_open()) {
		std::string line = "";
		while (getline(shader_source_stream, line))
			shader_source += "\n" + line;

		shader_source_stream.close();
	} else {
		LEARY_LOGF(eLogType::Error, 
		          "Failed to open shader file: %s", 
		          file_path.c_str());
		return 0;
	}

	id = glCreateShader(type);

	char const * shader_source_ptr = shader_source.c_str();
	glShaderSource(id, 1, &shader_source_ptr, NULL);

	glCompileShader(id);

	int32_t result = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);

	if (result == GL_TRUE) {
		this->type = type;
		LEARY_LOGF(eLogType::Info, "Compiled shader %s", file_path.c_str());

		return true;
	} 

	int32_t result_log_length;
	glGetShaderiv(id, GL_INFO_LOG_LENGTH, &result_log_length);

	LEARY_ASSERT(result_log_length >= 0);
	
	char *result_log = new char[static_cast<size_t>(result_log_length + 1)];
	glGetShaderInfoLog(id, result_log_length, NULL, result_log);

	LEARY_LOGF(eLogType::Error,
		"Failed to compile shader %s %s", file_path.c_str(), result_log);

	delete[] result_log;
	
	return false;
}

void OpenGLShader::destroy()
{
	glDeleteShader(id);
}
