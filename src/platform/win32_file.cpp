/**
 * @file:   win32_file.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2016-2017 Jesper Stefansson
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

#include "core/file.h"

#include "platform.h"
#include "leary_macros.h"

extern SystemAllocator *g_system_alloc;

struct PlatformPaths {
    String preferences;
    String data;
    String exe;
    String shaders;
    String textures;
    String models;
};

PlatformPaths g_paths;

void init_paths(Allocator *a)
{
    g_paths = {};

    HRESULT result;
    WCHAR buffer[MAX_PATH];

    // --- exe dir
    // NOTE(jesper): other paths depend on this being resolved early
    DWORD module_length = GetModuleFileNameW(NULL, buffer, MAX_PATH);

    if (module_length != 0) {
        if (PathRemoveFileSpecW(buffer) == TRUE)
            module_length = (DWORD) wcslen(buffer);

        buffer[module_length] = '\0';
        g_paths.exe = create_string(a, buffer, "\\");
    }

    // --- app data dir
    // NOTE(jesper): other paths depend on this being resolved early
    DWORD env_length = GetEnvironmentVariableW(L"LEARY_DATA_ROOT", buffer, MAX_PATH);

    if (env_length != 0) {
        buffer[env_length-1] = '\0';
        g_paths.data = create_string(a, buffer);
    } else {
        g_paths.data = create_string(a, g_paths.exe, "..\\assets\\");
    }

    // --- app preferences dir
    result = SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buffer);
    if (result == S_OK) {
        g_paths.preferences = create_string(a, buffer, "\\leary\\");
    }

    g_paths.shaders  = create_string(a, g_paths.exe, "data\\shaders\\");
    g_paths.textures = create_string(a, g_paths.data, "textures\\");
    g_paths.models   = create_string(a, g_paths.data, "models\\");
}

Array<Path> list_files(const char *folder, Allocator *allocator)
{
    Array<Path> files = {};
    files.allocator   = allocator;

    // TODO(jesper): ROBUSTNESS: better path length
    char path[2048];
    sprintf(path, "%s\\*.*", folder);

    isize dlen = strlen(folder);
    bool eslash = (folder[dlen-1] == '\\');

    HANDLE h = NULL;
    WIN32_FIND_DATA fd;
    h = FindFirstFile(path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        LOG(Log_error, "could not find file in folder: %s", folder);
        return {};
    }

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
            continue;
        }
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // TODO(jesper): handle sub-folders
            LOG(Log_warning, "sub-folders are unimplemented");
        } else {
            i32 flen = utf8_size(fd.cFileName);

            Path p = {};
            if (!eslash) {
                p.absolute = create_string(allocator, folder, "\\", fd.cFileName);
            } else {
                p.absolute = create_string(allocator, folder, fd.cFileName);
            }

            if (!eslash) {
                p.filename = { flen, p.absolute.bytes + dlen + 1 };
            } else {
                p.filename = { flen, p.absolute.bytes + dlen };
            }

            i32 ext = 0;
            for (i32 i = 0; i < p.filename.size; i++) {
                if (p.filename[i] == '.') {
                    ext = i;
                }
            }
            p.extension = { flen - ext, p.filename.bytes + ext + 1 };

            LOG("adding file: %s", p.absolute.bytes);
            array_add(&files, p);
        }
    } while (FindNextFile(h, &fd));

    FindClose(h);
    return files;
}

char* resolve_relative(const char *path)
{
    DWORD result;

    char buffer[MAX_PATH];
    result = GetFullPathName(path, MAX_PATH, buffer, nullptr);
    ASSERT(result > 0);

    // TODO(jesper): currently leaking memory
    return strdup(buffer);
}

char* resolve_path(GamePath rp, const char *path, Allocator *a)
{
    usize length, plength;
    char *resolved;
    char *p, *root;

    plength = strlen(path);

    switch (rp) {
    case GamePath_data: {
        length = g_paths.data.size + plength;
        root   = g_paths.data.bytes;
    } break;
    case GamePath_exe: {
        length = g_paths.exe.size + plength;
        root   = g_paths.exe.bytes;
    } break;
    case GamePath_shaders: {
        length = g_paths.shaders.size + plength;
        root   = g_paths.shaders.bytes;
    } break;
    case GamePath_textures: {
        length = g_paths.textures.size + plength;
        root   = g_paths.textures.bytes;
    } break;
    case GamePath_models: {
        length = g_paths.models.size + plength;
        root   = g_paths.models.bytes;
    } break;
    case GamePath_preferences: {
        length = g_paths.preferences.size + plength;
        root   = g_paths.preferences.bytes;
    } break;
    default:
        LOG(Log_error, "unknown path root: %d", rp);
        ASSERT(false);
        return nullptr;
    }

    resolved = (char*)a->alloc(length + 1);
    p        = strcpy(resolved, root);
    strcat(p, path);

    for (i32 i = 0; i < length; i++) {
        if (resolved[i] == '/') {
            resolved[i] = '\\';
        }
    }

    return resolved;
}

bool file_exists(const char *path)
{
    return PathFileExists(path) == TRUE;
}

bool folder_exists(const char *path)
{
    DWORD r = GetFileAttributes(path);
    return (r != INVALID_FILE_ATTRIBUTES && (r & FILE_ATTRIBUTE_DIRECTORY));
}

bool create_file(const char *path, bool create_folders = false)
{
    if (create_folders) {
        Path p = create_file_path(g_system_alloc, path);

        char folder[MAX_PATH];
        strncpy(folder, p.absolute.bytes, p.absolute.size - p.filename.size - 1);

        if (!folder_exists(folder)) {
            int result = SHCreateDirectoryEx(NULL, folder, NULL);
            if (result != ERROR_SUCCESS) {
                LOG(Log_error, "couldn't create folders: %s", folder);
                return false;
            }
        }
    }


    HANDLE file_handle = CreateFile(path, 0, 0, NULL, CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    CloseHandle(file_handle);
    return true;
}

void* open_file(const char *path, FileAccess access)
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
        ASSERT(false);
        return nullptr;
    }

    HANDLE file_handle = CreateFile(path, flags, share_mode, NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    return (void*)file_handle;
}

void close_file(void *file_handle)
{
    CloseHandle((HANDLE)file_handle);
}

char* read_file(const char *filename, usize *o_size, Allocator *a)
{
    char *buffer = nullptr;

    HANDLE file = CreateFile(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (file == INVALID_HANDLE_VALUE) {
        LOG(Log_error,
            "failed to open file %s - %s",
            filename, win32_system_error_message(GetLastError()));
        return nullptr;
    }

    LARGE_INTEGER file_size;
    if (GetFileSizeEx(file, &file_size)) {
        // NOTE(jesper): ReadFile only works on 32 bit sizes
        ASSERT(file_size.QuadPart <= 0xFFFFFFFF);
        *o_size = (usize) file_size.QuadPart;

        buffer = (char*)a->alloc((usize)file_size.QuadPart);
        if (buffer == nullptr) {
            LOG(Log_error,
                "failed to allocate %d bytes from allocator for file %s",
                file_size.QuadPart, filename);
            return nullptr;
        }

        DWORD bytes_read;
        if (!ReadFile(file, buffer, (u32)file_size.QuadPart, &bytes_read, 0)) {
            a->dealloc(buffer);
            buffer = nullptr;
        }

        CloseHandle(file);
    }

    return buffer;
}

void write_file(void *file_handle, void *buffer, usize bytes)
{
    BOOL result;

    // NOTE(jesper): WriteFile takes 32 bit number of bytes to write
    ASSERT(bytes <= 0xFFFFFFFF);

    DWORD bytes_written;
    result = WriteFile((HANDLE) file_handle,
                            buffer,
                            (DWORD) bytes,
                            &bytes_written,
                            NULL);


    ASSERT(bytes  == bytes_written);
    ASSERT(result == TRUE);
}

