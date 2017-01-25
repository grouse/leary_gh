/**
 * @file:   settings.cpp
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

#include "settings.h"

#include <cstdio>
#include <cinttypes>

#include "platform/platform_main.h"
#include "platform/debug.h"
#include "platform/file.h"

Settings load_settings(const char* filename, PlatformState &platform_state)
{
	Settings settings;

	// TODO(jesper): choose some more reasonable buffer sizes
	char path[255];
	strcpy(path, platform_state.folders.preferences);
	strcat(path, FILE_SEP);
	strcat(path, filename);

	if (!file_exists(path)) return settings;

	size_t size;
	void *buffer = file_read(path, &size);

	if (buffer == nullptr) return settings;

	char *p   = (char*) buffer;
	char *end = ((char*) buffer) + size;

	// TODO(jesper): choose some more reasonable buffer sizes
	// NOTE(jesper): we could probably use one buffer here for both path, key and value to waste
	// less space, or just malloc/free inside the loop - it's hardly a critical path in performance
	// and it'd make sure we don't overflow in some weird edge case/bad user input
	char key[255];
	char value[255];

	while (p < end)
	{
		char *key_start = p;
		while (*p != '=' && *p != ' ') ++p;
		char *key_end = p;

		DEBUG_ASSERT((key_end - key_start) > 0);
		size_t key_length = (size_t) (key_end - key_start);

		memcpy(key, key_start, key_length);
		key[key_length] = '\0';

		while (p < end && (*p == '=' || *p == ' ')) ++p;

		char *value_start = p;
		while (p < end && *p != '\r' && *p != '\n' && *p != ' ') ++p;
		char *value_end = p;

		DEBUG_ASSERT((value_end - value_start) > 0);
		size_t value_length = (size_t) (value_end - value_start);

		memcpy(value, value_start, value_length);
		value[value_length] = '\0';

		// TODO(jesper): just a strcmp chain works for now, probably consider turning this into
		// something a bit smarter that can be folded into just one lookup
		// TODO(jesper): validate user input, making sure values are within expected range and that
		// they are the correct data type
		if (strcmp(key, "video.resolution.width") == 0)
			sscanf(value, "%" SCNd32, &settings.video.resolution.width);
		else if (strcmp(key, "video.resolution.height") == 0)
			sscanf(value, "%" SCNd32, &settings.video.resolution.height);
		else if (strcmp(key, "video.fullscreen") == 0)
			sscanf(value, "%" SCNd16, &settings.video.fullscreen);
		else if (strcmp(key, "video.vsync") == 0)
			sscanf(value, "%" SCNd16, &settings.video.vsync);
		else
			DEBUG_ASSERT(false);

		while (p < end && (*p == '\r' || *p == '\n')) ++p;
	}

	return settings;
}


void save_settings(Settings &settings, const char* filename, PlatformState &platform_state)
{
	// TODO(jesper): choose some more reasonable buffer sizes
	char path[255];
	strcpy(path, platform_state.folders.preferences);
	strcat(path, FILE_SEP);
	strcat(path, filename);

	if (!file_exists(path) && !file_create(path))
	{
		DEBUG_ASSERT(false);
		return;
	}

	void *file_handle = file_open(path, FileMode::write);

	// TODO(jesper): rewrite this so that we do fewer file_write, potentially so that we only open
	// file and write into it after we've got an entire buffer to write into it
	char *buffer = (char*)malloc(2048);

	int32_t bytes = sprintf(buffer, "%s = %d" FILE_EOL,
	                        "video.resolution.width", settings.video.resolution.width);
	file_write(file_handle, buffer, (size_t) bytes);

	bytes = sprintf(buffer, "%s = %d" FILE_EOL,
	                "video.resolution.height", settings.video.resolution.height);
	file_write(file_handle, buffer, (size_t) bytes);

	bytes = sprintf(buffer, "%s = %" PRId16 FILE_EOL,
	                "video.fullscreen", settings.video.fullscreen);
	file_write(file_handle, buffer, (size_t) bytes);

	bytes = sprintf(buffer, "%s = %" PRId16 FILE_EOL,
	                "video.vsync", settings.video.vsync);
	file_write(file_handle, buffer, (size_t) bytes);

	free(buffer);
	file_close(file_handle);
}
