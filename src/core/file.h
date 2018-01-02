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

struct Path {
    String absolute;
    StringView filename;  // NOTE(jesper): includes extension
    StringView extension; // NOTE(jesper): excluding .
};

template<typename... Args>
Path create_file_path(Allocator *a, Args... args)
{
    Path p = {};
    p.absolute = create_string(a, args...);

    i32 slash = 0;
    i32 ext   = 0;
    for (i32 i = 0; i < p.absolute.size; i++) {
        if (p.absolute[i] == '/' || p.absolute[i] == '\\') {
            slash = i;
        } else if (p.absolute[i] == '.') {
            ext = i;
        }
    }

    p.filename = { p.absolute.size - slash, p.absolute.bytes + slash + 1 };

    if (ext != 0) {
        p.extension = { p.absolute.size - ext, p.absolute.bytes + ext + 1 };
    }

    return p;
}

#endif // LEARY_FILE_H
