/**
 * @file:   linux_main.cpp @author: Jesper Stefansson (grouse)
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

#include <cstdio>
#include <cstdint>
#include <cstdlib>

#include <linux/limits.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>

#include <time.h>

#include "platform.h"
#include "platform/platform_debug.h"

struct DynamicLib {
	void   *handle;
	time_t load_time;
};

#if LEARY_DYNAMIC
#include "linux_debug.cpp"

typedef PLATFORM_INIT_FUNC(platform_init_t);
typedef PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload_t);
typedef PLATFORM_RELOAD_FUNC(platform_reload_t);
typedef PLATFORM_UPDATE_FUNC(platform_update_t);

PLATFORM_INIT_FUNC(platform_init_stub)
{
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

#define DLOAD_FUNC(lib, name) (name##_t*)dlsym(lib, #name)
DynamicLib load_code(char *path)
{
	DynamicLib lib = {};

	struct stat st;
	int result = stat(path, &st);
	DEBUG_ASSERT(result == 0);

	lib.load_time = st.st_mtime;
	lib.handle    = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);

	if (lib.handle) {
		platform_init       = DLOAD_FUNC(lib.handle, platform_init);
		platform_pre_reload = DLOAD_FUNC(lib.handle, platform_pre_reload);
		platform_reload     = DLOAD_FUNC(lib.handle, platform_reload);
		platform_update     = DLOAD_FUNC(lib.handle, platform_update);
	}

	if (!platform_init)       platform_init       = &platform_init_stub;
	if (!platform_pre_reload) platform_pre_reload = &platform_pre_reload_stub;
	if (!platform_reload)     platform_reload     = &platform_reload_stub;
	if (!platform_update)     platform_update     = &platform_update_stub;

	return lib;
}

void unload_code(DynamicLib lib)
{
	dlclose(lib.handle);
}

#else
#include "linux_leary.cpp"

DynamicLib load_code(char *)
{
	return DynamicLib{};
}

void unload_code(DynamicLib)
{
}
#endif

timespec get_time()
{
	timespec ts;
	i32 result = clock_gettime(CLOCK_MONOTONIC, &ts);
	DEBUG_ASSERT(result == 0);
	return ts;
}

i64 get_time_difference(timespec start, timespec end)
{
	i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
	                 (end.tv_nsec - start.tv_nsec);
	DEBUG_ASSERT(difference >= 0);
	return difference;
}


int main()
{
	char linkname[64];
	pid_t pid = getpid();
	i64 result = snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);
	DEBUG_ASSERT(result >= 0);

	char buffer[PATH_MAX];
	i64 length = readlink(linkname, buffer, PATH_MAX);
	DEBUG_ASSERT(length >= 0);

	for (; length >= 0; length--) {
		if (buffer[length-1] == '/') {
			break;
		}
	}

	char *lib_path = (char*)malloc(length + strlen("game.so") + 1);
	strncpy(lib_path, buffer, length);
	strcat(lib_path, "game.so");

#if LEARY_DYNAMIC
	DynamicLib lib = load_code(lib_path);

	constexpr f32 code_reload_rate = 1.0f;
	f32 code_reload_timer = code_reload_rate;
#endif

	PlatformState platform = {};
	platform_init(&platform);

	timespec last_time = get_time();
	while (true) {
		timespec current_time = get_time();
		i64 difference        = get_time_difference(last_time, current_time);
		last_time             = current_time;

		f32 dt = (f32)difference / 1000000000.0f;
		DEBUG_ASSERT(difference >= 0);

#if LEARY_DYNAMIC
		code_reload_timer += dt;
		if (code_reload_timer >= code_reload_rate) {
			struct stat st;
			int result = stat(lib_path, &st);
			DEBUG_ASSERT(result == 0);

			if (st.st_mtime > lib.load_time) {
				platform_pre_reload(&platform);

				unload_code(lib);
				lib = load_code(lib_path);

				platform_reload(&platform);
				DEBUG_LOG("reloaded code: %lld", lib.load_time);
			}

			code_reload_timer = 0.0f;
		}
#endif

		platform_update(&platform, dt);
	}

	return 0;
}
