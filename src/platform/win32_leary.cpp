/**
 * file:    win32_leary.cpp
 * created: 2017-03-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <stdio.h>

#include "core/types.h"
#include "platform.h"

#include "win32_debug.cpp"
#include "win32_file.cpp"
#include "win32_input.cpp"

PlatformState   *g_platform;
Settings         g_settings;
HeapAllocator   *g_heap;
LinearAllocator *g_frame;
LinearAllocator *g_debug_frame;
LinearAllocator *g_persistent;
StackAllocator  *g_stack;

struct Win32State {
    HINSTANCE hinstance;
    HWND      hwnd;
};

#include "win32_vulkan.cpp"
#include "leary.cpp"
#include "generated/type_info.h"

void create_catalog_thread(Array<char*> folders, catalog_callback_t *callback)
{
    (void)folders;
    (void)callback;
}


void init_mutex(Mutex *m)
{
    m->native = CreateMutex(NULL, FALSE, NULL);
}

void lock_mutex(Mutex *m)
{
    WaitForSingleObject(m->native, INFINITE);
}

void unlock_mutex(Mutex *m)
{
    ReleaseMutex(m->native);
}

struct MouseState {
    f32 x, y;
    f32 dx, dy;
};

void platform_quit()
{
    char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
    serialize_save_conf(settings_path, Settings_members,
                        ARRAY_SIZE(Settings_members), &g_settings);

    _exit(EXIT_SUCCESS);
}

void platform_toggle_raw_mouse()
{
    g_platform->raw_mouse = !g_platform->raw_mouse;
    DEBUG_LOG("toggle raw mouse: %d", g_platform->raw_mouse);

    if (g_platform->raw_mouse) {
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_NOLEGACY;
        rid[0].hwndTarget = 0;

        if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == false) {
            DEBUG_ASSERT(false);
        }

        while(ShowCursor(false) > 0) {}
    } else {
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_REMOVE;
        rid[0].hwndTarget = 0;

        if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == false) {
            DEBUG_ASSERT(false);
        }

        while(ShowCursor(true) > 0) {}
    }
}

void platform_set_raw_mouse(bool enable)
{
    if ((enable && !g_platform->raw_mouse) ||
        (!enable && g_platform->raw_mouse))
    {
        platform_toggle_raw_mouse();
    }
}

LRESULT CALLBACK
window_proc(HWND   hwnd,
            UINT   message,
            WPARAM wparam,
            LPARAM lparam)
{
    static MouseState mouse_state = {};

    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

        EndPaint(hwnd, &ps);
    } break;
    case WM_KEYDOWN: {
        InputEvent event;
        event.type         = InputType_key_press;
        event.key.code     = win32_keycode(wparam);
        event.key.repeated = lparam & 0x40000000;

        game_input(event);
    } break;
    case WM_KEYUP: {
        InputEvent event;
        event.type         = InputType_key_release;
        event.key.code     = win32_keycode(wparam);
        event.key.repeated = false;

        game_input(event);
    } break;
    case WM_MOUSEMOVE: {
        if (g_platform->raw_mouse) {
            break;
        }

        i32 x = lparam & 0xffff;
        i32 y = (lparam >> 16) & 0xffff;

        mouse_state.dx = (f32)(x - mouse_state.x);
        mouse_state.dy = (f32)(y - mouse_state.y);
        mouse_state.x  = (f32)x;
        mouse_state.y  = (f32)y;

        InputEvent event;
        event.type = InputType_mouse_move;
        event.mouse.x  = mouse_state.x;
        event.mouse.y  = mouse_state.y;
        event.mouse.dx = mouse_state.dx;
        event.mouse.dy = mouse_state.dy;

        game_input( event);
    } break;
    case WM_INPUT: {
        u32 size;
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL,
                        &size, sizeof(RAWINPUTHEADER));

        u8 *data = new u8[size];
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, data,
                        &size, sizeof(RAWINPUTHEADER));

        RAWINPUT *raw = (RAWINPUT*)data;

        switch (raw->header.dwType) {
        case RIM_TYPEMOUSE: {
            if (!g_platform->raw_mouse) {
                break;
            }

            if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
                mouse_state.dx = (f32)raw->data.mouse.lLastX;
                mouse_state.dy = (f32)raw->data.mouse.lLastY;
                mouse_state.x  += (f32)raw->data.mouse.lLastX;
                mouse_state.y  += (f32)raw->data.mouse.lLastY;
            } else if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                mouse_state.dx = (f32)raw->data.mouse.lLastX - mouse_state.x;
                mouse_state.dy = (f32)raw->data.mouse.lLastY - mouse_state.y;
                mouse_state.x  = (f32)raw->data.mouse.lLastX;
                mouse_state.y  = (f32)raw->data.mouse.lLastY;
            } else {
                DEBUG_LOG(Log_error, "unsupported flags");
                DEBUG_ASSERT(false);
            }

            InputEvent event;
            event.type = InputType_mouse_move;
            event.mouse.dx = mouse_state.dx;
            event.mouse.dy = mouse_state.dy;
            event.mouse.x  = mouse_state.x;
            event.mouse.y  = mouse_state.y;

            game_input(event);
        } break;
        default:
            DEBUG_LOG("unhandled raw input device type: %d",
                      raw->header.dwType);
            break;
        }
    } break;
    default:
        std::printf("unhandled event: %d\n", message);
        return DefWindowProc(hwnd, message, wparam, lparam);

    }

    return 0;
}

DL_EXPORT
PLATFORM_INIT_FUNC(platform_init)
{
    g_platform = platform;
    g_settings = {};

    char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
    serialize_load_conf(settings_path, Settings_members,
                        ARRAY_SIZE(Settings_members), &g_settings);

    isize frame_size      = 64  * 1024 * 1024;
    isize persistent_size = 256 * 1024 * 1024;
    isize heap_size       = 256 * 1024 * 1024;
    isize stack_size      = 16  * 1024 * 1024;

    // TODO(jesper): allocate these using appropriate syscalls
    void *frame_mem      = malloc(frame_size);
    void *persistent_mem = malloc(persistent_size);
    void *heap_mem       = malloc(heap_size);
    void *stack_mem      = malloc(stack_size);

    g_heap       = new HeapAllocator  (heap_mem,       heap_size);
    g_frame      = new LinearAllocator(frame_mem,      frame_size);
    g_persistent = new LinearAllocator(persistent_mem, persistent_size);
    g_stack      = new StackAllocator (stack_mem,      stack_size);

    Win32State *native = g_persistent->talloc<Win32State>();
    g_platform->native = native;

    native->hinstance = instance;

    WNDCLASS wc = {};
    wc.lpfnWndProc   = window_proc;
    wc.hInstance     = instance;
    wc.lpszClassName = "leary";

    RegisterClass(&wc);

    native->hwnd = CreateWindow("leary",
                                "leary",
                                WS_TILED | WS_VISIBLE,
                                0,
                                0,
                                g_settings.video.resolution.width,
                                g_settings.video.resolution.height,
                                nullptr,
                                nullptr,
                                instance,
                                nullptr);

    if (native->hwnd == nullptr) {
        platform_quit();
    }

    game_init();
}

DL_EXPORT
PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload)
{
    platform->game_reload_state = game_pre_reload();

    platform->frame      = g_frame;
    platform->heap       = g_heap;
    platform->persistent = g_persistent;
    platform->stack      = g_stack;
}

DL_EXPORT
PLATFORM_RELOAD_FUNC(platform_reload)
{
    g_frame      = platform->frame;
    g_heap       = platform->heap;
    g_persistent = platform->persistent;
    g_stack      = platform->stack;
    g_platform   = platform;

    game_reload(platform->game_reload_state);
}

DL_EXPORT
PLATFORM_UPDATE_FUNC(platform_update)
{
    (void)platform;
    profile_start_frame();

    PROFILE_START(win32_input);
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (msg.message == WM_QUIT) {
        game_quit();
    }
    PROFILE_END(win32_input);

    //Win32State *native = (Win32State*)platform->native;

    game_update_and_render(dt);
    profile_end_frame();
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD fwd, LPVOID reserved)
{
    (void)instance;
    (void)fwd;
    (void)reserved;

    return TRUE;
}
