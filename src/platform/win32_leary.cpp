/**
 * file:    win32_leary.cpp
 * created: 2017-03-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <stdio.h>

#include "platform.h"

struct Win32State {
	HINSTANCE hinstance;
	HWND      hwnd;
};

#include "win32_debug.cpp"
#include "win32_vulkan.cpp"
#include "win32_file.cpp"
#include "win32_input.cpp"

void platform_toggle_raw_mouse(PlatformState *state);
void platform_set_raw_mouse(PlatformState *state, bool enable);
void platform_quit(PlatformState *);

#include "leary.cpp"

static PlatformState *g_platform = nullptr;

struct MouseState {
	f32 x, y;
	f32 dx, dy;
};

void platform_quit(PlatformState *)
{
	_exit(EXIT_SUCCESS);
}

void platform_toggle_raw_mouse(PlatformState *state)
{
	state->raw_mouse = !state->raw_mouse;
	DEBUG_LOG("toggle raw mouse: %d", state->raw_mouse);

	if (state->raw_mouse) {
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

void platform_set_raw_mouse(PlatformState *state, bool enable)
{
	if ((enable && !state->raw_mouse) ||
	    (!enable && state->raw_mouse))
	{
		platform_toggle_raw_mouse(state);
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
		event.key.vkey     = wparam_to_virtual(wparam);
		event.key.repeated = lparam & 0x40000000;

		game_input(&g_platform->memory, g_platform, event);
	} break;
	case WM_KEYUP: {
		InputEvent event;
		event.type         = InputType_key_release;
		event.key.vkey     = wparam_to_virtual(wparam);
		event.key.repeated = false;

		game_input(&g_platform->memory, g_platform, event);
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

		game_input(&g_platform->memory, g_platform, event);
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

			game_input(&g_platform->memory, g_platform, event);
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

	Settings settings = {};

	isize frame_size      = 64  * 1024 * 1024;
	isize persistent_size = 256 * 1024 * 1024;
	isize free_list_size  = 256 * 1024 * 1024;
	isize stack_size      = 16  * 1024 * 1024;

	// TODO(jesper): allocate these using VirtualAlloc?
	u8 *mem = (u8*)malloc(frame_size + persistent_size + free_list_size +
	                      stack_size);

	platform->memory = {};
	platform->memory.frame      = allocator_create(Allocator_Linear, mem, frame_size);
	mem += frame_size;

	platform->memory.free_list  = allocator_create(Allocator_FreeList, mem, free_list_size);
	mem += free_list_size;

	platform->memory.persistent = allocator_create(Allocator_Linear, mem, persistent_size);
	mem += persistent_size;

	platform->memory.stack = allocator_create(Allocator_Stack, mem, stack_size);

	Win32State *native = alloc<Win32State>(&platform->memory.persistent);
	platform->native = native;

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
	                            settings.video.resolution.width,
	                            settings.video.resolution.height,
	                            nullptr,
	                            nullptr,
	                            instance,
	                            nullptr);

	if (native->hwnd == nullptr) {
		platform_quit(platform);
	}

	game_init(&platform->memory, platform);
}

DL_EXPORT
PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload)
{
	game_pre_reload(&platform->memory);
}

DL_EXPORT
PLATFORM_RELOAD_FUNC(platform_reload)
{
	g_platform = platform;

	game_reload(&platform->memory);
}

DL_EXPORT
PLATFORM_UPDATE_FUNC(platform_update)
{
	profile_start_frame();

	PROFILE_START(win32_input);
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (msg.message == WM_QUIT) {
		game_quit(&platform->memory, platform);
	}
	PROFILE_END(win32_input);

	//Win32State *native = (Win32State*)platform->native;

	game_update_and_render(&platform->memory, dt);
	profile_end_frame();
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD fwd, LPVOID reserved)
{
	(void)instance;
	(void)fwd;
	(void)reserved;

	return TRUE;
}
