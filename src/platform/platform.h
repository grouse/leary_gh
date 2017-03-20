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

	#define FILE_SEP "/"
	#define FILE_EOL "\n"

	#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(_WIN32)
	#include <Windows.h>
	#include <Shlobj.h>
	#include <Shlwapi.h>

	#define VK_USE_PLATFORM_WIN32_KHR

	#undef near
	#undef far

	#define FILE_SEP "\\"
	#define FILE_EOL "\r\n"
#else
	#error "unsupported platform"
#endif

#include <vulkan/vulkan.h>

#ifndef INTROSPECT
#define INTROSPECT
#endif

struct PlatformState;

// TODO(jesper): reevaluate whether some of the platform layer should be
// compiled into the game.dll

#include "platform_file.h"

#define PLATFORM_FUNCS(M)\
	M(void,     toggle_raw_mouse,                 PlatformState*);\
	M(void,     set_raw_mouse,                    PlatformState *, bool);\
	M(void,     quit,                             PlatformState *);\
\
	M(VkResult, vulkan_create_surface,            VkInstance, VkSurfaceKHR *, PlatformState *);\
	M(bool,     vulkan_enable_instance_extension, VkExtensionProperties &);\
	M(bool,     vulkan_enable_instance_layer,     VkLayerProperties);\
\
	M(char*,    resolve_relative,                 const char*);\
	M(char*,    resolve_path,                     GamePath, const char *);\
	M(bool,     file_exists,                      const char *);\
	M(bool,     file_create,                      const char *);\
	M(void*,    file_open,                        const char *, FileAccess);\
	M(void,     file_close,                       void*);\
	M(void,     file_write,                       void*, void*, usize);\
	M(char*,    file_read,                        const char *, usize *)

#define PLATFORM_TYPEDEF_FUNC(ret, name, ...) typedef ret platform_##name##_t (__VA_ARGS__)
#define PLATFORM_FUNC_STUB(ret, name, ...) ret platform_##name##_stub(__VA_ARGS__) { return (ret)0; }
#define PLATFORM_DCL_FPTR(ret, name, ...) platform_##name##_t *name
#define PLATFORM_DCL_STATIC_FPTR(ret, name, ...) static platform_##name##_t *platform_##name

PLATFORM_FUNCS(PLATFORM_TYPEDEF_FUNC);

INTROSPECT struct Resolution
{
	i32 width  = 1280;
	i32 height = 720;
};

INTROSPECT struct VideoSettings
{
	Resolution resolution;

	// NOTE: these are integers to later support different fullscreen and vsync techniques
	i16 fullscreen = 0;
	i16 vsync      = 1;
};

INTROSPECT struct Settings
{
	VideoSettings video;
};

struct PlatformState {
	Settings settings = {};
	bool raw_mouse    = false;

	void *native      = nullptr;
};

struct PlatformCode {
	PLATFORM_FUNCS(PLATFORM_DCL_FPTR);
};

#endif // LEARY_PLATFORM_MAIN_H
