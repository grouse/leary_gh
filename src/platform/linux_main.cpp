/**
 * file:    linux_main.cpp
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2015-2018 - all rights reserved
 */

struct DynamicLib {
	void   *handle;
	time_t load_time;
};

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
	assert(result == 0);

	lib.load_time = st.st_mtime;
	lib.handle    = dlopen(path, RTLD_NOW | RTLD_GLOBAL);

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

timespec get_time()
{
	timespec ts;
	i32 result = clock_gettime(CLOCK_MONOTONIC, &ts);
	assert(result == 0);
	return ts;
}

i64 get_time_difference(timespec start, timespec end)
{
	i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
	                 (end.tv_nsec - start.tv_nsec);
	assert(difference >= 0);
	return difference;
}


int main()
{
	char linkname[64];
	pid_t pid = getpid();
	i64 result = snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid);
	assert(result >= 0);

	char buffer[PATH_MAX];
	i64 length = readlink(linkname, buffer, PATH_MAX);
	assert(length >= 0);

	for (; length >= 0; length--) {
		if (buffer[length-1] == '/') {
			break;
		}
	}

	char *lib_path = (char*)malloc(length + strlen("game.so") + 1);
	strncpy(lib_path, buffer, length);
	strcat(lib_path, "game.so");

	DynamicLib lib = load_code(lib_path);

	constexpr f32 code_reload_rate = 1.0f;
	f32 code_reload_timer = code_reload_rate;

	PlatformState platform = {};
	platform_init(&platform);

	timespec last_time = get_time();
	while (true) {
		timespec current_time = get_time();
		i64 difference        = get_time_difference(last_time, current_time);
		last_time             = current_time;

		f32 dt = (f32)difference / 1000000000.0f;
		assert(difference >= 0);

        #if 0
		code_reload_timer += dt;
		if (code_reload_timer >= code_reload_rate) {
			struct stat st;
			int result = stat(lib_path, &st);
			assert(result == 0);

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
