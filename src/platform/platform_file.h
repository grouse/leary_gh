/**
 * file:    platform_file.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PLATFORM_FILE_H
#define PLATFORM_FILE_H

enum FileAccess {
    FileAccess_read,
    FileAccess_write,
    FileAccess_read_write
};

enum GamePath {
    GamePath_data,
    GamePath_binary,
    GamePath_models,
    GamePath_shaders,
    GamePath_fonts,
    GamePath_textures,
    GamePath_preferences
};

struct Path {
    // TODO(jesper): make a String struct for these kind of things so I can
    // allocate 1 memory block for the absolute path, and just point filename
    // and extension into it.
    char *absolute;
    char *filename; // TODO: should this be with or without extension?
    char *extension;
};

#endif /* PLATFORM_FILE_H */

