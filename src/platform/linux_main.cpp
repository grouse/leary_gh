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

#include <dlfcn.h>

#include <time.h>

#include "platform.h"

#include "linux_debug.cpp"
#include "linux_file.cpp"
#include "linux_input.cpp"

#include "leary.h"
#include "core/profiling.h"
#include "core/math.h"

#include "generated/type_info.h"

typedef GAME_INIT_FUNC(game_init_t);
typedef GAME_LOAD_PLATFORM_CODE_FUNC(game_load_platform_code_t);
typedef GAME_QUIT_FUNC(game_quit_t);
typedef GAME_INPUT_FUNC(game_input_t);
typedef GAME_UPDATE_AND_RENDER_FUNC(game_update_and_render_t);

typedef MAKE_LINEAR_ALLOCATOR_FUNC(make_linear_allocator_t);
typedef MAKE_STACK_ALLOCATOR_FUNC(make_stack_allocator_t);
typedef PROFILE_INIT_FUNC(profile_init_t);
typedef PROFILE_START_FRAME_FUNC(profile_start_frame_t);
typedef PROFILE_END_FRAME_FUNC(profile_end_frame_t);
typedef PROFILE_START_TIMER_FUNC(profile_start_timer_t);
typedef PROFILE_END_TIMER_FUNC(profile_end_timer_t);
typedef SERIALIZE_LOAD_CONF_FUNC(serialize_load_conf_t);
typedef SERIALIZE_SAVE_CONF_FUNC(serialize_save_conf_t);

static game_init_t               *game_init;
static game_load_platform_code_t *game_load_platform_code;
static game_quit_t               *game_quit;
static game_input_t              *game_input;
static game_update_and_render_t  *game_update_and_render;

static make_linear_allocator_t   *make_linear_allocator;
static make_stack_allocator_t    *make_stack_allocator;
static profile_init_t            *profile_init;
static profile_start_frame_t     *profile_start_frame;
static profile_end_frame_t       *profile_end_frame;
static profile_start_timer_t     *profile_start_timer;
static profile_end_timer_t       *profile_end_timer;
static serialize_load_conf_t     *serialize_load_conf;
static serialize_save_conf_t     *serialize_save_conf;

#define DLOAD_FUNC(lib, name, result) name = (name##_t*)dlsym(lib, #name); result = result && name
void* load_game_code()
{
	char *path = platform_resolve_path(GamePath_binary, "game.so");
	void *lib = dlopen(path, RTLD_NOW);

	if (lib) {
		bool valid = true;
		DLOAD_FUNC(lib, game_init, valid);
		DLOAD_FUNC(lib, game_load_platform_code, valid);
		DLOAD_FUNC(lib, game_quit, valid);
		DLOAD_FUNC(lib, game_input, valid);
		DLOAD_FUNC(lib, game_update_and_render, valid);

		DLOAD_FUNC(lib, make_linear_allocator, valid);
		DLOAD_FUNC(lib, make_stack_allocator, valid);

		DLOAD_FUNC(lib, profile_init, valid);
		DLOAD_FUNC(lib, profile_start_frame, valid);
		DLOAD_FUNC(lib, profile_end_frame, valid);
		DLOAD_FUNC(lib, profile_start_timer, valid);
		DLOAD_FUNC(lib, profile_end_timer, valid);

		DLOAD_FUNC(lib, serialize_load_conf, valid);
		DLOAD_FUNC(lib, serialize_save_conf, valid);

		if (!valid) {
			dlclose(lib);
			lib = nullptr;
		}
	}

	return lib;
}

struct LinuxState {
	Window     window;
	Display    *display;
	XkbDescPtr xkb;
	i32        xinput2;
	Cursor     hidden_cursor;

	void *game_lib;

	struct {
		i32 x;
		i32 y;
	} mouse;
};

#include "linux_vulkan.cpp"

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
	SERIALIZE_SAVE_CONF(settings_path, Settings, &platform->settings);

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

PlatformCode make_platform_code()
{
	// NOTE(jesper): I wanted to preprocess macro this in the same vein as in
	// platform.h, but couldn't wrestle the stupid preprocessor to my will
	PlatformCode code = {};
	code.toggle_raw_mouse                 = &platform_toggle_raw_mouse;
	code.set_raw_mouse                    = &platform_set_raw_mouse;
	code.quit                             = &platform_quit;
	code.vulkan_create_surface            = &platform_vulkan_create_surface;
	code.vulkan_enable_instance_extension = &platform_vulkan_enable_instance_extension;
	code.vulkan_enable_instance_layer     = &platform_vulkan_enable_instance_layer;
	code.resolve_relative                 = &platform_resolve_relative;
	code.resolve_path                     = &platform_resolve_path;
	code.file_exists                      = &platform_file_exists;
	code.file_create                      = &platform_file_create;
	code.file_open                        = &platform_file_open;
	code.file_close                       = &platform_file_close;
	code.file_write                       = &platform_file_write;
	code.file_read                        = &platform_file_read;
	return code;
}

