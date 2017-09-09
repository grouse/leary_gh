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

struct PlatformPaths {
    String preferences;
    String data;
    String exe;
    String shaders;
    String textures;
    String models;
};

PlatformPaths g_paths;

void init_paths()
{
    g_paths = {};

    isize length;
    char *p;

    // --- exe dir
    // NOTE(jesper): other paths depend on this being resolved early
    char linkname[64];
    pid_t pid = getpid();
    i64 result = snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);
    DEBUG_ASSERT(result >= 0);

    char link_buffer[PATH_MAX];
    i64 link_length = readlink(linkname, link_buffer, PATH_MAX);
    DEBUG_ASSERT(link_length >= 0);

    char *last = nullptr;
    for (i32 i = 0; i < link_length; i++) {
        if (link_buffer[i] == '/') {
            last = &link_buffer[i];
        }
    }
    link_length = (i64)(last - link_buffer + 1);

    g_paths.exe = { link_length, (char*)malloc(link_length + 1) };
    strncpy(g_paths.exe.bytes, link_buffer, link_length);

    // --- app data dir
    // NOTE(jesper): other paths depend on this being resolved early
    char *data_env = getenv("LEARY_DATA_ROOT");
    if (data_env) {
        length = strlen(data_env);
        if (data_env[length-1] != '/') {
            g_paths.data = { length + 1, (char*)malloc(length + 2) };
            p = strcpy(g_paths.data.bytes, data_env);
            g_paths.data[length-1] = '/';
            g_paths.data[length]   = '\0';
        } else {
            g_paths.data = { length, (char*)malloc(length + 1) };
            strcpy(g_paths.data.bytes, data_env);
        }
    } else {
        length = g_paths.exe.length + strlen("../assets/");
        g_paths.data = { length, (char*)malloc(length + 1) };
        p = strncpy(g_paths.data.bytes, g_paths.exe.bytes, g_paths.exe.length);
        strcat(p, "../assets/");
    }

    // --- app preferences dir
    char *local_share = getenv("XDG_DATA_HOME");
    if (local_share) {
        length = strlen(local_share) + strlen("leary/");
        g_paths.preferences = { length, (char*)malloc(length) + 1 };
        p = strcpy(g_paths.preferences.bytes, local_share);
        strcat(p, "leary/");
    } else {
        struct passwd *pw = getpwuid(getuid());
        length = strlen(pw->pw_dir) + strlen("/.local/share/leary/");
        g_paths.preferences = { length, (char*)malloc(length) + 1 };
        p = strcpy(g_paths.preferences.bytes, pw->pw_dir);
        strcat(p, "/.local/share/leary/");
    }


    // --- shaders dir
    length = g_paths.exe.length + strlen("data/sahders/");
    g_paths.shaders = { length, (char*)malloc(length + 1) };
    p = strcpy(g_paths.shaders.bytes, g_paths.exe.bytes);
    strcat(p, "data/shaders/");

    // -- textures dir
    length = g_paths.data.length + strlen("textures/");
    g_paths.textures = { length, (char*)malloc(length + 1) };
    p = strcpy(g_paths.textures.bytes, g_paths.data.bytes);
    strcat(p, "textures/");

    // -- models dir
    length = g_paths.data.length + strlen("models/");
    g_paths.models = { length, (char*)malloc(length + 1) };
    p = strcpy(g_paths.models.bytes, g_paths.data.bytes);
    strcat(p, "models/");
}

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

                if (!eslash) {
                    p.filename = { dlen, p.absolute.bytes + flen + 1 };
                } else {
                    p.filename = { dlen, p.absolute.bytes + flen };
                }

                isize ext = 0;
                for (i32 i = 0; i < p.filename.length; i++) {
                    if (p.filename[i] == '.') {
                        ext = i;
                    }
                }
                p.extension = { dlen - ext, p.filename.bytes + ext + 1 };
                array_add(&files, p);
            } else {
                DEBUG_LOG("unimplemented d_type: %d", dir->d_type);
            }
        }

        closedir(d);
    }

    return files;
}

char* resolve_relative(const char *path)
{
    return realpath(path, nullptr);
}

char* resolve_path(GamePath rp, const char *path)
{
    usize length, plength;
    char *resolved;
    char *p, *root;

    plength = strlen(path);

    switch (rp) {
    case GamePath_data: {
        length = g_paths.data.length + plength;
        root   = g_paths.data.bytes;
    } break;
    case GamePath_exe: {
        length = g_paths.exe.length + plength;
        root   = g_paths.exe.bytes;
    } break;
    case GamePath_shaders: {
        length = g_paths.shaders.length + plength;
        root   = g_paths.shaders.bytes;
    } break;
    case GamePath_textures: {
        length = g_paths.textures.length + plength;
        root   = g_paths.textures.bytes;
    } break;
    case GamePath_models: {
        length = g_paths.models.length + plength;
        root   = g_paths.models.bytes;
    } break;
    case GamePath_preferences: {
        length = g_paths.preferences.length + plength;
        root   = g_paths.preferences.bytes;
    } break;
    default:
        DEBUG_LOG(Log_error, "unknown path root: %d", rp);
        DEBUG_ASSERT(false);
        return nullptr;
    }

    resolved = (char*)malloc(length + 1);
    p        = strcpy(resolved, root);
    strcat(p, path);

    return resolved;
}



bool file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

bool create_file(const char *path)
{
    // TODO(jesper): create folders if needed
    i32 access = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    i32 fd = open(path, O_CREAT, access);

    if (fd >= 0) {
        close(fd);
    }

    return fd >= 0;
}



void* open_file(const char *path, FileAccess access)
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

void close_file(void *file_handle)
{
    i32 fd = (i32)(i64)file_handle;
    DEBUG_ASSERT(fd >= 0);

    close(fd);
}



void write_file(void *file_handle, void *buffer, usize bytes)
{
    i32 fd = (i32)(i64)file_handle;
    isize written = write(fd, buffer, bytes);
    DEBUG_ASSERT(written >= 0);
    DEBUG_ASSERT((usize)written == bytes);
}

char* read_file(const char *path, usize *file_size)
{
    struct stat st;
    i32 result = stat(path, &st);
    if (result != 0) {
        DEBUG_LOG(Log_error, "couldn't stat file: %s", path);
        return nullptr;
    }

    char *buffer = (char*)malloc(st.st_size);

    i32 fd = open(path, O_RDONLY);
    DEBUG_ASSERT(fd >= 0);

    isize bytes_read = read(fd, buffer, st.st_size);
    DEBUG_ASSERT(bytes_read == st.st_size);
    *file_size = bytes_read;

    return buffer;
}

