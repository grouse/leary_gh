/**
 * @file:   win32_main.cpp
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

#include <cstdio>
#include <cstdint>
#include <cinttypes>
#include <cstdlib>

#include "platform.h"

#include "win32_debug.cpp"
#include "win32_vulkan.cpp"
#include "win32_file.cpp"
#include "win32_input.cpp"

// TODO(jesper): move this into its own translation unit. Eventually i want to be able to compile
// the game into a .dll and load it dynamically in the platform layer, if feature is turned off,
// for live code editing.
#include "leary.cpp"

namespace {
	Settings      settings = {};
	PlatformState platform_state = {};
	GameState     game_state = {};
}

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

struct MouseState {
	f32 x, y;
	f32 dx, dy;
};

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
		event.type = InputType_key_press;
		event.key.vkey = (VirtualKey)wparam;
		event.key.repeated = lparam & 0x40000000;

		game_input(&game_state, &platform_state, &settings, event);
	} break;
	case WM_KEYUP: {
		InputEvent event;
		event.type = InputType_key_release;
		event.key.vkey = (VirtualKey)wparam;
		event.key.repeated = lparam & 0x40000000;

		game_input(&game_state, &platform_state, &settings, event);
	} break;
	case WM_MOUSEMOVE: {
		if (platform_state.raw_mouse) {
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

		game_input(&game_state, &platform_state, &settings, event);
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
			if (!platform_state.raw_mouse) {
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

			game_input(&game_state, &platform_state, &settings, event);
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

int WINAPI
WinMain(HINSTANCE instance,
        HINSTANCE /*prev_instance*/,
        LPSTR     /*cmd_line*/,
        int       /*cmd_show*/)
{
	profile_init();
	game_load_settings(&settings);

	platform_state.win32.hinstance = instance;

	WNDCLASS wc = {};
	wc.lpfnWndProc   = window_proc;
	wc.hInstance     = instance;
	wc.lpszClassName = "leary";

	RegisterClass(&wc);

	platform_state.win32.hwnd = CreateWindow("leary",
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

	if (platform_state.win32.hwnd == nullptr) {
		platform_quit(&platform_state);
	}


	game_init(&settings, &platform_state, &game_state);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER last_time;
	QueryPerformanceCounter(&last_time);

	MSG msg;
	while(true) {
		profile_start_frame();

		LARGE_INTEGER current_time;
		QueryPerformanceCounter(&current_time);
		i64 elapsed = current_time.QuadPart - last_time.QuadPart;
		last_time = current_time;

		f32 dt = (f32)elapsed / frequency.QuadPart;

		PROFILE_START(win32_input)
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT) {
			game_quit(&game_state, &platform_state, &settings);
		}
		PROFILE_END(win32_input);


		game_update_and_render(&game_state, dt);

		profile_end_frame();
	}

	return 0;
}
