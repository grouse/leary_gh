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
#elif defined(_WIN32)
	#include <Windows.h>
	#include <Shlobj.h>
	#include <Shlwapi.h>
#else
	#error "unsupported platform"
#endif

#if defined(_WIN32)
	#undef near
	#undef far

	#define FILE_SEP "\\"
	#define FILE_EOL "\r\n"
#elif defined(__linux__)
	#define FILE_SEP "/"
	#define FILE_EOL "\n"
#else
	#error "unsupported platform"
#endif

#ifndef INTROSPECT
#define INTROSPECT
#endif

#define VAR_UNUSED(var) (void)(var)

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

			struct {
				i32 x;
				i32 y;
			} mouse;
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

void platform_toggle_raw_mouse(PlatformState *state);
void platform_set_raw_mouse(PlatformState *state, bool enable);
void platform_quit(PlatformState *state);

#endif // LEARY_PLATFORM_MAIN_H
