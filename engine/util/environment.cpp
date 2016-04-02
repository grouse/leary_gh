/**
 * @file:   environment.h
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

#include "environment.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm>

#if LEARY_LINUX
#include <unistd.h> 
#endif

#if LEARY_WIN
#include <io.h>

#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include <codecvt>
#endif

std::string Environment::resolvePath(eEnvironmentFolder type, const char* filename)
{
	std::string resolved = "";

#if LEARY_WIN
	switch (type) {
	case eEnvironmentFolder::GameData:
	{
		TCHAR   path[MAX_PATH];
		HMODULE hModule = GetModuleHandle(NULL);

		GetModuleFileName(hModule, path, MAX_PATH);
		PathRemoveFileSpec(path);

		resolved += path;
		resolved += "/data/";

		break;
	}
	case eEnvironmentFolder::UserPreferences:
	{
		LPWSTR path = NULL;
		HRESULT result = 
			SHGetKnownFolderPath(FOLDERID_LocalAppData, 
			                     KF_FLAG_CREATE, 
			                     NULL, 
			                     &path);

		LEARY_ASSERT(result == S_OK);
		LEARY_UNUSED(result);

		std::wstring path_wstr = path;

		typedef std::codecvt_utf8<wchar_t> convert_type;
		std::wstring_convert<convert_type, wchar_t> converter;

		resolved += converter.to_bytes(path_wstr);
		resolved += "/grouse_games/preferences/";

		break;
	}
	default:
		LEARY_LOGF(eLogType::Warning,
		          "Unhandled Environment Folder type: %d", type);
		break;
	}
#else
	LEARY_UNIMPLEMENTED_FUNCTION;
	LEARY_UNUSED(type);
#endif
	
	// fix mixed path separators
#if LEARY_WIN
	std::replace(resolved.begin(), resolved.end(), '/', '\\');
#else
	std::replace(resolved.begin(), resolved.end(), '\\', '/');
#endif

	// create folder if it doesn't exist
	if (!Environment::directoryExists(resolved.c_str()))
		Environment::createDirectory(resolved.c_str());
	
	resolved += filename;
	return resolved;
}

bool Environment::directoryExists(const char* path)
{
	if (access(path, 0) == 0)
	{
		struct stat status;
		stat(path, &status);

		return (status.st_mode & S_IFDIR) != 0;
	}
	return false;
}

void Environment::createDirectory(const char* path)
{
#if LEARY_WIN
	int result = SHCreateDirectoryEx(NULL, path, NULL);
	LEARY_UNUSED(result);

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

	LEARY_ASSERT_PRINT(result == ERROR_SUCCESS, error);
#endif
#else
	LEARY_UNIMPLEMENTED_FUNCTION;
	LEARY_UNUSED(path);
#endif
}