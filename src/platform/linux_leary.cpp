/**
 * file:    linux_leary.cpp
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "core/types.h"

#include "platform.h"

#include "platform/linux_debug.cpp"
#include "platform/linux_file.cpp"
#include "platform/linux_input.cpp"


struct LinuxState {
	Window     window;
	Display    *display;
	XkbDescPtr xkb;
	i32        xinput2;
	Cursor     hidden_cursor;

	Atom WM_DELETE_WINDOW;

	struct {
		i32 x;
		i32 y;
	} mouse;
};

void platform_toggle_raw_mouse(PlatformState *platform);
void platform_set_raw_mouse(PlatformState *platform, bool enable);
void platform_quit(PlatformState *platform);

#include "platform/linux_vulkan.cpp"

#include "leary.cpp"

#include "generated/type_info.h"

void platform_toggle_raw_mouse(PlatformState *platform)
{
	LinuxState *native = (LinuxState*)platform->native;

	platform->raw_mouse = !platform->raw_mouse;
	DEBUG_LOG("raw mouse mode set to: %d", platform->raw_mouse);

	if (platform->raw_mouse) {
		XGrabPointer(native->display, native->window, false,
		             (KeyPressMask | KeyReleaseMask) & 0,
		             GrabModeAsync, GrabModeAsync,
		             native->window, native->hidden_cursor, CurrentTime);
	} else {
		XUngrabPointer(native->display, CurrentTime);

		i32 num_screens = XScreenCount(native->display);

		for (i32 i = 0; i < num_screens; i++) {
			Window root = XRootWindow(native->display, i);
			Window child;

			u32 mask;
			i32 root_x, root_y, win_x, win_y;
			bool result = XQueryPointer(native->display, native->window,
			                            &root, &child,
			                            &root_x, &root_y, &win_x, &win_y,
			                            &mask);
			if (result) {
				native->mouse.x = win_x;
				native->mouse.y = win_y;
				break;
			}
		}
	}

	XFlush(native->display);
}

void platform_set_raw_mouse(PlatformState *platform, bool enable)
{
	if ((enable && !platform->raw_mouse) ||
	    (!enable && platform->raw_mouse))
	{
		platform_toggle_raw_mouse(platform);
	}
}

void platform_quit(PlatformState *platform)
{
	LinuxState *native = (LinuxState*)platform->native;
	XUngrabPointer(native->display, CurrentTime);

	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	serialize_save_conf(settings_path, Settings_members,
	                    ARRAY_SIZE(Settings_members), &platform->settings);

	// TODO(jesper): do we need to unload the .so ?
	exit(EXIT_SUCCESS);
}

DL_EXPORT
PLATFORM_INIT_FUNC(platform_init)
{
	isize frame_size      = 64  * 1024 * 1024;
	isize persistent_size = 256 * 1024 * 1024;
	isize free_list_size  = 256 * 1024 * 1024;

	// TODO(jesper): allocate these using linux call
	u8 *mem = (u8*)malloc(frame_size + persistent_size + free_list_size);

	platform->memory = {};
	platform->memory.frame      = make_linear_allocator(mem, frame_size);
	mem += frame_size;

	platform->memory.free_list  = make_free_list_allocator(mem, free_list_size);
	mem += free_list_size;

	platform->memory.persistent = make_linear_allocator(mem, persistent_size);

	LinuxState *native = alloc<LinuxState>(&platform->memory.persistent);
	platform->native   = native;

	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	serialize_load_conf(settings_path, Settings_members,
	                    ARRAY_SIZE(Settings_members), &platform->settings);

	platform->profile = profile_init(&platform->memory);

	native->display = XOpenDisplay(nullptr);
	i32 screen     = DefaultScreen(native->display);
	native->window  = XCreateSimpleWindow(native->display,
	                                     DefaultRootWindow(native->display),
	                                     0, 0,
	                                     platform->settings.video.resolution.width,
	                                     platform->settings.video.resolution.height,
	                                     2, BlackPixel(native->display, screen),
	                                     BlackPixel(native->display, screen));

	XSelectInput(native->display, native->window,
	             KeyPressMask | KeyReleaseMask | StructureNotifyMask |
	             PointerMotionMask | EnterWindowMask);
	XMapWindow(native->display, native->window);

	native->WM_DELETE_WINDOW = XInternAtom(native->display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(native->display, native->window, &native->WM_DELETE_WINDOW, 1);

	i32 xkb_major = XkbMajorVersion;
	i32 xkb_minor = XkbMinorVersion;

	if (XkbQueryExtension(native->display, NULL, NULL, NULL, &xkb_major, &xkb_minor)) {
		native->xkb = XkbGetMap(native->display, XkbAllClientInfoMask, XkbUseCoreKbd);
		DEBUG_LOG("Initialised XKB version %d.%d", xkb_major, xkb_minor);
	}

	{
		i32 event, error;
		if (!XQueryExtension(native->display, "XInputExtension",
		                     &native->xinput2,
		                     &event, &error))
		{
			DEBUG_ASSERT(false && "XInput2 extension not found, this is required");
			platform_quit(platform);
		}

		u8 mask[3] = { 0, 0, 0 };

		XIEventMask emask;
		emask.deviceid = XIAllMasterDevices;
		emask.mask_len = sizeof(mask);
		emask.mask     = mask;

		XISetMask(mask, XI_RawMotion);
		XISetMask(mask, XI_RawButtonPress);
		XISetMask(mask, XI_RawButtonRelease);

		XISelectEvents(native->display, DefaultRootWindow(native->display),
		               &emask, 1);
		XFlush(native->display);
	}

	{ // create hidden cursor used when raw mouse is enabled
		XColor xcolor;
		char csr_bits[] = { 0x00 };

		Pixmap csr = XCreateBitmapFromData(native->display, native->window,
		                                   csr_bits, 1, 1);
		native->hidden_cursor = XCreatePixmapCursor(native->display,
		                                           csr, csr, &xcolor, &xcolor,
		                                           1, 1);
	}

	game_init(&platform->memory, platform);

	i32 num_screens = XScreenCount(native->display);
	for (i32 i = 0; i < num_screens; i++) {
		Window root = XRootWindow(native->display, i);
		Window child;

		u32 mask;
		i32 root_x, root_y, win_x, win_y;
		bool result = XQueryPointer(native->display, native->window,
		                            &root, &child,
		                            &root_x, &root_y,
		                            &win_x, &win_y,
		                            &mask);
		if (result) {
			native->mouse.x = win_x;
			native->mouse.y = win_y;
			break;
		}
	}
}

DL_EXPORT
PLATFORM_PRE_RELOAD_FUNC(platform_pre_reload)
{
	game_pre_reload(&platform->memory);
}

DL_EXPORT
PLATFORM_RELOAD_FUNC(platform_reload)
{
	profile_set_state(&platform->profile);
	game_reload(&platform->memory);
}

DL_EXPORT
PLATFORM_UPDATE_FUNC(platform_update)
{
	profile_start_frame();

	LinuxState *native = (LinuxState*)platform->native;

	PROFILE_START(linux_input);
	XEvent xevent;
	while (XPending(native->display) > 0) {
		XNextEvent(native->display, &xevent);

		switch (xevent.type) {
		case KeyPress: {
			InputEvent event;
			event.type         = InputType_key_press;
			event.key.vkey     = keycode_to_virtual(xevent.xkey.keycode);
			event.key.repeated = false;

			game_input(&platform->memory, platform, event);
		} break;
		case KeyRelease: {
			InputEvent event;
			event.type         = InputType_key_release;
			event.key.vkey     = keycode_to_virtual(xevent.xkey.keycode);
			event.key.repeated = false;

			if (XEventsQueued(native->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(native->display, &next);

				if (next.type == KeyPress &&
					next.xkey.time == xevent.xkey.time &&
					next.xkey.keycode == xevent.xkey.keycode)
				{
					event.key.repeated = true;
				}
			}

			game_input(&platform->memory, platform, event);
		} break;
		case MotionNotify: {
			if (platform->raw_mouse) {
				break;
			}

			InputEvent event;
			event.type = InputType_mouse_move;
			event.mouse.x = xevent.xmotion.x;
			event.mouse.y = xevent.xmotion.y;

			event.mouse.dx = xevent.xmotion.x - native->mouse.x;
			event.mouse.dy = xevent.xmotion.y - native->mouse.y;

			native->mouse.x = xevent.xmotion.x;
			native->mouse.y = xevent.xmotion.y;

			game_input(&platform->memory, platform, event);
		} break;
		case EnterNotify: {
			if (xevent.xcrossing.focus == true &&
				xevent.xcrossing.window == native->window &&
				xevent.xcrossing.display == native->display)
			{
				native->mouse.x = xevent.xcrossing.x;
				native->mouse.y = xevent.xcrossing.y;
			}
		} break;
		case ClientMessage: {
			if ((Atom)xevent.xclient.data.l[0] == native->WM_DELETE_WINDOW) {
				game_quit(&platform->memory, platform);
			}
		} break;
		case GenericEvent: {
			if (xevent.xcookie.extension == native->xinput2 &&
				XGetEventData(native->display, &xevent.xcookie))
			{
				switch (xevent.xcookie.evtype) {
				case XI_RawMotion: {
					if (!platform->raw_mouse) {
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
					event.type     = InputType_mouse_move;
					event.mouse.dx = deltas[0];
					event.mouse.dy = deltas[1];

					game_input(&platform->memory, platform, event);
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
	PROFILE_END(linux_input);

	game_update_and_render(&platform->memory, dt);
	profile_end_frame();
}
