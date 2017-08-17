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

#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <dirent.h>

#include "core/array.h"
#include "platform_file.h"

Array<Path> list_files(const char *folder, Allocator *allocator)
{
    // TODO: this should maybe return an array of File which might include full
    // path, filename, and extension?
    Array<Path> files = {};
    files.allocator = allocator;

    isize flen   = strlen(folder);
    bool  eslash = (folder[flen-1] == '/');

    DIR *d = opendir(folder);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            // TODO(jesper): dir with sub-folders
            if (dir->d_type == DT_REG) {
                isize dlen = strlen(dir->d_name);

                Path p;

                if (!eslash) {
                    isize length = flen + dlen + 2;
                    p.absolute = { length, (char*)allocator->alloc(length) };

                    strcpy(p.absolute.bytes, folder);
                    p.absolute[flen]   = '/';
                    p.absolute[flen+1] = '\0';
                } else {
                    isize length = flen + dlen + 1;
                    p.absolute   = { length, (char*)allocator->alloc(length) };
                    strcpy(p.absolute.bytes, folder);
                }
                strcat(p.absolute.bytes, dir->d_name);

                p.filename = { dlen, p.absolute.bytes + flen + 1 };
                isize ext = 0;
                for (i32 i = 0; i < p.filename.length; i++) {
                    if (p.filename[i] == '.') {
                        ext = i;
                    }
                }
                p.extension = { dlen - ext, p.filename.bytes + ext };
                array_add(&files, p);
            } else {
                DEBUG_LOG("unimplemented d_type: %d", dir->d_type);
            }
        }

        closedir(d);
    }

    return files;
}

char *platform_path(GamePath root)
{
    char *path = nullptr;

    switch (root) {
    case GamePath_data: {
        char *data_env = getenv("LEARY_DATA_ROOT");
        if (data_env) {
            u64 length = strlen(data_env) + 1;
            if (data_env[length-2] != '/') {
                path = (char*)malloc(length + 1);
                strcpy(path, data_env);
                path[length-1] = '/';
                path[length]   = '\0';
            } else {
                path = (char*)malloc(length);
                strcpy(path, data_env);
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
        }
    } break;
    case GamePath_binary: {
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

        path = (char*)malloc(length + 1);
        strncpy(path, buffer, length);
    } break;
    case GamePath_shaders: {
        char *data_path = platform_path(GamePath_data);
        u64 length = strlen(data_path) + strlen("shaders/") + 1;
        path = (char*)malloc(length);
        strcpy(path, data_path);
        strcat(path, "shaders/");
    } break;
    case GamePath_textures: {
        char *data_path = platform_path(GamePath_data);
        u64 length = strlen(data_path) + strlen("textures/") + 1;
        path = (char*)malloc(length);
        strcpy(path, data_path);
        strcat(path, "textures/");
    } break;
    case GamePath_models: {
        char *data_path = platform_path(GamePath_data);
        u64 length = strlen(data_path) + strlen("models/") + 1;
        path = (char*)malloc(length);
        strcpy(path, data_path);
        strcat(path, "models/");
    } break;
    case GamePath_preferences: {
        char *local_share = getenv("XDG_DATA_HOME");
        if (local_share) {
            u64 length = strlen(local_share) + strlen("leary/") + 1;
            path = (char*)malloc(length);
            strcpy(path, local_share);
            strcat(path, "leary/");
        } else {
            struct passwd *pw = getpwuid(getuid());
            u64 length = strlen(pw->pw_dir) + strlen("/.local/share/leary/") + 1;
            path = (char*)malloc(length);
            strcpy(path, pw->pw_dir);
            strcat(path, "/.local/share/leary/");
        }
    } break;
    default:
        DEBUG_ASSERT(false);
        break;
    }

    return path;
}

char *platform_resolve_relative(const char *path)
{
    return realpath(path, nullptr);
}

char *platform_resolve_path(GamePath root, const char *path)
{
    static const char *data_path        = platform_path(GamePath_data);
    static const char *binary_path      = platform_path(GamePath_binary);
    static const char *models_path      = platform_path(GamePath_models);
    static const char *shaders_path     = platform_path(GamePath_shaders);
    static const char *textures_path    = platform_path(GamePath_textures);
    static const char *preferences_path = platform_path(GamePath_preferences);

    static const u64 data_path_length        = strlen(data_path);
    static const u64 binary_path_length      = strlen(binary_path);
    static const u64 models_path_length      = strlen(models_path);
    static const u64 shaders_path_length     = strlen(shaders_path);
    static const u64 textures_path_length    = strlen(textures_path);
    static const u64 preferences_path_length = strlen(preferences_path);

    char *resolved = nullptr;

    switch (root) {
    case GamePath_data: {
        usize length = data_path_length + strlen(path) + 1;
        resolved = (char*)malloc(length);
        strcpy(resolved, data_path);
        strcat(resolved, path);
    } break;
    case GamePath_binary: {
        usize length = binary_path_length + strlen(path) + 1;
        resolved = (char*)malloc(length);
        strcpy(resolved, binary_path);
        strcat(resolved, path);
    } break;
    case GamePath_shaders: {
        usize length = shaders_path_length + strlen(path) + 1;
        resolved = (char*)malloc(length);
        strcpy(resolved, shaders_path);
        strcat(resolved, path);
    } break;
    case GamePath_textures: {
        usize length = textures_path_length + strlen(path) + 1;
        resolved = (char*)malloc(length);
        strcpy(resolved, textures_path);
        strcat(resolved, path);
    } break;
    case GamePath_models: {
        usize length = models_path_length + strlen(path) + 1;
        resolved = (char*)malloc(length);
        strcpy(resolved, models_path);
        strcat(resolved, path);
    } break;
    case GamePath_preferences: {
        usize length = preferences_path_length + strlen(path) + 1;
        resolved = (char*)malloc(length);
        strcpy(resolved, preferences_path);
        strcat(resolved, path);
    } break;
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

    return (void*)(i64)fd;
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