int main()
{
	LinuxState native = {};
	PlatformCode code = make_platform_code();

	native.game_lib = load_game_code();
	DEBUG_ASSERT(native.game_lib != nullptr);

	PlatformState platform = {};
	platform.native           = &native;

	game_load_platform_code(&code);

	char *settings_path = platform_resolve_path(GamePath_preferences, "settings.conf");
	SERIALIZE_LOAD_CONF(settings_path, Settings, &platform.settings);

	isize frame_alloc_size      = 64 * 1024 * 1024;
	isize persistent_alloc_size = 256 * 1024 * 1024;

	// TODO(jesper): allocate these using linux call
	u8 *mem = (u8*)malloc(frame_alloc_size + persistent_alloc_size);

	GameMemory memory = {};
	memory.frame      = make_linear_allocator(mem, frame_alloc_size);
	memory.persistent = make_linear_allocator(mem + frame_alloc_size,
	                                          persistent_alloc_size);

	profile_init(&memory);

	native.display = XOpenDisplay(nullptr);
	i32 screen     = DefaultScreen(native.display);
	native.window  = XCreateSimpleWindow(native.display,
	                                     DefaultRootWindow(native.display),
	                                     0, 0,
	                                     platform.settings.video.resolution.width,
	                                     platform.settings.video.resolution.height,
	                                     2, BlackPixel(native.display, screen),
	                                     BlackPixel(native.display, screen));

	XSelectInput(native.display, native.window,
	             KeyPressMask | KeyReleaseMask | StructureNotifyMask |
	             PointerMotionMask | EnterWindowMask);
	XMapWindow(native.display, native.window);

	Atom WM_DELETE_WINDOW = XInternAtom(native.display, "WM_DELETE_WINDOW", false);
	XSetWMProtocols(native.display, native.window, &WM_DELETE_WINDOW, 1);

	i32 xkb_major = XkbMajorVersion;
	i32 xkb_minor = XkbMinorVersion;

	if (XkbQueryExtension(native.display, NULL, NULL, NULL, &xkb_major, &xkb_minor)) {
		native.xkb = XkbGetMap(native.display, XkbAllClientInfoMask, XkbUseCoreKbd);
		DEBUG_LOG("Initialised XKB version %d.%d", xkb_major, xkb_minor);
	}

	{
		i32 event, error;
		if (!XQueryExtension(native.display, "XInputExtension",
		                     &native.xinput2,
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

		XISelectEvents(native.display, DefaultRootWindow(native.display),
		               &emask, 1);
		XFlush(native.display);
	}

	{ // create hidden cursor used when raw mouse is enabled
		XColor xcolor;
		char csr_bits[] = { 0x00 };

		Pixmap csr = XCreateBitmapFromData(native.display, native.window,
		                                   csr_bits, 1, 1);
		native.hidden_cursor = XCreatePixmapCursor(native.display,
		                                           csr, csr, &xcolor, &xcolor,
		                                           1, 1);
	}

	game_init(&memory, &platform);

	i32 num_screens = XScreenCount(native.display);
	for (i32 i = 0; i < num_screens; i++) {
		Window root = XRootWindow(native.display, i);
		Window child;

		u32 mask;
		i32 root_x, root_y, win_x, win_y;
		bool result = XQueryPointer(native.display, native.window,
		                            &root, &child,
		                            &root_x, &root_y,
		                            &win_x, &win_y,
		                            &mask);
		if (result) {
			native.mouse.x = win_x;
			native.mouse.y = win_y;
			break;
		}
	}

	XEvent xevent;

	timespec last_time = get_time();
	while (true) {
		profile_start_frame();

		timespec current_time = get_time();
		i64 difference = get_time_difference(last_time, current_time);
		last_time = current_time;
		f32 dt = (f32)difference / 1000000000.0f;
		DEBUG_ASSERT(difference >= 0);

		PROFILE_START(linux_input);

		while (XPending(native.display) > 0) {
			XNextEvent(native.display, &xevent);

			switch (xevent.type) {
			case KeyPress: {
				InputEvent event;
				event.type         = InputType_key_press;
				event.key.vkey     = keycode_to_virtual(xevent.xkey.keycode);
				event.key.repeated = false;

				game_input(&memory, &platform, event);
			} break;
			case KeyRelease: {
				InputEvent event;
				event.type         = InputType_key_release;
				event.key.vkey     = keycode_to_virtual(xevent.xkey.keycode);
				event.key.repeated = false;

				if (XEventsQueued(native.display, QueuedAfterReading)) {
					XEvent next;
					XPeekEvent(native.display, &next);

					if (next.type == KeyPress &&
					    next.xkey.time == xevent.xkey.time &&
					    next.xkey.keycode == xevent.xkey.keycode)
					{
						event.key.repeated = true;
					}
				}

				game_input(&memory, &platform, event);
			} break;
			case MotionNotify: {
				if (platform.raw_mouse) {
					break;
				}

				InputEvent event;
				event.type = InputType_mouse_move;
				event.mouse.x = xevent.xmotion.x;
				event.mouse.y = xevent.xmotion.y;

				event.mouse.dx = xevent.xmotion.x - native.mouse.x;
				event.mouse.dy = xevent.xmotion.y - native.mouse.y;

				native.mouse.x = xevent.xmotion.x;
				native.mouse.y = xevent.xmotion.y;

				game_input(&memory, &platform, event);
			} break;
			case EnterNotify: {
				if (xevent.xcrossing.focus == true &&
				    xevent.xcrossing.window == native.window &&
				    xevent.xcrossing.display == native.display)
				{
					native.mouse.x = xevent.xcrossing.x;
					native.mouse.y = xevent.xcrossing.y;
				}
			} break;
			case ClientMessage: {
				if ((Atom)xevent.xclient.data.l[0] == WM_DELETE_WINDOW) {
					game_quit(&memory, &platform);
				}
			} break;
			case GenericEvent: {
				if (xevent.xcookie.extension == native.xinput2 &&
				    XGetEventData(native.display, &xevent.xcookie))
				{
					switch (xevent.xcookie.evtype) {
					case XI_RawMotion: {
						if (!platform.raw_mouse) {
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

						game_input(&memory, &platform, event);
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

		game_update_and_render(&memory, dt);

		profile_end_frame();
	}

	return 0;
}
