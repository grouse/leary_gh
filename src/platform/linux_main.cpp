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
	Settings      settings       = {};
	PlatformState platform_state = {};
	GameState     game_state     = {};
}

void platform_toggle_raw_mouse(PlatformState *state) {
	state->raw_mouse = !state->raw_mouse;
	DEBUG_LOG("raw mouse mode set to: %d", state->raw_mouse);

	if (state->raw_mouse) {
		XGrabPointer(platform_state.x11.display,
		             platform_state.x11.window,
		             false,
		             (KeyPressMask | KeyReleaseMask) & 0,
		             GrabModeAsync,
		             GrabModeAsync,
		             None,
		             platform_state.x11.hidden_cursor,
		             CurrentTime);
	} else {
		XUngrabPointer(platform_state.x11.display, CurrentTime);
	}
}

void platform_quit(PlatformState *state)
{
	XUngrabPointer(state->x11.display, CurrentTime);
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

int main()
{
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
	             KeyPressMask | KeyReleaseMask | StructureNotifyMask |
	             PointerMotionMask | EnterWindowMask);
	XMapWindow(display, window);

	platform_state.x11.window  = window;
	platform_state.x11.display = display;

	Atom WM_DELETE_WINDOW = XInternAtom(display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(display, window, &WM_DELETE_WINDOW, 1);

	i32 xkb_major = XkbMajorVersion;
	i32 xkb_minor = XkbMinorVersion;

	if (XkbQueryExtension(display, NULL, NULL, NULL, &xkb_major, &xkb_minor)) {
		platform_state.x11.xkb = XkbGetMap(display,
		                                   XkbAllClientInfoMask,
		                                   XkbUseCoreKbd);
		DEBUG_LOG("Initialised XKB version %d.%d", xkb_major, xkb_minor);
	}

	{
		i32 event, error;

		if (!XQueryExtension(display, "XInputExtension",
		                     &platform_state.x11.xi2_opcode,
		                     &event, &error))
		{
			DEBUG_ASSERT(false && "XInput2 extension not found, this is required");
			return 0;
		}

		u8 mask[3] = { 0, 0, 0 };

		XIEventMask emask;
		emask.deviceid = XIAllMasterDevices;
		emask.mask_len = sizeof(mask);
		emask.mask     = mask;

		XISetMask(mask, XI_RawMotion);
		XISetMask(mask, XI_RawButtonPress);
		XISetMask(mask, XI_RawButtonRelease);

		XISelectEvents(display, DefaultRootWindow(display), &emask, 1);
		XFlush(display);
	}

	{ // create hidden cursor used when raw mouse is enabled
		XColor xcolor;
		char csr_bits[] = { 0x00 };

		Pixmap csr = XCreateBitmapFromData(platform_state.x11.display,
		                                   platform_state.x11.window,
		                                   csr_bits, 1, 1);
		Cursor cursor = XCreatePixmapCursor(platform_state.x11.display,
		                                    csr, csr, &xcolor, &xcolor, 1, 1);
		platform_state.x11.hidden_cursor = cursor;
	}

	game_init(&settings, &platform_state, &game_state);

	i32 num_screens = XScreenCount(platform_state.x11.display);

	i32 mouse_x = 0;
	i32 mouse_y = 0;

	for (i32 i = 0; i < num_screens; i++) {
		Window root = XRootWindow(platform_state.x11.display, i);
		Window child;

		u32 mask;
		i32 root_x, root_y, win_x, win_y;
		bool result = XQueryPointer(platform_state.x11.display,
		                            platform_state.x11.window,
		                            &root, &child,
		                            &root_x, &root_y,
		                            &win_x, &win_y,
		                            &mask);
		if (result) {
			mouse_x = win_x;
			mouse_y = win_y;
			break;
		}
	}


	XEvent xevent;

	timespec last_time = get_time();
	while (true) {
		timespec current_time = get_time();
		i64 difference = get_time_difference(last_time, current_time);
		last_time = current_time;
		f32 dt = (f32)difference / 1000000000.0f;
		DEBUG_ASSERT(difference >= 0);

		{ // HACK(jesper): reset mouse dx
			InputEvent event;
			event.type = InputType_mouse_move;
			event.mouse.x = mouse_x;
			event.mouse.y = mouse_y;
			event.mouse.dx = 0;
			event.mouse.dy = 0;

			game_input(&game_state, &platform_state, &settings, event);
		}

		while (XPending(platform_state.x11.display) > 0) {
			XNextEvent(platform_state.x11.display, &xevent);

			switch (xevent.type) {
			case KeyPress: {
				InputEvent event;
				event.type = InputType_key_press;
				event.key.vkey = (VirtualKey)xevent.xkey.keycode;
				event.key.repeated = false;

				game_input(&game_state, &platform_state, &settings, event);
			} break;
			case KeyRelease: {
				InputEvent event;
				event.type = InputType_key_release;
				event.key.vkey = (VirtualKey)xevent.xkey.keycode;
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

				game_input(&game_state, &platform_state, &settings, event);
			} break;
			case MotionNotify: {
				if (platform_state.raw_mouse) {
					break;
				}

				InputEvent event;
				event.type = InputType_mouse_move;
				event.mouse.x = xevent.xmotion.x;
				event.mouse.y = xevent.xmotion.y;

				event.mouse.dx = xevent.xmotion.x - mouse_x;
				event.mouse.dy = xevent.xmotion.y - mouse_y;

				mouse_x = xevent.xmotion.x;
				mouse_y = xevent.xmotion.y;

				game_input(&game_state, &platform_state, &settings, event);
			} break;
			case EnterNotify: {
				if (xevent.xcrossing.focus == true &&
				    xevent.xcrossing.window == platform_state.x11.window &&
				    xevent.xcrossing.display == platform_state.x11.display)
				{
					mouse_x = xevent.xcrossing.x;
					mouse_y = xevent.xcrossing.y;
				}
			} break;
			case ClientMessage: {
				if ((Atom)xevent.xclient.data.l[0] == WM_DELETE_WINDOW) {
					game_quit(&game_state, &platform_state, &settings);
				}
			} break;
			case GenericEvent: {
				if (xevent.xcookie.extension == platform_state.x11.xi2_opcode &&
				    XGetEventData(platform_state.x11.display, &xevent.xcookie))
				{
					switch (xevent.xcookie.evtype) {
					case XI_RawMotion: {
						if (!platform_state.raw_mouse) {
							break;
						}

						static Time prev_time = 0;
						static f64  prev_deltas[2];

						XIRawEvent *revent = (XIRawEvent*)xevent.xcookie.data;

						f64 deltas[2];

						u8  *mask    = revent->valuators.mask;
						i32 mask_len = revent->valuators.mask_len;
						f64 *ivalues = revent->raw_values;

						i32 top = MIN(mask_len * 8, 16);
						for (i32 i = 0, j = 0; i < top && j < 2; i++,j++) {
							if (XIMaskIsSet(mask, i)) {
								deltas[j] = *ivalues++;
							}
						}

						if (revent->time == prev_time &&
						    deltas[0] == prev_deltas[0] &&
						    deltas[1] == prev_deltas[1])
						{
							// NOTE(jesper): discard duplicate events,
							// apparently can happen?
							break;
						}

						prev_deltas[0] = deltas[0];
						prev_deltas[1] = deltas[1];

						InputEvent event = {};
						event.type = InputType_mouse_move;
						event.mouse.dx = deltas[0];
						event.mouse.dy = deltas[1];

						game_input(&game_state, &platform_state, &settings,
						           event);
					} break;
					default:
						DEBUG_LOG("unhandled xinput2 event: %d",
						          xevent.xcookie.evtype);
						break;
					}
				} else {
					DEBUG_LOG("unhandled generic event");
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
