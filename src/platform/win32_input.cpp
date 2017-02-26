/**
 * file:    win32_input.cpp
 * created: 2017-02-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

enum VirtualKey {
	VirtualKey_escape    = VK_ESCAPE,
	VirtualKey_F1        = VK_F1,
	VirtualKey_F2        = VK_F2,
	VirtualKey_F3        = VK_F3,
	VirtualKey_F4        = VK_F4,
	VirtualKey_F5        = VK_F5,
	VirtualKey_F6        = VK_F6,
	VirtualKey_F7        = VK_F7,
	VirtualKey_F8        = VK_F8,
	VirtualKey_F9        = VK_F9,
	VirtualKey_F10       = VK_F10,
	VirtualKey_F11       = VK_F11,
	VirtualKey_F12       = VK_F12,
	VirtualKey_tilde     = VK_OEM_3,
	VirtualKey_0         = 0x30,
	VirtualKey_1         = 0x31,
	VirtualKey_2         = 0x32,
	VirtualKey_3         = 0x33,
	VirtualKey_4         = 0x34,
	VirtualKey_5         = 0x35,
	VirtualKey_6         = 0x36,
	VirtualKey_7         = 0x37,
	VirtualKey_8         = 0x38,
	VirtualKey_9         = 0x39,
	VirtualKey_hyphen    = VK_OEM_MINUS,
	VirtualKey_plus      = VK_OEM_PLUS,
	VirtualKey_backspace = VK_BACK,
	VirtualKey_tab       = VK_TAB,
	VirtualKey_A         = 0x41,
	VirtualKey_B         = 0x42,
	VirtualKey_C         = 0x43,
	VirtualKey_D         = 0x44,
	VirtualKey_E         = 0x45,
	VirtualKey_F         = 0x46,
	VirtualKey_G         = 0x47,
	VirtualKey_H         = 0x48,
	VirtualKey_I         = 0x49,
	VirtualKey_J         = 0x4A,
	VirtualKey_K         = 0x4B,
	VirtualKey_L         = 0x4C,
	VirtualKey_M         = 0x4D,
	VirtualKey_N         = 0x4E,
	VirtualKey_O         = 0x4F,
	VirtualKey_P         = 0x50,
	VirtualKey_Q         = 0x51,
	VirtualKey_R         = 0x52,
	VirtualKey_S         = 0x53,
	VirtualKey_T         = 0x54,
	VirtualKey_U         = 0x55,
	VirtualKey_V         = 0x56,
	VirtualKey_W         = 0x57,
	VirtualKey_X         = 0x58,
	VirtualKey_Y         = 0x59,
	VirtualKey_Z         = 0x5A,
	VirtualKey_lbracket  = VK_OEM_4,
	VirtualKey_rbracket  = VK_OEM_6,
	VirtualKey_bslash    = VK_OEM_5,
	VirtualKey_scolon    = VK_OEM_1,
	VirtualKey_quote     = VK_OEM_7,
	VirtualKey_enter     = VK_RETURN,
	VirtualKey_lshift    = VK_LSHIFT,
	VirtualKey_comma     = VK_OEM_COMMA,
	VirtualKey_period    = VK_OEM_PERIOD,
	VirtualKey_fslash    = VK_OEM_2,
	VirtualKey_rshift    = VK_RSHIFT,
	VirtualKey_lctrl     = VK_LCONTROL,
	VirtualKey_lalt      = VK_LMENU,
	VirtualKey_space     = VK_SPACE,
	VirtualKey_ralt      = VK_RMENU,
	VirtualKey_rsuper    = VK_RWIN,
	VirtualKey_menu      = VK_APPS,
	VirtualKey_rctrl     = VK_RCONTROL
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
