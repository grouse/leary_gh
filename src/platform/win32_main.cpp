/**
 * @file:   win32_main.cpp
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2017 Jesper Stefansson
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

#include <cstdio>
#include <cstdint>
#include <cinttypes>

#include "platform.h"

#if LEARY_DYNAMIC
typedef PLATFORM_INIT_FUNC(platform_init_t);
typedef PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload_t);
typedef PLATFORM_RELOAD_FUNC(platform_reload_t);
typedef PLATFORM_UPDATE_FUNC(platform_update_t);

PLATFORM_INIT_FUNC(platform_init_stub)
{
	(void)instance;
	(void)platform;
}

PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload_stub)
{
	(void)platform;
}

PLATFORM_RELOAD_FUNC(platform_reload_stub)
{
	(void)platform;
}

PLATFORM_UPDATE_FUNC(platform_update_stub)
{
	(void)platform;
	(void)dt;
}

static platform_init_t       *platform_init       = &platform_init_stub;
static platform_pre_reload_t *platform_pre_reload = &platform_pre_reload_stub;
static platform_reload_t     *platform_reload     = &platform_reload_stub;
static platform_update_t     *platform_update     = &platform_update_stub;

HMODULE reload_code(PlatformState *platform, HMODULE current)
{
	if (platform != nullptr) {
		platform_pre_reload(platform);
	}

	if (current != NULL) {
		FreeLibrary(current);
	}

	HMODULE lib = LoadLibrary(".\\game.dll");
	if (lib != NULL) {
		platform_init       = (platform_init_t*)GetProcAddress(lib, "platform_init");
		platform_pre_reload = (platform_pre_reload_t*)GetProcAddress(lib, "platform_pre_reload");
		platform_reload     = (platform_reload_t*)GetProcAddress(lib, "platform_reload");
		platform_update     = (platform_update_t*)GetProcAddress(lib, "platform_update");
	}

	if (!platform_init)       platform_init       = &platform_init_stub;
	if (!platform_pre_reload) platform_pre_reload = &platform_pre_reload_stub;
	if (!platform_reload)     platform_reload     = &platform_reload_stub;
	if (!platform_update)     platform_update     = &platform_update_stub;

	if (platform != nullptr) {
		platform_reload(platform);
	}

	return lib;
}
#else

#include "win32_leary.cpp"

// TODO(jesper): include the game code .cpp files directly
HMODULE reload_code(PlatformState *, HMODULE)
{
	return NULL;
}

#endif

int WINAPI
WinMain(HINSTANCE instance,
        HINSTANCE /*prev_instance*/,
        LPSTR     /*cmd_line*/,
        int       /*cmd_show*/)
{
	HMODULE lib = reload_code(nullptr, NULL);
	(void)lib;

	PlatformState platform = {};
	platform_init(&platform, instance);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER last_time;
	QueryPerformanceCounter(&last_time);

	constexpr f32 code_reload_rate = 1.0f;
	f32 code_reload_timer = code_reload_rate;

	while(true) {
		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);
		i64 elapsed = current_time.QuadPart - last_time.QuadPart;
		last_time   = current_time;

		f32 dt = (f32)elapsed / frequency.QuadPart;

		code_reload_timer += dt;
		if (code_reload_timer >= code_reload_rate) {
			reload_code(&platform, lib);
			code_reload_timer = 0.0f;
		}


		platform_update(&platform, dt);
	}

	return 0;
}
