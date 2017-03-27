/**
 * file:    win32_input.cpp
 * created: 2017-02-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

enum Win32VirtualKey {
	Win32VirtualKey_escape    = VK_ESCAPE,
	Win32VirtualKey_F1        = VK_F1,
	Win32VirtualKey_F2        = VK_F2,
	Win32VirtualKey_F3        = VK_F3,
	Win32VirtualKey_F4        = VK_F4,
	Win32VirtualKey_F5        = VK_F5,
	Win32VirtualKey_F6        = VK_F6,
	Win32VirtualKey_F7        = VK_F7,
	Win32VirtualKey_F8        = VK_F8,
	Win32VirtualKey_F9        = VK_F9,
	Win32VirtualKey_F10       = VK_F10,
	Win32VirtualKey_F11       = VK_F11,
	Win32VirtualKey_F12       = VK_F12,
	Win32VirtualKey_tilde     = VK_OEM_3,
	Win32VirtualKey_0         = 0x30,
	Win32VirtualKey_1         = 0x31,
	Win32VirtualKey_2         = 0x32,
	Win32VirtualKey_3         = 0x33,
	Win32VirtualKey_4         = 0x34,
	Win32VirtualKey_5         = 0x35,
	Win32VirtualKey_6         = 0x36,
	Win32VirtualKey_7         = 0x37,
	Win32VirtualKey_8         = 0x38,
	Win32VirtualKey_9         = 0x39,
	Win32VirtualKey_hyphen    = VK_OEM_MINUS,
	Win32VirtualKey_plus      = VK_OEM_PLUS,
	Win32VirtualKey_backspace = VK_BACK,
	Win32VirtualKey_tab       = VK_TAB,
	Win32VirtualKey_A         = 0x41,
	Win32VirtualKey_B         = 0x42,
	Win32VirtualKey_C         = 0x43,
	Win32VirtualKey_D         = 0x44,
	Win32VirtualKey_E         = 0x45,
	Win32VirtualKey_F         = 0x46,
	Win32VirtualKey_G         = 0x47,
	Win32VirtualKey_H         = 0x48,
	Win32VirtualKey_I         = 0x49,
	Win32VirtualKey_J         = 0x4A,
	Win32VirtualKey_K         = 0x4B,
	Win32VirtualKey_L         = 0x4C,
	Win32VirtualKey_M         = 0x4D,
	Win32VirtualKey_N         = 0x4E,
	Win32VirtualKey_O         = 0x4F,
	Win32VirtualKey_P         = 0x50,
	Win32VirtualKey_Q         = 0x51,
	Win32VirtualKey_R         = 0x52,
	Win32VirtualKey_S         = 0x53,
	Win32VirtualKey_T         = 0x54,
	Win32VirtualKey_U         = 0x55,
	Win32VirtualKey_V         = 0x56,
	Win32VirtualKey_W         = 0x57,
	Win32VirtualKey_X         = 0x58,
	Win32VirtualKey_Y         = 0x59,
	Win32VirtualKey_Z         = 0x5A,
	Win32VirtualKey_lbracket  = VK_OEM_4,
	Win32VirtualKey_rbracket  = VK_OEM_6,
	Win32VirtualKey_bslash    = VK_OEM_5,
	Win32VirtualKey_scolon    = VK_OEM_1,
	Win32VirtualKey_quote     = VK_OEM_7,
	Win32VirtualKey_enter     = VK_RETURN,
	Win32VirtualKey_lshift    = VK_LSHIFT,
	Win32VirtualKey_comma     = VK_OEM_COMMA,
	Win32VirtualKey_period    = VK_OEM_PERIOD,
	Win32VirtualKey_fslash    = VK_OEM_2,
	Win32VirtualKey_rshift    = VK_RSHIFT,
	Win32VirtualKey_lctrl     = VK_LCONTROL,
	Win32VirtualKey_lalt      = VK_LMENU,
	Win32VirtualKey_space     = VK_SPACE,
	Win32VirtualKey_ralt      = VK_RMENU,
	Win32VirtualKey_rsuper    = VK_RWIN,
	Win32VirtualKey_menu      = VK_APPS,
	Win32VirtualKey_rctrl     = VK_RCONTROL
};

VirtualKey wparam_to_virtual(WPARAM code)
{
	switch (code) {
	case Win32VirtualKey_escape:    return VirtualKey_escape;
	case Win32VirtualKey_F1:        return VirtualKey_F1;
	case Win32VirtualKey_F2:        return VirtualKey_F2;
	case Win32VirtualKey_F3:        return VirtualKey_F3;
	case Win32VirtualKey_F4:        return VirtualKey_F4;
	case Win32VirtualKey_F5:        return VirtualKey_F5;
	case Win32VirtualKey_F6:        return VirtualKey_F6;
	case Win32VirtualKey_F7:        return VirtualKey_F7;
	case Win32VirtualKey_F8:        return VirtualKey_F8;
	case Win32VirtualKey_F9:        return VirtualKey_F9;
	case Win32VirtualKey_F10:       return VirtualKey_F10;
	case Win32VirtualKey_F11:       return VirtualKey_F11;
	case Win32VirtualKey_F12:       return VirtualKey_F12;
	case Win32VirtualKey_tilde:     return VirtualKey_tilde;
	case Win32VirtualKey_1:         return VirtualKey_1;
	case Win32VirtualKey_2:         return VirtualKey_2;
	case Win32VirtualKey_3:         return VirtualKey_3;
	case Win32VirtualKey_4:         return VirtualKey_4;
	case Win32VirtualKey_5:         return VirtualKey_5;
	case Win32VirtualKey_6:         return VirtualKey_6;
	case Win32VirtualKey_7:         return VirtualKey_7;
	case Win32VirtualKey_8:         return VirtualKey_8;
	case Win32VirtualKey_9:         return VirtualKey_9;
	case Win32VirtualKey_0:         return VirtualKey_0;
	case Win32VirtualKey_hyphen:    return VirtualKey_hyphen;
	case Win32VirtualKey_plus:      return VirtualKey_plus;
	case Win32VirtualKey_backspace: return VirtualKey_backspace;
	case Win32VirtualKey_tab:       return VirtualKey_tab;
	case Win32VirtualKey_Q:         return VirtualKey_Q;
	case Win32VirtualKey_W:         return VirtualKey_W;
	case Win32VirtualKey_E:         return VirtualKey_E;
	case Win32VirtualKey_R:         return VirtualKey_R;
	case Win32VirtualKey_T:         return VirtualKey_T;
	case Win32VirtualKey_Y:         return VirtualKey_Y;
	case Win32VirtualKey_U:         return VirtualKey_U;
	case Win32VirtualKey_I:         return VirtualKey_I;
	case Win32VirtualKey_O:         return VirtualKey_O;
	case Win32VirtualKey_P:         return VirtualKey_P;
	case Win32VirtualKey_lbracket:  return VirtualKey_lbracket;
	case Win32VirtualKey_rbracket:  return VirtualKey_rbracket;
	case Win32VirtualKey_bslash:    return VirtualKey_bslash;
	case Win32VirtualKey_A:         return VirtualKey_A;
	case Win32VirtualKey_S:         return VirtualKey_S;
	case Win32VirtualKey_D:         return VirtualKey_D;
	case Win32VirtualKey_F:         return VirtualKey_F;
	case Win32VirtualKey_G:         return VirtualKey_G;
	case Win32VirtualKey_H:         return VirtualKey_H;
	case Win32VirtualKey_J:         return VirtualKey_J;
	case Win32VirtualKey_K:         return VirtualKey_K;
	case Win32VirtualKey_L:         return VirtualKey_L;
	case Win32VirtualKey_scolon:    return VirtualKey_scolon;
	case Win32VirtualKey_quote:     return VirtualKey_quote;
	case Win32VirtualKey_enter:     return VirtualKey_enter;
	case Win32VirtualKey_lshift:    return VirtualKey_lshift;
	case Win32VirtualKey_Z:         return VirtualKey_Z;
	case Win32VirtualKey_X:         return VirtualKey_X;
	case Win32VirtualKey_C:         return VirtualKey_C;
	case Win32VirtualKey_V:         return VirtualKey_V;
	case Win32VirtualKey_B:         return VirtualKey_B;
	case Win32VirtualKey_N:         return VirtualKey_N;
	case Win32VirtualKey_M:         return VirtualKey_M;
	case Win32VirtualKey_comma:     return VirtualKey_comma;
	case Win32VirtualKey_period:    return VirtualKey_period;
	case Win32VirtualKey_fslash:    return VirtualKey_fslash;
	case Win32VirtualKey_rshift:    return VirtualKey_rshift;
	case Win32VirtualKey_lctrl:     return VirtualKey_lctrl;
	case Win32VirtualKey_lalt:      return VirtualKey_lalt;
	case Win32VirtualKey_space:     return VirtualKey_space;
	case Win32VirtualKey_ralt:      return VirtualKey_ralt;
	case Win32VirtualKey_rsuper:    return VirtualKey_rsuper;
	case Win32VirtualKey_menu:      return VirtualKey_menu;
	case Win32VirtualKey_rctrl:     return VirtualKey_rctrl;
	};
	return VirtualKey_unknown;
}
