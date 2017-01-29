/**
 * @file:   win32_file.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016 Jesper Stefansson
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

#include <Shlobj.h>
#include <Shlwapi.h>

#include "debug.h"
#include "platform_main.h"

void init_platform_paths(PlatformState *state)
{
	TCHAR buffer[MAX_PATH];

	HRESULT result = SHGetFolderPath(NULL,
	                                 CSIDL_LOCAL_APPDATA,
	                                 NULL,
	                                 SHGFP_TYPE_CURRENT,
	                                 buffer);
	if (result == S_OK)
	{
		const char *suffix = "\\leary";

		size_t suffix_length = strlen(suffix);
		size_t base_length   = strlen(buffer);

		state->folders.preferences = (char*) malloc(base_length + suffix_length + 1);
		strcpy(state->folders.preferences, buffer);
		strcat(state->folders.preferences + base_length, suffix);

		state->folders.preferences_length = strlen(state->folders.preferences);
	}

	DWORD env_length = GetEnvironmentVariable("LEARY_GAME_DATA",
	                                          buffer,
	                                          MAX_PATH);

	if (env_length != 0)
	{
		state->folders.game_data = (char*) malloc(env_length + 1);
		strcpy(state->folders.game_data, buffer);

		state->folders.game_data_length = strlen(state->folders.game_data);
	}
	else
	{
		DWORD module_length = GetModuleFileName(NULL, buffer, MAX_PATH);

		if (module_length != 0)
		{
			if (PathRemoveFileSpec(buffer) == TRUE)
				module_length = (DWORD) strlen(buffer);

			const char *suffix = "\\data";

			state->folders.game_data = (char*) malloc(module_length + strlen(suffix) + 1);
			strcpy(state->folders.game_data, buffer);
			strcat(state->folders.game_data + module_length, suffix);

			state->folders.game_data_length = strlen(state->folders.game_data);
		}
	}
}


bool file_exists(const char *path)
{
	return PathFileExists(path) == TRUE;
}

bool file_create(const char *path)
{
	HANDLE file_handle = CreateFile(path,
	                                0,
	                                0,
	                                NULL,
	                                CREATE_NEW,
	                                FILE_ATTRIBUTE_NORMAL,
	                                NULL);

	if (file_handle == INVALID_HANDLE_VALUE)
		return false;

	CloseHandle(file_handle);
	return true;
}

void* file_open(const char *path, FileMode mode)
{
	DWORD access;
	DWORD share_mode;

	switch (mode) {
	case FileMode::read:
		access     = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		break;
	case FileMode::write:
		access     = GENERIC_WRITE;
		share_mode = 0;
		break;
	case FileMode::read_write:
		access     = GENERIC_READ | GENERIC_WRITE;
		share_mode = 0;
		break;
	default:
		DEBUG_ASSERT(false);
		return nullptr;
	}

	HANDLE file_handle = CreateFile(path,
	                                access,
	                                share_mode,
	                                NULL,
	                                OPEN_EXISTING,
	                                FILE_ATTRIBUTE_NORMAL,
	                                NULL);

	if (file_handle == INVALID_HANDLE_VALUE)
		return nullptr;

	return (void*) file_handle;
}

void file_close(void *file_handle)
{
	CloseHandle((HANDLE)file_handle);
}

void* file_read(const char *filename, size_t *size)
{
	void *buffer = nullptr;

	HANDLE file = CreateFile(filename,
	                         GENERIC_READ,
	                         FILE_SHARE_READ,
	                         NULL,
	                         OPEN_EXISTING,
	                         FILE_ATTRIBUTE_NORMAL,
	                         NULL);

	LARGE_INTEGER file_size;
	if (GetFileSizeEx(file, &file_size))
	{
		// NOTE(jesper): ReadFile only works on 32 bit sizes
		DEBUG_ASSERT(file_size.QuadPart <= 0xFFFFFFFF);
		*size = (size_t) file_size.QuadPart;


		buffer = malloc((size_t)file_size.QuadPart);

		DWORD bytes_read;
		if (!ReadFile(file, buffer, (u32)file_size.QuadPart, &bytes_read, 0))
		{
			free(buffer);
			buffer = nullptr;
		}

		CloseHandle(file);
	}

	return buffer;
}

void file_write(void *file_handle, void *buffer, size_t bytes)
{
	BOOL result;
	VAR_UNUSED(result);

	// NOTE(jesper): WriteFile takes 32 bit number of bytes to write
	DEBUG_ASSERT(bytes <= 0xFFFFFFFF);

	DWORD bytes_written;
	result = WriteFile((HANDLE) file_handle,
	                        buffer,
	                        (DWORD) bytes,
	                        &bytes_written,
	                        NULL);


	DEBUG_ASSERT(bytes  == bytes_written);
	DEBUG_ASSERT(result == TRUE);
}

