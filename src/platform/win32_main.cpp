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
#include <cstdlib>

#include <Windows.h>

#include "core/settings.h"
#include "render/vulkan/vulkan_device.h"

#include "win32_debug.cpp"
#include "win32_vulkan.cpp"

namespace {
	void quit() 
	{
		Settings::get()->save("settings.ini");
		_exit(EXIT_SUCCESS);
	}

	LRESULT CALLBACK
	window_proc(HWND hwnd, 
	            UINT message, 
	            WPARAM wparam, 
	            LPARAM lparam)
	{
		switch (message) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

			EndPaint(hwnd, &ps);
			break;
		}

		case WM_KEYUP:
		{
			switch (wparam) {
			default:
				std::printf("unhandled key release event: %d\n", wparam);
				break;
			}
			break;
		}

		case WM_KEYDOWN:
		{
			switch (wparam) {
			case VK_ESCAPE:
				quit();
				break;

			default:
				std::printf("unhandled key press event: %d\n", wparam);
				break;
			}
			break;
		}

		default:
			std::printf("unhandled event: %d\n", message);
			return DefWindowProc(hwnd, message, wparam, lparam);

		}

		return 0;
	}
}

int main()
{
	
}

int 
WinMain(HINSTANCE instance,
        HINSTANCE prev_instance,
    	LPSTR cmd_line,
    	int cmd_show) 
{
	Settings::create();
	Settings *settings = Settings::get();

	settings->load("settings.ini");

	PlatformState state;
	state.win32.hinstance = instance;

	WNDCLASS wc = {};
	wc.lpfnWndProc   = window_proc;
	wc.hInstance     = instance;
	wc.lpszClassName = "leary";

	RegisterClass(&wc);

	state.win32.hwnd = CreateWindow("leary", 
	                                "leary", 
	                                WS_TILED | WS_VISIBLE, 
	                                0, 
	                                0, 
	                                settings->video.resolution.width, 
	                                settings->video.resolution.height, 
	                                nullptr, 
	                                nullptr, 
	                                instance, 
	                                nullptr);

	if (state.win32.hwnd == nullptr)
		quit();

	VulkanDevice vulkan_device;
	vulkan_device.create(state);

	MSG msg;
	while(true)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			quit();	
			

		vulkan_device.present();
	}

	return 0;
}
