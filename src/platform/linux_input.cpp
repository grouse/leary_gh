/**
 * file:    linux_input.cpp
 * created: 2017-02-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

enum LinuxKey {
    LinuxKey_escape    = 9,
    LinuxKey_F1        = 67,
    LinuxKey_F2        = 68,
    LinuxKey_F3        = 69,
    LinuxKey_F4        = 70,
    LinuxKey_F5        = 71,
    LinuxKey_F6        = 72,
    LinuxKey_F7        = 73,
    LinuxKey_F8        = 74,
    LinuxKey_F9        = 75,
    LinuxKey_F10       = 76,
    LinuxKey_F11       = 95,
    LinuxKey_F12       = 96,
    LinuxKey_tilde     = 49,
    LinuxKey_1         = 10,
    LinuxKey_2         = 11,
    LinuxKey_3         = 12,
    LinuxKey_4         = 13,
    LinuxKey_5         = 14,
    LinuxKey_6         = 15,
    LinuxKey_7         = 16,
    LinuxKey_8         = 17,
    LinuxKey_9         = 18,
    LinuxKey_0         = 19,
    LinuxKey_hyphen    = 20,
    LinuxKey_plus      = 21,
    LinuxKey_backspace = 22,
    LinuxKey_tab       = 23,
    LinuxKey_Q         = 24,
    LinuxKey_W         = 25,
    LinuxKey_E         = 26,
    LinuxKey_R         = 27,
    LinuxKey_T         = 28,
    LinuxKey_Y         = 29,
    LinuxKey_U         = 30,
    LinuxKey_I         = 31,
    LinuxKey_O         = 32,
    LinuxKey_P         = 33,
    LinuxKey_lbracket  = 34,
    LinuxKey_rbracket  = 35,
    LinuxKey_bslash    = 51,
    LinuxKey_A         = 38,
    LinuxKey_S         = 39,
    LinuxKey_D         = 40,
    LinuxKey_F         = 41,
    LinuxKey_G         = 42,
    LinuxKey_H         = 43,
    LinuxKey_J         = 44,
    LinuxKey_K         = 45,
    LinuxKey_L         = 46,
    LinuxKey_scolon    = 47,
    LinuxKey_quote     = 48,
    LinuxKey_enter     = 36,
    LinuxKey_lshift    = 50,
    LinuxKey_Z         = 52,
    LinuxKey_X         = 53,
    LinuxKey_C         = 54,
    LinuxKey_V         = 55,
    LinuxKey_B         = 56,
    LinuxKey_N         = 57,
    LinuxKey_M         = 58,
    LinuxKey_comma     = 59,
    LinuxKey_period    = 60,
    LinuxKey_fslash    = 61,
    LinuxKey_rshift    = 62,
    LinuxKey_lctrl     = 37,
    LinuxKey_lalt      = 64,
    LinuxKey_space     = 65,
    LinuxKey_ralt      = 108,
    LinuxKey_rsuper    = 134,
    LinuxKey_menu      = 135,
    LinuxKey_rctrl     = 105
};

Key linux_keycode(u32 code)
{
    switch (code) {
    case LinuxKey_escape:    return Key_escape;
    case LinuxKey_F1:        return Key_F1;
    case LinuxKey_F2:        return Key_F2;
    case LinuxKey_F3:        return Key_F3;
    case LinuxKey_F4:        return Key_F4;
    case LinuxKey_F5:        return Key_F5;
    case LinuxKey_F6:        return Key_F6;
    case LinuxKey_F7:        return Key_F7;
    case LinuxKey_F8:        return Key_F8;
    case LinuxKey_F9:        return Key_F9;
    case LinuxKey_F10:       return Key_F10;
    case LinuxKey_F11:       return Key_F11;
    case LinuxKey_F12:       return Key_F12;
    case LinuxKey_tilde:     return Key_tilde;
    case LinuxKey_1:         return Key_1;
    case LinuxKey_2:         return Key_2;
    case LinuxKey_3:         return Key_3;
    case LinuxKey_4:         return Key_4;
    case LinuxKey_5:         return Key_5;
    case LinuxKey_6:         return Key_6;
    case LinuxKey_7:         return Key_7;
    case LinuxKey_8:         return Key_8;
    case LinuxKey_9:         return Key_9;
    case LinuxKey_0:         return Key_0;
    case LinuxKey_hyphen:    return Key_hyphen;
    case LinuxKey_plus:      return Key_plus;
    case LinuxKey_backspace: return Key_backspace;
    case LinuxKey_tab:       return Key_tab;
    case LinuxKey_Q:         return Key_Q;
    case LinuxKey_W:         return Key_W;
    case LinuxKey_E:         return Key_E;
    case LinuxKey_R:         return Key_R;
    case LinuxKey_T:         return Key_T;
    case LinuxKey_Y:         return Key_Y;
    case LinuxKey_U:         return Key_U;
    case LinuxKey_I:         return Key_I;
    case LinuxKey_O:         return Key_O;
    case LinuxKey_P:         return Key_P;
    case LinuxKey_lbracket:  return Key_lbracket;
    case LinuxKey_rbracket:  return Key_rbracket;
    case LinuxKey_bslash:    return Key_bslash;
    case LinuxKey_A:         return Key_A;
    case LinuxKey_S:         return Key_S;
    case LinuxKey_D:         return Key_D;
    case LinuxKey_F:         return Key_F;
    case LinuxKey_G:         return Key_G;
    case LinuxKey_H:         return Key_H;
    case LinuxKey_J:         return Key_J;
    case LinuxKey_K:         return Key_K;
    case LinuxKey_L:         return Key_L;
    case LinuxKey_scolon:    return Key_scolon;
    case LinuxKey_quote:     return Key_quote;
    case LinuxKey_enter:     return Key_enter;
    case LinuxKey_lshift:    return Key_lshift;
    case LinuxKey_Z:         return Key_Z;
    case LinuxKey_X:         return Key_X;
    case LinuxKey_C:         return Key_C;
    case LinuxKey_V:         return Key_V;
    case LinuxKey_B:         return Key_B;
    case LinuxKey_N:         return Key_N;
    case LinuxKey_M:         return Key_M;
    case LinuxKey_comma:     return Key_comma;
    case LinuxKey_period:    return Key_period;
    case LinuxKey_fslash:    return Key_fslash;
    case LinuxKey_rshift:    return Key_rshift;
    case LinuxKey_lctrl:     return Key_lctrl;
    case LinuxKey_lalt:      return Key_lalt;
    case LinuxKey_space:     return Key_space;
    case LinuxKey_ralt:      return Key_ralt;
    case LinuxKey_rsuper:    return Key_rsuper;
    case LinuxKey_menu:      return Key_menu;
    case LinuxKey_rctrl:     return Key_rctrl;
    case 113: return Key_left;
    case 111: return Key_up;
    case 114: return Key_right;
    case 116: return Key_down;
    default:
        LOG(Log_warning, "unhandled Linux keycode: %d", code);
        break;
    };
    return Key_unknown;
}
