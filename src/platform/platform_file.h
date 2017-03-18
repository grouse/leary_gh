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
	GamePath_models,
	GamePath_shaders,
	GamePath_fonts,
	GamePath_preferences
};

char *platform_path(GamePath root);
char *platform_resolve_path(const char *path);
char *platform_resolve_path(GamePath root, const char *path);

bool  platform_file_exists(const char *path);
bool  platform_file_create(const char *path);

void* platform_file_open(const char *path, FileAccess access);
void  platform_file_close(void *file_handle);

void  platform_file_write(void *file_handle, void *buffer, usize bytes);
char *platform_file_read(const char* path, usize *out_size);

#endif /* PLATFORM_FILE_H */

