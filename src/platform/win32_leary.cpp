/**
 * file:    win32_leary.cpp
 * created: 2017-03-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#include <stdio.h>

#include "platform.h"
#include "leary.h"

#include "win32_debug.cpp"
#include "win32_file.cpp"
#include "win32_input.cpp"
#include "win32_thread.cpp"

PlatformState   *g_platform;
Settings         g_settings;

Allocator *g_heap;
Allocator *g_frame;
Allocator *g_debug_frame;
Allocator *g_persistent;
Allocator *g_stack;
Allocator *g_system_alloc;

#include "win32_vulkan.cpp"
#include "leary.cpp"
#include "generated/type_info.h"

struct CatalogThreadData {
    FolderPathView folder;
    catalog_callback_t *callback;
};

struct MouseState {
    f32 x, y;
    f32 dx, dy;
    bool in_window = false;
};

extern "C"
DWORD catalog_thread_process(void *data)
{
    CatalogThreadData *ctd = (CatalogThreadData*)data;

    HANDLE fh = CreateFile(
        ctd->folder.absolute.bytes,
        GENERIC_READ | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    ASSERT(fh != INVALID_HANDLE_VALUE);

    bool eslash = ctd->folder[ctd->folder.absolute.size - 1] == '\\';

    DWORD buffer[2048];
    DWORD bytes = 0;

    while (ReadDirectoryChangesW(fh,
                                 buffer, sizeof buffer,
                                 true, FILE_NOTIFY_CHANGE_LAST_WRITE,
                                 &bytes, NULL, NULL) != FALSE)
    {
        DWORD *ptr = buffer;

        FILE_NOTIFY_INFORMATION *fni;
        do {
            fni = (FILE_NOTIFY_INFORMATION*)ptr;
            ptr += fni->NextEntryOffset;

            if (fni->Action == FILE_ACTION_MODIFIED) {
                ASSERT(fni->FileNameLength <= I32_MAX);

                FilePath p;
                if (eslash) {
                    p = create_file_path( g_system_alloc, ctd->folder, fni->FileName);
                } else {
                    p = create_file_path(g_system_alloc, ctd->folder, '\\', fni->FileName);
                }

                ctd->callback(p);
            }
        } while (fni->NextEntryOffset > 0);
    }

    CloseHandle(fh);
    return 0;
}

void create_catalog_thread(Array<FolderPath> folders, catalog_callback_t *callback)
{
    for (auto f : folders) {
        auto data = ialloc<CatalogThreadData>(g_persistent);
        data->folder   = f;
        data->callback = callback;

        HANDLE th = CreateThread(NULL,
                                 8 * 1024,
                                 &catalog_thread_process, data,
                                 0, NULL);
        ASSERT(th != NULL);
    }
}

void platform_quit()
{
    FilePath settings_path = resolve_file_path(GamePath_preferences, "settings.conf", g_stack);
    serialize_save_conf(settings_path, Settings_members,
                        ARRAY_SIZE(Settings_members), &g_settings);

    _exit(EXIT_SUCCESS);
}

void platform_toggle_raw_mouse()
{
    g_platform->raw_mouse = !g_platform->raw_mouse;
    LOG("toggle raw mouse: %d", g_platform->raw_mouse);

    if (g_platform->raw_mouse) {
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_INPUTSINK | RIDEV_CAPTUREMOUSE | RIDEV_NOLEGACY;
        rid[0].hwndTarget = g_platform->native.hwnd;

        bool result = RegisterRawInputDevices(rid, 1, sizeof rid[0]);
        ASSERT(result != false);
        (void)result;

        while(ShowCursor(false) > 0) {}
    } else {
        RAWINPUTDEVICE rid[1];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x02;
        rid[0].dwFlags = RIDEV_REMOVE;
        rid[0].hwndTarget = 0;

        bool result = RegisterRawInputDevices(rid, 1, sizeof rid[0]);
        ASSERT(result != false);
        (void)result;

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
    case WM_CLOSE:
    case WM_QUIT:
        game_quit();
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
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

        if (!mouse_state.in_window) {
            mouse_state.in_window = false;
            mouse_state.dx = 0.0f;
            mouse_state.dy = 0.0f;
            mouse_state.x = (f32)x;
            mouse_state.y = (f32)y;
            break;
        }

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

        game_input(event);
    } break;
    case WM_LBUTTONDOWN: {
        if (g_platform->raw_mouse) {
            break;
        }

        InputEvent event;
        event.type = InputType_mouse_press;
        event.mouse.x = (f32)(lparam & 0xffff);
        event.mouse.y = (f32)((lparam >> 16) & 0xffff);

        game_input(event);
    } break;
    case WM_MOUSELEAVE:
        mouse_state.in_window = false;
        break;
    case WM_INPUT: {
        u32 size;
        GetRawInputData((HRAWINPUT)lparam, RID_INPUT, NULL,
                        &size, sizeof(RAWINPUTHEADER));

        u8 *data = new u8[size];
        defer { delete[] data; };

        ASSERT(data != nullptr);

        UINT result = GetRawInputData((HRAWINPUT)lparam, RID_INPUT, data,
                                      &size, sizeof(RAWINPUTHEADER));
        if (result != size) {
            LOG(Log_error,
                "incorrect size from GetRawInputData. Expected: %u, received %u",
                size, result);
            ASSERT(false);
            break;
        }

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
                LOG(Log_error, "unsupported flags");
                ASSERT(false);
            }

            InputEvent event;
            event.type = InputType_mouse_move;
            event.mouse.dx = mouse_state.dx;
            event.mouse.dy = mouse_state.dy;
            event.mouse.x  = mouse_state.x;
            event.mouse.y  = mouse_state.y;

            game_input(event);

            DefWindowProc(hwnd, message, wparam, lparam);
        } break;
        default:
            LOG("unhandled raw input device type: %d", raw->header.dwType);
            break;
        }
    } break;
    default:
        //LOG(Log_info, "unhandled event: %d", message);
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    return 0;
}

DL_EXPORT
PLATFORM_INIT_FUNC(platform_init)
{
    g_platform = platform;
    g_settings = {};

    auto native = &g_platform->native;
    native->hinstance = instance;

    isize frame_size       = 64  * 1024 * 1024;
    isize debug_frame_size = 64  * 1024 * 1024;
    isize persistent_size  = 256 * 1024 * 1024;
    isize heap_size        = 256 * 1024 * 1024;
    isize stack_size       = 16  * 1024 * 1024;

    // TODO(jesper): alloc these using appropriate syscalls
    void *frame_mem       = malloc(frame_size);
    void *debug_frame_mem = malloc(debug_frame_size);
    void *persistent_mem  = malloc(persistent_size);
    void *heap_mem        = malloc(heap_size);
    void *stack_mem       = malloc(stack_size);

    g_platform->allocators.heap        = heap_allocator(heap_mem, heap_size);
    g_platform->allocators.debug_frame = linear_allocator(debug_frame_mem, debug_frame_size);
    g_platform->allocators.frame       = linear_allocator(frame_mem, frame_size);
    g_platform->allocators.persistent  = linear_allocator(persistent_mem, persistent_size);
    g_platform->allocators.stack       = stack_allocator(stack_mem, stack_size);
    g_platform->allocators.system      = system_allocator();

    g_heap         = &g_platform->allocators.heap;
    g_debug_frame  = &g_platform->allocators.debug_frame;
    g_frame        = &g_platform->allocators.frame;
    g_persistent   = &g_platform->allocators.persistent;
    g_stack        = &g_platform->allocators.stack;
    g_system_alloc = &g_platform->allocators.system;

    init_paths(g_persistent);

    FilePath settings_path = resolve_file_path(GamePath_preferences, "settings.conf", g_frame);
    serialize_load_conf(settings_path, Settings_members,
                        ARRAY_SIZE(Settings_members), &g_settings);

    WNDCLASS wc = {};
    wc.lpfnWndProc   = window_proc;
    wc.hInstance     = instance;
    wc.lpszClassName = "leary";

    RegisterClass(&wc);

    native->hwnd = CreateWindow(
        "leary", "leary",
        WS_TILED | WS_VISIBLE | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
        0, 0,
        g_settings.video.resolution.width,
        g_settings.video.resolution.height,
        nullptr, nullptr, native->hinstance, nullptr);

    if (native->hwnd == NULL) {
        char* msg = win32_system_error_message(GetLastError());
        LOG(Log_error, "failed to create window: %s", msg);
        game_quit();
    }

    game_init();
}

DL_EXPORT
PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload)
{
    platform->reload_state.game = game_pre_reload();

    platform->reload_state.frame        = g_frame;
    platform->reload_state.debug_frame  = g_debug_frame;
    platform->reload_state.stack        = g_stack;
    platform->reload_state.heap         = g_heap;
    platform->reload_state.persistent   = g_persistent;
    platform->reload_state.system_alloc = g_system_alloc;
}

DL_EXPORT
PLATFORM_RELOAD_FUNC(platform_reload)
{
    g_frame        = platform->reload_state.frame;
    g_debug_frame  = platform->reload_state.debug_frame;
    g_heap         = platform->reload_state.heap;
    g_persistent   = platform->reload_state.persistent;
    g_stack        = platform->reload_state.stack;
    g_system_alloc = platform->reload_state.system_alloc;
    g_platform     = platform;

    game_reload(platform->reload_state.game);
}

DL_EXPORT
PLATFORM_UPDATE_FUNC(platform_update)
{
    (void)platform;

    profiler_begin_frame();
    game_begin_frame();

    PROFILE_START(win32_input);
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    PROFILE_END(win32_input);

    game_update_and_render(dt);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD fwd, LPVOID reserved)
{
    (void)instance;
    (void)fwd;
    (void)reserved;

    return TRUE;
}
