/**
 * file:    linux_file.cpp
 * created: 2016-09-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2018 - all rights reserved
 */

PlatformPaths g_paths;

void init_paths(Allocator *a)
{
    g_paths = {};

    isize length;

    // --- exe dir
    // NOTE(jesper): other paths depend on this being resolved early
    char linkname[64];
    pid_t pid = getpid();
    i64 result = snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);
    ASSERT(result >= 0);

    char link_buffer[PATH_MAX];
    i64 link_length = readlink(linkname, link_buffer, PATH_MAX);
    ASSERT(link_length >= 0);

    char *last = nullptr;
    for (i32 i = 0; i < link_length; i++) {
        if (link_buffer[i] == '/') {
            last = &link_buffer[i];
        }
    }
    link_length = (i64)(last - link_buffer + 1);

    g_paths.exe = create_folder_path(a, StringView{ (i32)link_length, link_buffer });

    // --- app data dir
    // NOTE(jesper): other paths depend on this being resolved early
    char *data_env = getenv("LEARY_DATA_ROOT");
    if (data_env) {
        length = strlen(data_env);
        if (data_env[length-1] != '/') {
            g_paths.data = create_folder_path(a, data_env, '/');
        } else {
            g_paths.data = create_folder_path(a, data_env);
        }
    } else {
        g_paths.data = create_folder_path(a, g_paths.exe, "../assets/");
    }

    // --- app preferences dir
    char *local_share = getenv("XDG_DATA_HOME");
    if (local_share) {
        g_paths.preferences = create_folder_path(a, local_share, "leary/");
    } else {
        struct passwd *pw = getpwuid(getuid());
        g_paths.preferences = create_folder_path(a, pw->pw_dir, "/.local/share/leary/");
    }

    g_paths.shaders  = create_folder_path(a, g_paths.exe, "data/shaders/");
    g_paths.textures = create_folder_path(a, g_paths.data, "textures/");
    g_paths.models   = create_folder_path(a, g_paths.data, "models/");
}

Array<FilePath> list_files(FolderPath folder, Allocator *allocator)
{
    // TODO: this should maybe return an array of File which might include full
    // path, filename, and extension?
    auto files = create_array<FilePath>(allocator);
    bool eslash = (folder[folder.absolute.size-1] == '/');

    DIR *d = opendir(folder.absolute.bytes);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            // TODO(jesper): dir with sub-folders
            if (dir->d_type == DT_REG) {
                FilePath p;

                if (!eslash) {
                    p = create_file_path(allocator, folder, "/", dir->d_name);
                } else {
                    p = create_file_path(allocator, folder, dir->d_name);
                }

                LOG("adding file: %s", p.absolute.bytes);
                array_add(&files, p);
            } else {
                LOG("unimplemented d_type: %d", dir->d_type);
            }
        }

        closedir(d);
    }

    return files;
}

char* resolve_relative(const char *path)
{
    char buffer[1024];
    char *resolved = realpath(path, buffer);

    // TODO(jesper): currently leaking memory
    return strdup(resolved);
}

FilePath resolve_file_path(GamePath rp, StringView path, Allocator *a)
{
    FolderPathView root;

    switch (rp) {
    case GamePath_data:
        root = g_paths.data;
        break;
    case GamePath_exe:
        root = g_paths.exe;
        break;
    case GamePath_shaders:
        root = g_paths.shaders;
        break;
    case GamePath_textures:
        root = g_paths.textures;
        break;
    case GamePath_models:
        root = g_paths.models;
        break;
    case GamePath_preferences:
        root = g_paths.preferences;
        break;
    default:
        LOG(Log_error, "unknown path root: %d", rp);
        ASSERT(false);
        return {};
    }

    FilePath resolved = create_file_path(a, root, path);
    for (i32 i = 0; i < resolved.absolute.size; i++) {
        if (resolved[i] == '\\') {
            resolved[i] = '/';
        }
    }

    return resolved;
}

FolderPath resolve_folder_path(GamePath rp, StringView path, Allocator *a)
{
    FolderPathView root;

    switch (rp) {
    case GamePath_data:
        root = g_paths.data;
        break;
    case GamePath_exe:
        root = g_paths.exe;
        break;
    case GamePath_shaders:
        root = g_paths.shaders;
        break;
    case GamePath_textures:
        root = g_paths.textures;
        break;
    case GamePath_models:
        root = g_paths.models;
        break;
    case GamePath_preferences:
        root = g_paths.preferences;
        break;
    default:
        LOG(Log_error, "unknown path root: %d", rp);
        ASSERT(false);
        return {};
    }

    FolderPath resolved = create_folder_path(a, root, path);
    for (i32 i = 0; i < resolved.absolute.size; i++) {
        if (resolved[i] == '\\') {
            resolved[i] = '/';
        }
    }

    return resolved;
}

bool file_exists(FilePathView file)
{
    struct stat st;
    return (stat(file.absolute.bytes, &st) == 0);
}

bool create_file(FilePathView path, bool create_folders = false)
{
    // TODO(jesper): create folders if needed
    (void)create_folders;

    i32 access = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    i32 fd = open(path.absolute.bytes, O_CREAT, access);

    if (fd >= 0) {
        close(fd);
    }

    return fd >= 0;
}

void* open_file(FilePathView path, FileAccess access)
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
        ASSERT(false);
        return nullptr;
    }

    i32 fd = open(path.absolute.bytes, flags);
    if (fd < 0) {
        return nullptr;
    }

    return (void*)(i64)fd;
}

void close_file(void *file_handle)
{
    i32 fd = (i32)(i64)file_handle;
    ASSERT(fd >= 0);

    close(fd);
}

void write_file(void *file_handle, void *buffer, usize bytes)
{
    i32 fd = (i32)(i64)file_handle;
    isize written = write(fd, buffer, bytes);
    ASSERT(written >= 0);
    ASSERT((usize)written == bytes);
}

char* read_file(FilePathView filename, usize *file_size, Allocator *a)
{
    struct stat st;
    i32 result = stat(filename.absolute.bytes, &st);
    if (result != 0) {
        LOG(Log_error, "couldn't stat file: %s", filename.absolute.bytes);
        return nullptr;
    }

    char *buffer = (char*)alloc(a, st.st_size);

    i32 fd = open(filename.absolute.bytes, O_RDONLY);
    ASSERT(fd >= 0);

    isize bytes_read = read(fd, buffer, st.st_size);
    ASSERT(bytes_read == st.st_size);
    *file_size = bytes_read;

    return buffer;
}

