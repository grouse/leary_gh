/**
 * file:    file.h
 * created: 2017-12-24
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */


#ifndef LEARY_FILE_H
#define LEARY_FILE_H

#include "string.h"

enum FileAccess {
    FileAccess_read,
    FileAccess_write,
    FileAccess_read_write
};

enum GamePath {
    GamePath_data,
    GamePath_exe,
    GamePath_models,
    GamePath_shaders,
    GamePath_fonts,
    GamePath_textures,
    GamePath_preferences
};

void resolve_filename_ext(
    StringView absolute,
    StringView *filename,
    StringView *extension);

struct FolderPath {
    String absolute = {};

    char& operator[](i32 i)
    {
        return absolute[i];
    }
};

struct FolderPathView {
    StringView absolute = {};

    FolderPathView() {}

    FolderPathView(FolderPath other)
    {
        absolute = other.absolute;
    }

    template<i32 N>
    FolderPathView(const char (&str)[N])
    {
        absolute = { N, str };
    }

    FolderPathView(const char *str)
    {
        absolute = { str };
    }

    FolderPathView(i32 size, const char *str)
    {
        absolute = { size, str };
    }

    const char& operator[] (i32 i)
    {
        return absolute[i];
    }
};

struct FilePath {
    String     absolute  = {};
    StringView filename  = {}; // NOTE(jesper): includes extension
    StringView extension = {}; // NOTE(jesper): excluding .

    char& operator[](i32 i)
    {
        return absolute[i];
    }
};

struct FilePathView {
    StringView absolute  = {};
    StringView filename  = {}; // NOTE(jesper): includes extension
    StringView extension = {}; // NOTE(jesper): excluding .

    template<i32 N>
    FilePathView(const char (&str)[N])
    {
        absolute = { N, byte };
        resolve_filename_ext(absolute, &filename, &extension);
    }

    FilePathView(const char *str)
    {
        absolute = { str };
        resolve_filename_ext(absolute, &filename, &extension);
    }

    FilePathView(FilePath other)
    {
        absolute  = other.absolute;
        filename  = other.filename;
        extension = other.extension;
    }

    FilePathView(i32 size, const char *str)
    {
        absolute = { size, str };
        resolve_filename_ext(absolute, &filename, &extension);
    }

    const char& operator[] (i32 i)
    {
        return absolute[i];
    }
};

bool operator==(FilePath lhs, FilePath rhs);
bool operator==(FolderPath lhs, FolderPath rhs);

template<typename... Args>
FilePath create_file_path(Allocator *a, Args... args)
{
    FilePath p = {};
    p.absolute = create_string(a, args...);
    resolve_filename_ext(p.absolute, &p.filename, &p.extension);

    return p;
}

template<typename... Args>
FolderPath create_folder_path(Allocator *a, Args... args)
{
    FolderPath p = {};
    p.absolute = create_string(a, args...);
    return p;
}

// NOTE(jesper): platform specific implementation
FilePath resolve_file_path(GamePath rp, StringView path, Allocator *a);

FilePath resolve_file_path(GamePath rp, FilePathView path, Allocator *a)
{
    return resolve_file_path(rp, path.absolute, a);
}

template<i32 N>
FilePath resolve_file_path(GamePath rp, const char (&str)[N], Allocator *a)
{
    return resolve_file_path(rp, StringView{str}, a);
}

i32 utf8_size(FilePath file);
i32 utf8_size(FilePathView file);
i32 utf8_size(FolderPath folder);
i32 utf8_size(FolderPathView folder);

void string_concat(String *str, FilePath file);
void string_concat(String *str, FilePathView file);
void string_concat(String *str, FolderPath folder);
void string_concat(String *str, FolderPathView folder);

#endif // LEARY_FILE_H
