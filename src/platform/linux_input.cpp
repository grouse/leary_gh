/**
 * file:    linux_input.cpp
 * created: 2017-02-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

enum VirtualKey {
	VirtualKey_escape    = 9,
	VirtualKey_F1        = 67,
	VirtualKey_F2        = 68,
	VirtualKey_F3        = 69,
	VirtualKey_F4        = 70,
	VirtualKey_F5        = 71,
	VirtualKey_F6        = 72,
	VirtualKey_F7        = 73,
	VirtualKey_F8        = 74,
	VirtualKey_F9        = 75,
	VirtualKey_F10       = 76,
	VirtualKey_F11       = 95,
	VirtualKey_F12       = 96,
	VirtualKey_tilde     = 49,
	VirtualKey_1         = 10,
	VirtualKey_2         = 11,
	VirtualKey_3         = 12,
	VirtualKey_4         = 13,
	VirtualKey_5         = 14,
	VirtualKey_6         = 15,
	VirtualKey_7         = 16,
	VirtualKey_8         = 17,
	VirtualKey_9         = 18,
	VirtualKey_0         = 19,
	VirtualKey_hyphen    = 20,
	VirtualKey_plus      = 21,
	VirtualKey_backspace = 22,
	VirtualKey_tab       = 23,
	VirtualKey_Q         = 24,
	VirtualKey_W         = 25,
	VirtualKey_E         = 26,
	VirtualKey_R         = 27,
	VirtualKey_T         = 28,
	VirtualKey_Y         = 29,
	VirtualKey_U         = 30,
	VirtualKey_I         = 31,
	VirtualKey_O         = 32,
	VirtualKey_P         = 33,
	VirtualKey_lbracket  = 34,
	VirtualKey_rbracket  = 35,
	VirtualKey_bslash    = 51,
	VirtualKey_A         = 38,
	VirtualKey_S         = 39,
	VirtualKey_D         = 40,
	VirtualKey_F         = 41,
	VirtualKey_G         = 42,
	VirtualKey_H         = 43,
	VirtualKey_J         = 44,
	VirtualKey_K         = 45,
	VirtualKey_L         = 46,
	VirtualKey_scolon    = 47,
	VirtualKey_quote     = 48,
	VirtualKey_enter     = 36,
	VirtualKey_lshift    = 50,
	VirtualKey_Z         = 52,
	VirtualKey_X         = 53,
	VirtualKey_C         = 54,
	VirtualKey_V         = 55,
	VirtualKey_B         = 56,
	VirtualKey_N         = 57,
	VirtualKey_M         = 58,
	VirtualKey_comma     = 59,
	VirtualKey_period    = 60,
	VirtualKey_fslash    = 61,
	VirtualKey_rshift    = 62,
	VirtualKey_lctrl     = 37,
	VirtualKey_lalt      = 64,
	VirtualKey_space     = 65,
	VirtualKey_ralt      = 108,
	VirtualKey_rsuper    = 134,
	VirtualKey_menu      = 135,
	VirtualKey_rctrl     = 105
};

enum InputType {
	InputType_key_release,
	InputType_key_press
};

struct InputEvent {
	InputType type;
	union {
		struct {
			VirtualKey vkey;
			bool repeated;
		} key;
	};
};

