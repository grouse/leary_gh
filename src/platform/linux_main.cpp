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

#include <time.h>

#include "platform_main.h"

#include "linux_debug.cpp"
#include "linux_vulkan.cpp"
#include "linux_file.cpp"
#include "linux_input.cpp"

#include "leary.cpp"

namespace  {
	Settings      settings;
	PlatformState platform_state;
	GameState     game_state;
}

struct KeyState {
	bool just_pressed  = false;
	bool pressed       = false;
	bool just_released = false;
	bool released      = true;
};


void platform_quit()
{
	exit(EXIT_SUCCESS);
}

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

VirtualKey keycode_to_virtual(PlatformState *platform, KeyCode code)
{
	// NOTE(jesper): PERFORMANCE: is this slow? profile and consider cache. I
	// believe there are events we can listen to for when the map become out of
	// date because of switched keyboard layouts
	KeySym sym = XkbKeycodeToKeysym(platform->x11.display, code, 0, 0);

	// NOTE(jesper): I don't know how this can happen, but we only want one
	// version of each key. I'm thinking maybe caps lock causes this?
	if (sym >= 0x0061 && sym <= 0x007a) {
		sym -= 0x0061 - 0x0041;
	}

	// NOTE(jesper): rudimentary assert for this, there are holes in the enum,
	// warrants further investigation. Especially for non-latin keyboards
	// NOTE(jesper): I think the best way to go about this is to generate an
	// array containing all the enum values using meta programming that we can
	// then binary search and see whether the enum contains the keysym
	//DEBUG_ASSERT(sym < VirtualKey_max);
	return (VirtualKey)sym;
}

int main()
{
	platform_state = {};
	game_state     = {};
	settings       = {};

	game_load_settings(&settings);

	Display *display = XOpenDisplay(nullptr);
	i32 screen       = DefaultScreen(display);
	Window window    = XCreateSimpleWindow(display, DefaultRootWindow(display),
	                                       0, 0,
	                                       settings.video.resolution.width,
	                                       settings.video.resolution.height,
	                                       2, BlackPixel(display, screen),
	                                       BlackPixel(display, screen));

	XSelectInput(display, window,
	             KeyPressMask | KeyReleaseMask | StructureNotifyMask);
	XMapWindow(display, window);

	Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);


	XkbDescPtr xkb = nullptr;
	i32 xkb_major = XkbMajorVersion;
	i32 xkb_minor = XkbMinorVersion;

	if (XkbQueryExtension(display, NULL, NULL, NULL, &xkb_major, &xkb_minor)) {
		xkb = XkbGetMap(display, XkbAllClientInfoMask, XkbUseCoreKbd);
	}

	platform_state.x11.window  = window;
	platform_state.x11.display = display;
	platform_state.x11.xkb     = xkb;

	game_init(&settings, &platform_state, &game_state);

	XEvent xevent;
	timespec last_time = get_time();
	while (true)
	{
		timespec current_time = get_time();
		i64 difference = get_time_difference(last_time, current_time);
		last_time = current_time;
		f32 dt = (f32)difference / 1000000000.0f;
		DEBUG_ASSERT(difference >= 0);

		//DEBUG_LOGF(LogType::info, "frame time: %ld ns", difference);
		//
		while (XPending(platform_state.x11.display) > 0) {
			XNextEvent(platform_state.x11.display, &xevent);
			switch (xevent.type) {
			case KeyPress: {
				InputEvent event;
				event.type = InputType_key_press;
				event.key.vkey = keycode_to_virtual(&platform_state,
				                                    xevent.xkey.keycode);
				event.key.repeated = false;

				game_input(&game_state, &settings, event);
			} break;
			case KeyRelease: {
				InputEvent event;
				event.type = InputType_key_release;
				event.key.vkey = keycode_to_virtual(&platform_state,
				                                    xevent.xkey.keycode);
				event.key.repeated = false;

				if (XEventsQueued(platform_state.x11.display,
				                  QueuedAfterReading))
				{
					XEvent next;
					XPeekEvent(platform_state.x11.display, &next);

					if (next.type == KeyPress &&
					    next.xkey.time == xevent.xkey.time &&
					    next.xkey.keycode == xevent.xkey.keycode)
					{
						event.key.repeated = true;
					}
				}

				game_input(&game_state, &settings, event);
			} break;
			case ClientMessage: {
				if ((Atom)xevent.xclient.data.l[0] == WM_DELETE_WINDOW) {
					game_quit(&game_state, &settings);
				}
			} break;
			default: {
				DEBUG_LOG("unhandled xevent type: %d", xevent.type);
			} break;
			}
		}


		game_update_and_render(&game_state, dt);
	}

	return 0;
}
