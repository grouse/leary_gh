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

#include <xcb/xcb.h>

#include "platform_main.h"

#include "linux_debug.cpp"
#include "linux_vulkan.cpp"
#include "linux_file.cpp"

#include "leary.cpp"

namespace  {
	Settings      settings;
	PlatformState platform_state;
	GameState     game_state;
}

void platform_quit()
{
	xcb_destroy_window(platform_state.window.xcb.connection,
	                   platform_state.window.xcb.window);
	exit(EXIT_SUCCESS);
}

timespec get_time()
{
	timespec ts;
	i32 result = clock_gettime(CLOCK_MONOTONIC, &ts);
	DEBUG_ASSERT(result == 0);
	return ts;
}

long get_time_difference(timespec start, timespec end)
{
	i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
	                 (end.tv_nsec - start.tv_nsec);
	DEBUG_ASSERT(difference >= 0);
	return difference;
}

int main()
{
	platform_state = {};
	game_state     = {};
	settings       = {};

	game_load_settings(&settings);

	platform_state.window.xcb.connection = xcb_connect(nullptr, nullptr);

	const xcb_setup_t *setup  = xcb_get_setup(platform_state.window.xcb.connection);
	xcb_screen_t      *screen = xcb_setup_roots_iterator(setup).data;

	platform_state.window.xcb.window = xcb_generate_id(platform_state.window.xcb.connection);

	u32 mask     = XCB_CW_EVENT_MASK;
	u32 values[] = {
		XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_KEY_PRESS      |
		XCB_EVENT_MASK_KEY_RELEASE
	};

	xcb_create_window(platform_state.window.xcb.connection,
	                  XCB_COPY_FROM_PARENT,
	                  platform_state.window.xcb.window,
	                  screen->root,
	                  0, 0,
	                  settings.video.resolution.width,
	                  settings.video.resolution.height,
	                  0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT,
	                  screen->root_visual,
	                  mask,
	                  values);


	auto wm_protocols_cookie     = xcb_intern_atom(platform_state.window.xcb.connection,
	                                               0, 12, "WM_PROTOCOLS");
	auto wm_delete_window_cookie = xcb_intern_atom(platform_state.window.xcb.connection,
	                                               0, 16, "WM_DELETE_WINDOW");

	auto wm_protocols_reply      = xcb_intern_atom_reply(platform_state.window.xcb.connection,
	                                                     wm_protocols_cookie, 0);
	auto wm_delete_window_reply  = xcb_intern_atom_reply(platform_state.window.xcb.connection,
	                                                     wm_delete_window_cookie, 0);

	xcb_change_property(platform_state.window.xcb.connection,
	                    XCB_PROP_MODE_REPLACE,
	                    platform_state.window.xcb.window,
	                    wm_protocols_reply->atom,
	                    4,
	                    32,
	                    1,
	                    &wm_delete_window_reply->atom);

	xcb_map_window(platform_state.window.xcb.connection, platform_state.window.xcb.window);
	xcb_flush(platform_state.window.xcb.connection);

	game_init(&settings, &platform_state, &game_state);

	timespec last_time = get_time();

	while (true)
	{
		timespec current_time = get_time();
		long difference = get_time_difference(last_time, current_time);
		last_time = current_time;
		f32 dt = (f32)difference / 1000000000.0f;
		DEBUG_ASSERT(difference >= 0);

		DEBUG_LOGF(LogType::info, "frame time: %ld ns", difference);

		xcb_generic_event_t *event;
		while ((event = xcb_poll_for_event(platform_state.window.xcb.connection)))
		{
			switch (event->response_type & ~0x80) {
			case XCB_CLIENT_MESSAGE:
			{
				auto message = (xcb_client_message_event_t*)event;

				if (message->data.data32[0] == wm_delete_window_reply->atom)
					game_quit(&settings, &game_state);

				break;
			}
			case XCB_KEY_RELEASE:
			{
				auto key = (xcb_key_release_event_t*)event;

				// check if the release event is a repeat event by checking its pressed state.
				// NOTE(grouse): doing this with a keymap because the usual Xlib method of peeking
				// the timestamp of the next event seems to be unreliable.
				// NOTE(grouse): getting the keymap for every key press event is likely a horrible
				// performance penalty, look into either fixing the timestamp method or caching the
				// keymap.
				auto keymap_cookie = xcb_query_keymap(platform_state.window.xcb.connection);
				auto keymap = xcb_query_keymap_reply(platform_state.window.xcb.connection, keymap_cookie, nullptr);

				if (keymap->keys[key->detail / 8] & (1 << (key->detail % 8)))
					break;

				switch (key->detail) {
				case 40: /* D */
					game_input(&game_state, InputAction_move_horizontal_end);
					break;
				case 38: /* A */
					game_input(&game_state, InputAction_move_horizontal_end);
					break;
				case 39: /* S */
					game_input(&game_state, InputAction_move_vertical_end);
					break;
				case 25: /* W */
					game_input(&game_state, InputAction_move_vertical_end);
					break;
				default:
					std::printf("unhandled key release event: %d\n", key->detail);
				}

				break;
			}
			case XCB_KEY_PRESS:
			{
				auto key = (xcb_key_press_event_t*)event;

				switch (key->detail) {
				case 9: /* ESC */
					game_quit(&settings, &game_state);
					break;
				case 40: /* D */
					game_input(&game_state, InputAction_move_horizontal_start, 1.0f);
					break;
				case 38: /* A */
					game_input(&game_state, InputAction_move_horizontal_start, -1.0f);
					break;
				case 39: /* S */
					game_input(&game_state, InputAction_move_vertical_start, 1.0f);
					break;
				case 25: /* W */
					game_input(&game_state, InputAction_move_vertical_start, -1.0f);
					break;
				default:
#if 0
					std::printf("unhandled key press event: %d\n", key->detail);
#endif
					break;
				}

				break;
			}
			default:
#if 0
				std::printf("unhandled event: %d\n", event->response_type);
#endif
				break;
			}
		}

		game_update(&game_state, dt);
		game_render(&game_state);
	}

	return 0;
}
