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

#define PLATFORM_TOGGLE_RAW_MOUSE_FUNC(name)                 void name(PlatformState *platform)
#define PLATFORM_SET_RAW_MOUSE_FUNC(name)                    void name(PlatformState *platform, bool enable)
#define PLATFORM_QUIT_FUNC(name)                             void name(PlatformState *platform)

#define PLATFORM_VULKAN_CREATE_SURFACE_FUNC(name)            VkResult name(VkInstance instance, VkSurfaceKHR *surface, PlatformState *platform)
#define PLATFORM_VULKAN_ENABLE_INSTANCE_EXTENSION_FUNC(name) bool name(VkExtensionProperties &extension)
#define PLATFORM_VULKAN_ENABLE_INSTANCE_LAYER_FUNC(name)     bool name(VkLayerProperties layer)

#define PLATFORM_RESOLVE_RELATIVE_FUNC(name)                 char* name(const char *path)
#define PLATFORM_RESOLVE_PATH_FUNC(name)                     char* name(GamePath root, const char *path)
#define PLATFORM_FILE_EXISTS_FUNC(name)                      bool name(const char *path)
#define PLATFORM_FILE_CREATE_FUNC(name)                      bool name(const char *path)
#define PLATFORM_FILE_OPEN_FUNC(name)                        void* name(const char *path, FileAccess access)
#define PLATFORM_FILE_CLOSE_FUNC(name)                       void name(void *file_handle)
#define PLATFORM_FILE_WRITE_FUNC(name)                       void name(void *file_handle, void *buffer, usize size)
#define PLATFORM_FILE_READ_FUNC(name)                        char* name(const char *read, usize *size)

typedef PLATFORM_TOGGLE_RAW_MOUSE_FUNC(platform_toggle_raw_mouse_t);
typedef PLATFORM_SET_RAW_MOUSE_FUNC(platform_set_raw_mouse_t);
typedef PLATFORM_QUIT_FUNC(platform_quit_t);

typedef PLATFORM_VULKAN_CREATE_SURFACE_FUNC(platform_vulkan_create_surface_t);
typedef PLATFORM_VULKAN_ENABLE_INSTANCE_EXTENSION_FUNC(platform_vulkan_enable_instance_extension_t);
typedef PLATFORM_VULKAN_ENABLE_INSTANCE_LAYER_FUNC(platform_vulkan_enable_instance_layer_t);

typedef PLATFORM_RESOLVE_RELATIVE_FUNC(platform_resolve_relative_t);
typedef PLATFORM_RESOLVE_PATH_FUNC(platform_resolve_path_t);
typedef PLATFORM_FILE_EXISTS_FUNC(platform_file_exists_t);
typedef PLATFORM_FILE_CREATE_FUNC(platform_file_create_t);
typedef PLATFORM_FILE_OPEN_FUNC(platform_file_open_t);
typedef PLATFORM_FILE_CLOSE_FUNC(platform_file_close_t);
typedef PLATFORM_FILE_WRITE_FUNC(platform_file_write_t);
typedef PLATFORM_FILE_READ_FUNC(platform_file_read_t);

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
	platform_toggle_raw_mouse_t                 *toggle_raw_mouse;
	platform_set_raw_mouse_t                    *set_raw_mouse;
	platform_quit_t                             *quit;

	platform_vulkan_create_surface_t            *vulkan_create_surface;
	platform_vulkan_enable_instance_extension_t *vulkan_enable_instance_extension;
	platform_vulkan_enable_instance_layer_t     *vulkan_enable_instance_layer;

	platform_resolve_relative_t                 *resolve_relative;
	platform_resolve_path_t                     *resolve_path;
	platform_file_exists_t                      *file_exists;
	platform_file_create_t                      *file_create;
	platform_file_open_t                        *file_open;
	platform_file_close_t                       *file_close;
	platform_file_write_t                       *file_write;
	platform_file_read_t                        *file_read;
};

#endif // LEARY_PLATFORM_MAIN_H
