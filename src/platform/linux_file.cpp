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

#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>

#include "platform/platform_main.h"

char *platform_path(GamePath root)
{
	char *path = nullptr;

	switch (root) {
	case GamePath_data: {
		char *data_env = getenv("LEARY_DATA_ROOT");
		if (data_env) {
			u64 length = strlen(data_env) + 1;
			if (data_env[length-1] != '/') {
				path = (char*)malloc(length + 1);
				strcpy(path, data_env);
				path[length-1] = '/';
				path[length] = '\0';
			} else {
				path = (char*)malloc(length);
				strcpy(path, data_env);
				path[length-1] = '\0';
			}
		} else {
			char linkname[64];
			pid_t pid = getpid();
			i64 result = snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);
			DEBUG_ASSERT(result >= 0);

			char buffer[PATH_MAX];
			i64 length = readlink(linkname, buffer, PATH_MAX);
			DEBUG_ASSERT(length >= 0);

			for (; length >= 0; length--) {
				if (buffer[length-1] == '/') {
					break;
				}
			}

			u64 total_length = length + strlen("data/") + 1;
			path = (char*)malloc(total_length);
			strncpy(path, buffer, length);
			strcat(path, "data/");
			path[length-1] = '\0';
		}
	} break;
	case GamePath_shaders: {
		char *data_path = platform_path(GamePath_data);
		u64 length = strlen(data_path) + strlen("shaders/") + 1;
		path = (char*)malloc(length);
		strcpy(path, data_path);
		strcat(path, "shaders/");
		path[length-1] = '\0';
	} break;
	case GamePath_preferences: {
		char *local_share = getenv("XDG_DATA_HOME");
		if (local_share) {
			u64 length = strlen(local_share) + strlen("leary/") + 1;
			path = (char*)malloc(length);
			strcpy(path, local_share);
			strcat(path, "leary/");
			path[length-1] = '\0';
		} else {
			struct passwd *pw = getpwuid(getuid());
			u64 length = strlen(pw->pw_dir) + strlen("/.local/share/leary/") + 1;
			path = (char*)malloc(length);
			strcpy(path, pw->pw_dir);
			strcat(path, "/.local/share/leary/");
			path[length-1] = '\0';
		}
	} break;
	default:
		DEBUG_ASSERT(false);
		break;
	}

	return path;
}

char *platform_resolve_path(const char *path)
{
	return realpath(path, nullptr);
}

char *platform_resolve_path(GamePath root, const char *path)
{
	static const char *data_path        = platform_path(GamePath_data);
	static const char *shaders_path     = platform_path(GamePath_shaders);
	static const char *preferences_path = platform_path(GamePath_preferences);

	static const u64 data_path_length        = strlen(data_path);
	static const u64 shaders_path_length     = strlen(shaders_path);
	static const u64 preferences_path_length = strlen(preferences_path);

	char *resolved = nullptr;

	switch (root) {
	case GamePath_data:
		resolved = (char*)malloc(data_path_length + strlen(path));
		strcpy(resolved, data_path);
		strcat(resolved, path);
		break;
	case GamePath_shaders:
		resolved = (char*)malloc(shaders_path_length + strlen(path));
		strcpy(resolved, shaders_path);
		strcat(resolved, path);
		break;
	case GamePath_preferences:
		resolved = (char*)malloc(preferences_path_length + strlen(path));
		strcpy(resolved, preferences_path);
		strcat(resolved, path);
		break;
	default:
		DEBUG_ASSERT(false);
		break;
	}

	return resolved;
}



bool platform_file_exists(const char *path)
{
	struct stat st;
	return (stat(path, &st) == 0);
}

bool platform_file_create(const char *path)
{
	// TODO(jesper): create folders if needed
	i32 access = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	i32 fd = open(path, O_CREAT, access);

	if (fd >= 0) {
		close(fd);
	}

	return fd >= 0;
}



void* platform_file_open(const char *path, FileAccess access)
{
	i32 flags = 0;

	switch (access) {
	case FileAccess_read:
		flags = O_RDONLY;
		break;
	case FileAccess_write:
		flags = O_WRONLY;
		break;
	case FileAccess_read_write:
		flags = O_RDWR;
		break;
	default:
		DEBUG_ASSERT(false);
		return nullptr;
	}

	i32 fd = open(path, flags);
	if (fd < 0) {
		return nullptr;
	}

	return (void*)fd;
}

void platform_file_close(void *file_handle)
{
	i32 fd = (i32)(i64)file_handle;
	DEBUG_ASSERT(fd >= 0);

	close(fd);
}



void platform_file_write(void *file_handle, void *buffer, usize bytes)
{
	i32 fd = (i32)(i64)file_handle;
	isize written = write(fd, buffer, bytes);
	DEBUG_ASSERT(written >= 0);
	DEBUG_ASSERT((usize)written == bytes);
}

char* platform_file_read(const char *path, usize *file_size)
{
	struct stat st;
	i32 result = stat(path, &st);
	DEBUG_ASSERT(result == 0);

	char *buffer = (char*)malloc(st.st_size);

	i32 fd = open(path, O_RDONLY);
	DEBUG_ASSERT(fd >= 0);

	isize bytes_read = read(fd, buffer, st.st_size);
	DEBUG_ASSERT(bytes_read == st.st_size);
	*file_size = bytes_read;

	return buffer;
}

