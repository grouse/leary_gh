/**
 * file:    file.cpp
 * created: 2017-12-24
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "file.h"

bool operator==(FilePath lhs, FilePath rhs)
{
    return lhs.absolute == rhs.absolute;
}

bool operator==(FolderPath lhs, FolderPath rhs)
{
    return lhs.absolute == rhs.absolute;
}

void resolve_filename_ext(
    StringView absolute,
    StringView *filename,
    StringView *extension)
{
    i32 slash = 0;
    i32 ext   = 0;
    for (i32 i = 0; i < absolute.size; i++) {
        if (absolute[i] == '/' || absolute[i] == '\\') {
            slash = i;
        } else if (absolute[i] == '.') {
            ext = i;
        }
    }

    *filename = {
        absolute.size  - slash - 1,
        absolute.bytes + slash + 1
    };

    if (ext != 0) {
        *extension = {
            absolute.size  - ext - 1,
            absolute.bytes + ext + 1
        };
    }
}

i32 utf8_size(FilePath file)
{
    return utf8_size(file.absolute);
}

i32 utf8_size(FilePathView file)
{
    return utf8_size(file.absolute);
}

i32 utf8_size(FolderPath folder)
{
    return utf8_size(folder.absolute);
}

i32 utf8_size(FolderPathView folder)
{
    return utf8_size(folder.absolute);
}

void string_concat(String *str, FilePath file)
{
    string_concat(str, file.absolute);
}

void string_concat(String *str, FilePathView file)
{
    string_concat(str, file.absolute);
}

void string_concat(String *str, FolderPath folder)
{
    string_concat(str, folder.absolute);
}

void string_concat(String *str, FolderPathView folder)
{
    string_concat(str, folder.absolute);
}
