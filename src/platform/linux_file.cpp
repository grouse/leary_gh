/**
 * @file:   linux_file.cpp
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

#include "file.h"

#include <memory>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <pwd.h>

void init_platform_paths(PlatformState *state)
{
	VAR_UNUSED(state);
}

bool file_exists(const char *path)
{
	VAR_UNUSED(path);
	return false;
}

bool file_create(const char *path)
{
	VAR_UNUSED(path);
	return false;
}

void* file_open(const char *path, FileMode mode)
{
	VAR_UNUSED(path);
	VAR_UNUSED(mode);
	return nullptr;
}

void file_close(void *file_handle)
{
	VAR_UNUSED(file_handle);
}

void* file_read(const char *filename, size_t *file_size)
{
	VAR_UNUSED(filename);
	VAR_UNUSED(file_size);
	return nullptr;
}

void file_write(void *file_handle, void *buffer, size_t bytes)
{
	VAR_UNUSED(file_handle);
	VAR_UNUSED(buffer);
	VAR_UNUSED(bytes);
}
