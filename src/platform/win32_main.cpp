/**
 * file:   win32_main.cpp
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2015-2018 Jesper Stefansson
 */

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

    //DeleteFile(".\\leary_loaded.dll");
    //BOOL result = CopyFile(".\\game.dll", ".\\game_loaded.dll", true);
    //result = CopyFile(".\\game.pdb", ".\\game_loaded.pdb", true);
    //(void)result;
    
    // NOTE(jesper): commented out because it is the only place using assert in the main layer. do something else here?
    //ASSERT(result != FALSE);

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
    (void)code_reload_timer;

    while(true) {
        LARGE_INTEGER current_time;
        QueryPerformanceCounter(&current_time);
        i64 elapsed = current_time.QuadPart - last_time.QuadPart;
        last_time   = current_time;

        f32 dt = (f32)elapsed / frequency.QuadPart;

        // TODO(jesper) let's re-introduce this with a file watcher later
        #if 0
        code_reload_timer += dt;
        if (code_reload_timer >= code_reload_rate) {
            reload_code(&platform, lib);
            code_reload_timer = 0.0f;
        }
        #endif


        platform_update(&platform, dt);
    }

    return 0;
}
