/**
 * file:    linux_input.cpp
 * created: 2017-02-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "platform_input.h"

enum LinuxVirtualKey {
	LinuxVirtualKey_escape    = 9,
	LinuxVirtualKey_F1        = 67,
	LinuxVirtualKey_F2        = 68,
	LinuxVirtualKey_F3        = 69,
	LinuxVirtualKey_F4        = 70,
	LinuxVirtualKey_F5        = 71,
	LinuxVirtualKey_F6        = 72,
	LinuxVirtualKey_F7        = 73,
	LinuxVirtualKey_F8        = 74,
	LinuxVirtualKey_F9        = 75,
	LinuxVirtualKey_F10       = 76,
	LinuxVirtualKey_F11       = 95,
	LinuxVirtualKey_F12       = 96,
	LinuxVirtualKey_tilde     = 49,
	LinuxVirtualKey_1         = 10,
	LinuxVirtualKey_2         = 11,
	LinuxVirtualKey_3         = 12,
	LinuxVirtualKey_4         = 13,
	LinuxVirtualKey_5         = 14,
	LinuxVirtualKey_6         = 15,
	LinuxVirtualKey_7         = 16,
	LinuxVirtualKey_8         = 17,
	LinuxVirtualKey_9         = 18,
	LinuxVirtualKey_0         = 19,
	LinuxVirtualKey_hyphen    = 20,
	LinuxVirtualKey_plus      = 21,
	LinuxVirtualKey_backspace = 22,
	LinuxVirtualKey_tab       = 23,
	LinuxVirtualKey_Q         = 24,
	LinuxVirtualKey_W         = 25,
	LinuxVirtualKey_E         = 26,
	LinuxVirtualKey_R         = 27,
	LinuxVirtualKey_T         = 28,
	LinuxVirtualKey_Y         = 29,
	LinuxVirtualKey_U         = 30,
	LinuxVirtualKey_I         = 31,
	LinuxVirtualKey_O         = 32,
	LinuxVirtualKey_P         = 33,
	LinuxVirtualKey_lbracket  = 34,
	LinuxVirtualKey_rbracket  = 35,
	LinuxVirtualKey_bslash    = 51,
	LinuxVirtualKey_A         = 38,
	LinuxVirtualKey_S         = 39,
	LinuxVirtualKey_D         = 40,
	LinuxVirtualKey_F         = 41,
	LinuxVirtualKey_G         = 42,
	LinuxVirtualKey_H         = 43,
	LinuxVirtualKey_J         = 44,
	LinuxVirtualKey_K         = 45,
	LinuxVirtualKey_L         = 46,
	LinuxVirtualKey_scolon    = 47,
	LinuxVirtualKey_quote     = 48,
	LinuxVirtualKey_enter     = 36,
	LinuxVirtualKey_lshift    = 50,
	LinuxVirtualKey_Z         = 52,
	LinuxVirtualKey_X         = 53,
	LinuxVirtualKey_C         = 54,
	LinuxVirtualKey_V         = 55,
	LinuxVirtualKey_B         = 56,
	LinuxVirtualKey_N         = 57,
	LinuxVirtualKey_M         = 58,
	LinuxVirtualKey_comma     = 59,
	LinuxVirtualKey_period    = 60,
	LinuxVirtualKey_fslash    = 61,
	LinuxVirtualKey_rshift    = 62,
	LinuxVirtualKey_lctrl     = 37,
	LinuxVirtualKey_lalt      = 64,
	LinuxVirtualKey_space     = 65,
	LinuxVirtualKey_ralt      = 108,
	LinuxVirtualKey_rsuper    = 134,
	LinuxVirtualKey_menu      = 135,
	LinuxVirtualKey_rctrl     = 105
};

VirtualKey keycode_to_virtual(u32 code)
{
	switch (code) {
	case LinuxVirtualKey_escape:    return VirtualKey_escape;
	case LinuxVirtualKey_F1:        return VirtualKey_F1;
	case LinuxVirtualKey_F2:        return VirtualKey_F2;
	case LinuxVirtualKey_F3:        return VirtualKey_F3;
	case LinuxVirtualKey_F4:        return VirtualKey_F4;
	case LinuxVirtualKey_F5:        return VirtualKey_F5;
	case LinuxVirtualKey_F6:        return VirtualKey_F6;
	case LinuxVirtualKey_F7:        return VirtualKey_F7;
	case LinuxVirtualKey_F8:        return VirtualKey_F8;
	case LinuxVirtualKey_F9:        return VirtualKey_F9;
	case LinuxVirtualKey_F10:       return VirtualKey_F10;
	case LinuxVirtualKey_F11:       return VirtualKey_F11;
	case LinuxVirtualKey_F12:       return VirtualKey_F12;
	case LinuxVirtualKey_tilde:     return VirtualKey_tilde;
	case LinuxVirtualKey_1:         return VirtualKey_1;
	case LinuxVirtualKey_2:         return VirtualKey_2;
	case LinuxVirtualKey_3:         return VirtualKey_3;
	case LinuxVirtualKey_4:         return VirtualKey_4;
	case LinuxVirtualKey_5:         return VirtualKey_5;
	case LinuxVirtualKey_6:         return VirtualKey_6;
	case LinuxVirtualKey_7:         return VirtualKey_7;
	case LinuxVirtualKey_8:         return VirtualKey_8;
	case LinuxVirtualKey_9:         return VirtualKey_9;
	case LinuxVirtualKey_0:         return VirtualKey_0;
	case LinuxVirtualKey_hyphen:    return VirtualKey_hyphen;
	case LinuxVirtualKey_plus:      return VirtualKey_plus;
	case LinuxVirtualKey_backspace: return VirtualKey_backspace;
	case LinuxVirtualKey_tab:       return VirtualKey_tab;
	case LinuxVirtualKey_Q:         return VirtualKey_Q;
	case LinuxVirtualKey_W:         return VirtualKey_W;
	case LinuxVirtualKey_E:         return VirtualKey_E;
	case LinuxVirtualKey_R:         return VirtualKey_R;
	case LinuxVirtualKey_T:         return VirtualKey_T;
	case LinuxVirtualKey_Y:         return VirtualKey_Y;
	case LinuxVirtualKey_U:         return VirtualKey_U;
	case LinuxVirtualKey_I:         return VirtualKey_I;
	case LinuxVirtualKey_O:         return VirtualKey_O;
	case LinuxVirtualKey_P:         return VirtualKey_P;
	case LinuxVirtualKey_lbracket:  return VirtualKey_lbracket;
	case LinuxVirtualKey_rbracket:  return VirtualKey_rbracket;
	case LinuxVirtualKey_bslash:    return VirtualKey_bslash;
	case LinuxVirtualKey_A:         return VirtualKey_A;
	case LinuxVirtualKey_S:         return VirtualKey_S;
	case LinuxVirtualKey_D:         return VirtualKey_D;
	case LinuxVirtualKey_F:         return VirtualKey_F;
	case LinuxVirtualKey_G:         return VirtualKey_G;
	case LinuxVirtualKey_H:         return VirtualKey_H;
	case LinuxVirtualKey_J:         return VirtualKey_J;
	case LinuxVirtualKey_K:         return VirtualKey_K;
	case LinuxVirtualKey_L:         return VirtualKey_L;
	case LinuxVirtualKey_scolon:    return VirtualKey_scolon;
	case LinuxVirtualKey_quote:     return VirtualKey_quote;
	case LinuxVirtualKey_enter:     return VirtualKey_enter;
	case LinuxVirtualKey_lshift:    return VirtualKey_lshift;
	case LinuxVirtualKey_Z:         return VirtualKey_Z;
	case LinuxVirtualKey_X:         return VirtualKey_X;
	case LinuxVirtualKey_C:         return VirtualKey_C;
	case LinuxVirtualKey_V:         return VirtualKey_V;
	case LinuxVirtualKey_B:         return VirtualKey_B;
	case LinuxVirtualKey_N:         return VirtualKey_N;
	case LinuxVirtualKey_M:         return VirtualKey_M;
	case LinuxVirtualKey_comma:     return VirtualKey_comma;
	case LinuxVirtualKey_period:    return VirtualKey_period;
	case LinuxVirtualKey_fslash:    return VirtualKey_fslash;
	case LinuxVirtualKey_rshift:    return VirtualKey_rshift;
	case LinuxVirtualKey_lctrl:     return VirtualKey_lctrl;
	case LinuxVirtualKey_lalt:      return VirtualKey_lalt;
	case LinuxVirtualKey_space:     return VirtualKey_space;
	case LinuxVirtualKey_ralt:      return VirtualKey_ralt;
	case LinuxVirtualKey_rsuper:    return VirtualKey_rsuper;
	case LinuxVirtualKey_menu:      return VirtualKey_menu;
	case LinuxVirtualKey_rctrl:     return VirtualKey_rctrl;
	case 113: return VirtualKey_left;
	case 111: return VirtualKey_up;
	case 114: return VirtualKey_right;
	case 116: return VirtualKey_down;
	default:
	    DEBUG_LOG(Log_warning, "unhandled Linux keycode: %d", code);
	    break;
	};
	return VirtualKey_unknown;
}
