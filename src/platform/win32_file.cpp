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

#include "platform_file.h"

char *platform_path(GamePath root)
{
	char *path = nullptr;

	switch (root) {
	case GamePath_data: {
		TCHAR buffer[MAX_PATH];
		DWORD env_length = GetEnvironmentVariable("LEARY_GAME_DATA",
		                                          buffer,
		                                          MAX_PATH);

		if (env_length != 0) {
			path = (char*)malloc(env_length + 1);
			strcpy(path, buffer);
			path[env_length-1] = '\0';
		} else {
			DWORD module_length = GetModuleFileName(NULL, buffer, MAX_PATH);

			if (module_length != 0) {
				if (PathRemoveFileSpec(buffer) == TRUE)
					module_length = (DWORD) strlen(buffer);

				i64 length = module_length + strlen("\\data\\") + 1;
				path = (char*)malloc(length);
				strcpy(path, buffer);
				strcat(path, "\\data\\");
				path[length-1] = '\0';
			}
		}
	} break;
	case GamePath_shaders: {
		char *data_path = platform_path(GamePath_data);
		i64 length = strlen(data_path) + strlen("shaders\\") + 1;
		path = (char*)malloc(length);
		strcpy(path, data_path);
		strcat(path, "shaders\\");
		path[length-1] = '\0';
	} break;
	case GamePath_models: {
		char *data_path = platform_path(GamePath_data);
		u64 length = strlen(data_path) + strlen("models/") + 1;
		path = (char*)malloc(length);
		strcpy(path, data_path);
		strcat(path, "models/");
	} break;
	case GamePath_preferences: {
		TCHAR buffer[MAX_PATH];
		HRESULT result = SHGetFolderPath(NULL,
		                                 CSIDL_LOCAL_APPDATA,
		                                 NULL,
		                                 SHGFP_TYPE_CURRENT,
		                                 buffer);
		if (result == S_OK) {
			i64 length = strlen(buffer) + strlen("\\leary\\") + 1;
			path = (char*)malloc(length);
			strcpy(path, buffer);
			strcat(path, "\\leary\\");
			path[length-1] = '\0';
		}
	} break;
	default:
		DEBUG_ASSERT(false);
		return nullptr;
	}

	return path;
}

char *platform_resolve_path(const char *path)
{
	DWORD result;

	char buffer[MAX_PATH];
	result = GetFullPathName(path, MAX_PATH, buffer, nullptr);
	DEBUG_ASSERT(result > 0);
	return strdup(buffer);
}

char *platform_resolve_path(GamePath root, const char *path)
{
	char *resolved = nullptr;

	static const char *data_path        = platform_path(GamePath_data);
	static const char *models_path      = platform_path(GamePath_models);
	static const char *shaders_path     = platform_path(GamePath_shaders);
	static const char *preferences_path = platform_path(GamePath_preferences);

	static const u64 data_path_length        = strlen(data_path);
	static const u64 models_path_length      = strlen(models_path);
	static const u64 shaders_path_length     = strlen(shaders_path);
	static const u64 preferences_path_length = strlen(preferences_path);

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
	case GamePath_models: {
		usize length = models_path_length + strlen(path) + 1;
		resolved = (char*)malloc(length);
		strcpy(resolved, models_path);
		strcat(resolved, path);
	} break;
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
	return PathFileExists(path) == TRUE;
}

bool platform_file_create(const char *path)
{
	HANDLE file_handle = CreateFile(path,
	                                0,
	                                0,
	                                NULL,
	                                CREATE_NEW,
	                                FILE_ATTRIBUTE_NORMAL,
	                                NULL);

	if (file_handle == INVALID_HANDLE_VALUE) {
		return false;
	}

	CloseHandle(file_handle);
	return true;
}

void* platform_file_open(const char *path, FileAccess access)
{
	DWORD flags;
	DWORD share_mode;

	switch (access) {
	case FileAccess_read:
		flags      = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		break;
	case FileAccess_write:
		flags      = GENERIC_WRITE;
		share_mode = 0;
		break;
	case FileAccess_read_write:
		flags      = GENERIC_READ | GENERIC_WRITE;
		share_mode = 0;
		break;
	default:
		DEBUG_ASSERT(false);
		return nullptr;
	}

	HANDLE file_handle = CreateFile(path,
	                                flags,
	                                share_mode,
	                                NULL,
	                                OPEN_EXISTING,
	                                FILE_ATTRIBUTE_NORMAL,
	                                NULL);

	if (file_handle == INVALID_HANDLE_VALUE) {
		return nullptr;
	}

	return (void*)file_handle;
}

void platform_file_close(void *file_handle)
{
	CloseHandle((HANDLE)file_handle);
}

char* platform_file_read(const char *filename, usize *size)
{
	char *buffer = nullptr;

	HANDLE file = CreateFile(filename,
	                         GENERIC_READ,
	                         FILE_SHARE_READ,
	                         NULL,
	                         OPEN_EXISTING,
	                         FILE_ATTRIBUTE_NORMAL,
	                         NULL);

	LARGE_INTEGER file_size;
	if (GetFileSizeEx(file, &file_size)) {
		// NOTE(jesper): ReadFile only works on 32 bit sizes
		DEBUG_ASSERT(file_size.QuadPart <= 0xFFFFFFFF);
		*size = (usize) file_size.QuadPart;


		buffer = (char*)malloc((usize)file_size.QuadPart);

		DWORD bytes_read;
		if (!ReadFile(file, buffer, (u32)file_size.QuadPart, &bytes_read, 0)) {
			free(buffer);
			buffer = nullptr;
		}

		CloseHandle(file);
	}

	return buffer;
}

void platform_file_write(void *file_handle, void *buffer, usize bytes)
{
	BOOL result;

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

char *platform_resolve_relative(const char *path)
{
	DWORD result;

	char buffer[MAX_PATH];
	result = GetFullPathName(path, MAX_PATH, buffer, nullptr);
	DEBUG_ASSERT(result > 0);
	return strdup(buffer);
}
