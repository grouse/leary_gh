/**
 * @file:   platform_main.h
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

#ifndef LEARY_PLATFORM_MAIN_H
#define LEARY_PLATFORM_MAIN_H

#include <cstring>
#include "core/types.h"

#if defined(__linux__)
	#include <X11/Xlib.h>
	#include <X11/XKBlib.h>
	#include <X11/extensions/XInput2.h>

	#include <x86intrin.h>
#elif defined(_WIN32)
	#include <Windows.h>
	#include <Shlobj.h>
	#include <Shlwapi.h>

	#include <intrin.h>
#else
	#error "unsupported platform"
#endif


#if defined(_WIN32)
	#define rdtsc() __rdtsc()

	#undef near
	#undef far

	#define FILE_SEP "\\"
	#define FILE_EOL "\r\n"
#elif defined(__linux__)
	#define rdtsc() __rdtsc()

	#define FILE_SEP "/"
	#define FILE_EOL "\n"
#else
	#error "unsupported platform"
#endif

#ifndef INTROSPECT
#define INTROSPECT
#endif

#define DEBUG_LOG(...)        platform_debug_print(DEBUG_FILENAME, __LINE__, __FUNCTION__, __VA_ARGS__)
#define DEBUG_UNIMPLEMENTED() DEBUG_LOG(Log_unimplemented, "fixme! stub");


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

// NOTE(jesper): platform specific
enum VirtualKey : i32;

enum InputType {
	InputType_key_release,
	InputType_key_press,
	InputType_mouse_move
};

enum LogChannel {
	Log_error,
	Log_warning,
	Log_info,
	Log_assert,
	Log_unimplemented
};

struct InputEvent {
	InputType type;
	union {
		struct {
			VirtualKey vkey;
			bool repeated;
		} key;
		struct {
			f32 dx, dy;
			f32 x, y;
		} mouse;
	};
};

struct PlatformState {
	bool raw_mouse = false;

	union {
#if defined(__linux__)
		struct
		{
			Window     window;
			Display    *display;
			XkbDescPtr xkb;
			i32        xi2_opcode;
			Cursor     hidden_cursor;
		} x11;
#elif defined(_WIN32)
		struct
		{
			HWND      hwnd;
			HINSTANCE hinstance;
		} win32;
#else
	#error "unsupported platform"
#endif
	};
};


const char *log_channel_string(LogChannel channel)
{
	switch (channel) {
	case Log_info:          return "info";
	case Log_error:         return "error";
	case Log_warning:       return "warning";
	case Log_assert:        return "assert";
	case Log_unimplemented: return "unimplemented";
	default:                return "";
	}
}

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          LogChannel channel,
                          const char *fmt, ...);

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          const char *fmt, ...);


char *platform_path(GamePath root);
char *platform_resolve_path(const char *path);
char *platform_resolve_path(GamePath root, const char *path);

bool  platform_file_exists(const char *path);
bool  platform_file_create(const char *path);

void* platform_file_open(const char *path, FileAccess access);
void  platform_file_close(void *file_handle);

void  platform_file_write(void *file_handle, void *buffer, usize bytes);
char *platform_file_read(const char* path, usize *out_size);

void platform_toggle_raw_mouse(PlatformState *state);
void platform_quit(PlatformState *state);

#endif // LEARY_PLATFORM_MAIN_H
