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
#include <pwd.h>
#endif

#if LEARY_WIN
#include <io.h>

#include <Windows.h>
#include <Shlwapi.h>
#include <Shlobj.h>

#include <codecvt>
#endif

#include <memory>

const char* get_data_base_path(void);
const char* get_preferences_base_path(void);

std::string Environment::resolvePath(eEnvironmentFolder type, const char* filename)
{
	std::string resolved = "";

	static const std::unique_ptr<const char> data_base_path       (get_data_base_path());
	static const std::unique_ptr<const char> preferences_base_path(get_preferences_base_path());

	switch (type) {
	case eEnvironmentFolder::GameData:
	{
		resolved += data_base_path.get();
		resolved += "/data/";
		break;
	}
	case eEnvironmentFolder::UserPreferences:
	{
		resolved += preferences_base_path.get();
		resolved += "/grouse_games/preferences/";
		break;
	}
	default:
		LEARY_LOGF(eLogType::Warning,
		          "Unhandled Environment Folder type: %d", type);
		break;
	}

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
	if (access(path, 0) == 0) {
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
#elif LEARY_LINUX
	const size_t len = strlen(path);
	char* tmp = new char[len + 1];
	strcpy(tmp, path);

	if (tmp[len - 1] == '/')
		tmp[len - 1] = '\0';

	for (char* p = tmp + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(tmp, 0755);
			*p = '/';
		}	
	}

	int result = mkdir(tmp, 0775);
	LEARY_UNUSED(result);

#if LEARY_DEBUG
	const char* error;

	switch (errno) {
	case EACCES:
		error = "Search permission is denied on a component of the path prefix, or write "
		        "permission is denied on the parent directory of the directory to be created.";
		break;
	case EEXIST:
		error = "The named file exists.";
		break;
	case ELOOP:
		error = "A loop exists in symbolic links encountered during resolution of the path "
		        "argument.";
		break;
	case EMLINK:
		error = "The link count of the parent directory would exceed {LINK_MAX}.";
		break;
	case ENAMETOOLONG:
		error = "The length of the path argument exceeds {PATH_MAX} or a pathname component is "
		        "longer than {NAME_MAX}";
		break;
	case ENOENT:
		error = "A component of the path prefix specified by path does not name an existing "
		        "directory or path is an empty string.";
		break;
	case ENOSPC:
		error = "The file system does not contain enough space to hold the contents of the new "
		        "directory or to extend the parent directory of the new directory.";
		break;
	case ENOTDIR:
		error = "A component of the path prefix is not a directory.";
		break;
	case EROFS:
		error = "The parent directory resides on a read-only file system.";
		break;
	default:
		error = "Unknown error";
		break;
	} 

	LEARY_ASSERT_PRINT(result == 0, error);
#endif
#else
	LEARY_UNIMPLEMENTED_FUNCTION;
	LEARY_UNUSED(path);
#endif
}

const char* get_data_base_path()
{
#if LEARY_WIN
	TCHAR   path[MAX_PATH];
	HMODULE hModule = GetModuleHandle(NULL);

	GetModuleFileName(hModule, path, MAX_PATH);
	PathRemoveFileSpec(path);

	char* retPath = new char[strlen(path)];
	strcpy(retPath, path);

	return retPath;
#elif LEARY_LINUX
	struct stat sb;
	stat("/proc/self/exe", &sb);

	char* path  = new char[sb.st_size + 1];
	ssize_t len = readlink("/proc/self/exe", path, sb.st_size + 1);
	path[len]   = '\0';

	// the path includes the exe name, so find last '/' and replace it with a terminating '\0'
	//     e.g. p/ath/to/exe -> /path/to\0exe
	char* pch        = strrchr(path, '/');
	path[pch - path] = '\0';

	return path;
#else
	LEARY_UNIMPLEMENTED_FUNCTION;
	return "";
#endif
}

const char* get_preferences_base_path()
{
#if LEARY_WIN
	LPWSTR path    = NULL;
	HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 
		                                  KF_FLAG_CREATE, 
		                                  NULL, 
		                                  &path);

	LEARY_ASSERT(result == S_OK);
	LEARY_UNUSED(result);

	std::wstring path_wstr = path;
	CoTaskMemFree(path);

	typedef std::codecvt_utf8<wchar_t> convert_type;
	std::wstring_convert<convert_type, wchar_t> converter;

	return converter.to_bytes(path_wstr);
#elif LEARY_LINUX
	char* env_var = getenv("XDG_DATA_HOME");
	if (env_var != NULL) {
		char* path = new char[strlen(env_var)];
		strcpy(path, env_var);

		return path;
	} else {
		struct passwd *pw = getpwuid(getuid());
		const char* append_path = "/.local/share";
		const size_t append_path_len = strlen(append_path);
		const size_t base_path_len   = strlen(pw->pw_dir);

		char* path = new char[append_path_len + base_path_len + 1];
		strcpy(path, pw->pw_dir);
		strcat(path, append_path);
		
		return path;
	}
#else
	LEARY_UNIMPLEMENTED_FUNCTION;
	return "";
#endif
}
