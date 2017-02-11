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

enum FileAccess {
	FileAccess_read,
	FileAccess_write,
	FileAccess_read_write
};

enum GamePath {
	GamePath_data,
	GamePath_shaders,
	GamePath_fonts,
	GamePath_preferences
};

char *platform_path(GamePath root);
char *platform_resolve_path(const char *path);
char *platform_resolve_path(GamePath root, const char *path);

bool  platform_file_exists(const char *path);
bool  platform_file_create(const char *path);

void* platform_file_open(const char *path, FileAccess access);
void  platform_file_close(void *file_handle);

void  platform_file_write(void *file_handle, void *buffer, usize bytes);
char *platform_file_read(const char* path, usize *out_size);

#endif // LEARY_FILE_H
