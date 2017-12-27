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

Path create_path(const char *str, Allocator *a);

#endif // LEARY_FILE_H
