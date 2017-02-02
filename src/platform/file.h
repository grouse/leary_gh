/**
 * @file:   file.h
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

#ifndef LEARY_FILE_H
#define LEARY_FILE_H

#if defined(_WIN32)
	#define FILE_SEP "\\"
	#define FILE_EOL "\r\n"
#elif defined(__linux__)
	#define FILE_SEP "/"
	#define FILE_EOL "\n"
#else
	#error "unsupported platform"
#endif

struct PlatformState;

enum class EnvironmentFolder {
	preferences,
	game_data
};

enum class FileMode {
	read,
	write,
	read_write
};

void  init_platform_paths(PlatformState *state);

bool  file_exists(const char *path);
bool  file_create(const char *path);

void* file_open(const char *path, FileMode mode);
void  file_close(void *file_handle);

void  file_write(void *file_handle, void *buffer, size_t bytes);

char *platform_read_file(const char* path, size_t *out_size);
char *platform_resolve_relative(const char *path);

#endif // LEARY_FILE_H
