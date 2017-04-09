/**
 * file:    platform_input.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PLATFORM_INPUT_H
#define PLATFORM_INPUT_H

enum VirtualKey {
	VirtualKey_escape,
	VirtualKey_F1,
	VirtualKey_F2,
	VirtualKey_F3,
	VirtualKey_F4,
	VirtualKey_F5,
	VirtualKey_F6,
	VirtualKey_F7,
	VirtualKey_F8,
	VirtualKey_F9,
	VirtualKey_F10,
	VirtualKey_F11,
	VirtualKey_F12,
	VirtualKey_tilde,
	VirtualKey_1,
	VirtualKey_2,
	VirtualKey_3,
	VirtualKey_4,
	VirtualKey_5,
	VirtualKey_6,
	VirtualKey_7,
	VirtualKey_8,
	VirtualKey_9,
	VirtualKey_0,
	VirtualKey_hyphen,
	VirtualKey_plus,
	VirtualKey_backspace,
	VirtualKey_tab,
	VirtualKey_Q,
	VirtualKey_W,
	VirtualKey_E,
	VirtualKey_R,
	VirtualKey_T,
	VirtualKey_Y,
	VirtualKey_U,
	VirtualKey_I,
	VirtualKey_O,
	VirtualKey_P,
	VirtualKey_lbracket,
	VirtualKey_rbracket,
	VirtualKey_bslash,
	VirtualKey_A,
	VirtualKey_S,
	VirtualKey_D,
	VirtualKey_F,
	VirtualKey_G,
	VirtualKey_H,
	VirtualKey_J,
	VirtualKey_K,
	VirtualKey_L,
	VirtualKey_scolon,
	VirtualKey_quote,
	VirtualKey_enter,
	VirtualKey_lshift,
	VirtualKey_Z,
	VirtualKey_X,
	VirtualKey_C,
	VirtualKey_V,
	VirtualKey_B,
	VirtualKey_N,
	VirtualKey_M,
	VirtualKey_comma,
	VirtualKey_period,
	VirtualKey_fslash,
	VirtualKey_rshift,
	VirtualKey_lctrl,
	VirtualKey_lalt,
	VirtualKey_space,
	VirtualKey_ralt,
	VirtualKey_rsuper,
	VirtualKey_menu,
	VirtualKey_rctrl,

	VirtualKey_left,
	VirtualKey_up,
	VirtualKey_right,
	VirtualKey_down,

	VirtualKey_unknown
};

enum InputType {
	InputType_key_release,
	InputType_key_press,
	InputType_mouse_move
};

struct InputEvent {
	InputType type;
	union {
		struct {
			VirtualKey vkey;
			bool repeated;
		} key;
		struct {
			f32 dx, dy;
			f32 x, y;
		} mouse;
	};
};


#endif /* PLATFORM_INPUT_H */

