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

#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>

#include <io.h>

#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include <codecvt>

#include <memory>

#include "platform/debug.h"

namespace {
	const char* get_data_base_path(void);
	const char* get_preferences_base_path(void);
}

std::string 
resolve_path(EnvironmentFolder type, const char *filename)
{
	std::string resolved = "";

	static const std::unique_ptr<const char> data_base_path       (get_data_base_path());
	static const std::unique_ptr<const char> preferences_base_path(get_preferences_base_path());

	switch (type) {
	case EnvironmentFolder::GameData:
	{
		resolved += data_base_path.get();
		resolved += "/data/";
		break;
	}
	case EnvironmentFolder::UserPreferences:
	{
		resolved += preferences_base_path.get();
		resolved += "/grouse_games/preferences/";
		break;
	}
	default:
		DEBUG_LOGF(LogType::warning,
		           "Unhandled Environment Folder type: %d", type);
		break;
	}

	// fix mixed path separators
	std::replace(resolved.begin(), resolved.end(), '/', '\\');

	// create folder if it doesn't exist
	if (!directory_exists(resolved.c_str()))
		create_directory(resolved.c_str());
	
	resolved += filename;
	return resolved;
}

bool 
directory_exists(const char *path)
{
	if (access(path, 0) == 0) {
		struct stat status;
		stat(path, &status);

		return (status.st_mode & S_IFDIR) != 0;
	}
	return false;
}

void 
create_directory(const char* path)
{
	int result = SHCreateDirectoryEx(NULL, path, NULL);
	VAR_UNUSED(result);

#if LEARY_DEBUG
	const char* error;
	switch (result) {
	case ERROR_BAD_PATHNAME:
		error = "ERROR_BAD_PATHNAME";
		break;
	case ERROR_FILENAME_EXCED_RANGE:
		error = "ERROR_FILENAME_EXCED_RANGE";
		break;
	case ERROR_PATH_NOT_FOUND:
		error = "ERROR_PATH_NOT_FOUND";
		break;
	case ERROR_FILE_EXISTS:
		error = "ERROR_FILE_EXISTS";
		break;
	case ERROR_ALREADY_EXISTS:
		error = "ERROR_ALREADY_EXISTS";
		break;
	case ERROR_CANCELLED:
		error = "ERROR_CANCELLED";
		break;
	default:
		error = "Unknown error";
		break;
	}

	DEBUG_ASSERT(result == ERROR_SUCCESS && error);
#endif
}

namespace {
	const char* 
	get_data_base_path()
	{
		TCHAR   path[MAX_PATH];
		HMODULE hModule = GetModuleHandle(NULL);

		GetModuleFileName(hModule, path, MAX_PATH);
		PathRemoveFileSpec(path);

		char* retPath = new char[strlen(path)];
		strcpy(retPath, path);

		return retPath;
	}

	const char* 
	get_preferences_base_path()
	{
		LPWSTR path    = NULL;
		HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 
		                                      KF_FLAG_CREATE, 
		                                      NULL, 
		                                      &path);

		DEBUG_ASSERT(result == S_OK);
		VAR_UNUSED(result);

		std::wstring path_wstr = path;
		CoTaskMemFree(path);

		typedef std::codecvt_utf8<wchar_t> convert_type;
		std::wstring_convert<convert_type, wchar_t> converter;

		return converter.to_bytes(path_wstr).c_str();
	}
}
