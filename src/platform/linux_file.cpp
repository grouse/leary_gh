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


void init_platform_paths(PlatformState *state)
{
	// TODO(jesper): deal with cases where the absolute path to the binary
	// exceeds PATH_MAX bytes, haven't figured out a good way to do this
	char buffer[PATH_MAX];
	int result;

	char *data_env = getenv("LEARY_DATA_ROOT");
	if (data_env) {
		size_t length = strlen(data_env);
		state->folders.game_data = (char*)malloc(length + 1);
		state->folders.game_data_length = length;
		strcpy(state->folders.game_data, data_env);
	} else {
		char linkname[64];
		pid_t pid = getpid();
		result = snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);
		DEBUG_ASSERT(result >= 0);

		ssize_t length = readlink(linkname, buffer, sizeof(buffer));
		DEBUG_ASSERT(result >= 0);

		for (; length >= 0; length--) {
			if (buffer[length-1] == '/') {
				break;
			}
		}

		state->folders.game_data = (char*)malloc(length + strlen("data") + 1);
		strncpy(state->folders.game_data, buffer, length);
		strcat(state->folders.game_data, "data");
		state->folders.game_data_length = strlen(state->folders.game_data);
	}

	char *local_share = getenv("XDG_DATA_HOME");
	if (local_share == nullptr) {
		struct passwd *pw = getpwuid(getuid());
		result = snprintf(buffer, sizeof(buffer), "%s/.local/share", pw->pw_dir);
		DEBUG_ASSERT(result >= 0);

		local_share = buffer;
	}

	const char *suffix = "/leary";
	size_t suffix_length = strlen(suffix);

	state->folders.preferences = (char*)malloc(strlen(local_share) + suffix_length + 1);
	strcpy(state->folders.preferences, local_share);
	strcat(state->folders.preferences, suffix);

	state->folders.preferences_length = strlen(state->folders.preferences);
}

bool file_exists(const char *path)
{
	struct stat st;
	return (stat(path, &st) == 0);
}

bool file_create(const char *path)
{
	int fd = open(path, O_CREAT);

	if (fd >= 0) {
		close(fd);
	}

	return fd >= 0;
}

void* file_open(const char *path, FileMode mode)
{
	int flags = 0;

	switch (mode) {
	case FileMode::read:
		flags = O_RDONLY;
		break;
	case FileMode::write:
		flags = O_WRONLY;
		break;
	case FileMode::read_write:
		flags = O_RDWR;
		break;
	default:
		DEBUG_ASSERT(false);
		return nullptr;
	}

	int fd = open(path, flags);
	if (fd < 0) {
		return nullptr;
	}

	return (void*)fd;
}

void file_close(void *file_handle)
{
	int fd = (int)(int64_t)file_handle;
	DEBUG_ASSERT(fd >= 0);

	close(fd);
}

void* file_read(const char *path, size_t *file_size)
{
	struct stat st;
	int result = stat(path, &st);
	DEBUG_ASSERT(result == 0);

	char *buffer = (char*)malloc(st.st_size);

	int fd = open(path, O_RDONLY);
	DEBUG_ASSERT(fd >= 0);

	ssize_t bytes_read = read(fd, buffer, st.st_size);
	DEBUG_ASSERT(bytes_read == st.st_size);
	*file_size = bytes_read;

	return buffer;
}

void file_write(void *file_handle, void *buffer, size_t bytes)
{
	int fd = (int)(int64_t)file_handle;
	ssize_t written = write(fd, buffer, bytes);
	DEBUG_ASSERT(written >= 0);
	DEBUG_ASSERT((size_t)written == bytes);
}
