/**
 * file:    platform_file.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PLATFORM_FILE_H
#define PLATFORM_FILE_H

#include "core/string.h"

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
    String filename; // TODO: should this be with or without extension?
    String extension;
};

#endif /* PLATFORM_FILE_H */

