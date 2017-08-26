/**
 * file:    win32_input.cpp
 * created: 2017-02-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

enum Win32Key {
    Win32Key_escape    = VK_ESCAPE,
    Win32Key_F1        = VK_F1,
    Win32Key_F2        = VK_F2,
    Win32Key_F3        = VK_F3,
    Win32Key_F4        = VK_F4,
    Win32Key_F5        = VK_F5,
    Win32Key_F6        = VK_F6,
    Win32Key_F7        = VK_F7,
    Win32Key_F8        = VK_F8,
    Win32Key_F9        = VK_F9,
    Win32Key_F10       = VK_F10,
    Win32Key_F11       = VK_F11,
    Win32Key_F12       = VK_F12,
    Win32Key_tilde     = VK_OEM_3,
    Win32Key_0         = 0x30,
    Win32Key_1         = 0x31,
    Win32Key_2         = 0x32,
    Win32Key_3         = 0x33,
    Win32Key_4         = 0x34,
    Win32Key_5         = 0x35,
    Win32Key_6         = 0x36,
    Win32Key_7         = 0x37,
    Win32Key_8         = 0x38,
    Win32Key_9         = 0x39,
    Win32Key_hyphen    = VK_OEM_MINUS,
    Win32Key_plus      = VK_OEM_PLUS,
    Win32Key_backspace = VK_BACK,
    Win32Key_tab       = VK_TAB,
    Win32Key_A         = 0x41,
    Win32Key_B         = 0x42,
    Win32Key_C         = 0x43,
    Win32Key_D         = 0x44,
    Win32Key_E         = 0x45,
    Win32Key_F         = 0x46,
    Win32Key_G         = 0x47,
    Win32Key_H         = 0x48,
    Win32Key_I         = 0x49,
    Win32Key_J         = 0x4A,
    Win32Key_K         = 0x4B,
    Win32Key_L         = 0x4C,
    Win32Key_M         = 0x4D,
    Win32Key_N         = 0x4E,
    Win32Key_O         = 0x4F,
    Win32Key_P         = 0x50,
    Win32Key_Q         = 0x51,
    Win32Key_R         = 0x52,
    Win32Key_S         = 0x53,
    Win32Key_T         = 0x54,
    Win32Key_U         = 0x55,
    Win32Key_V         = 0x56,
    Win32Key_W         = 0x57,
    Win32Key_X         = 0x58,
    Win32Key_Y         = 0x59,
    Win32Key_Z         = 0x5A,
    Win32Key_lbracket  = VK_OEM_4,
    Win32Key_rbracket  = VK_OEM_6,
    Win32Key_bslash    = VK_OEM_5,
    Win32Key_scolon    = VK_OEM_1,
    Win32Key_quote     = VK_OEM_7,
    Win32Key_enter     = VK_RETURN,
    Win32Key_lshift    = VK_LSHIFT,
    Win32Key_comma     = VK_OEM_COMMA,
    Win32Key_period    = VK_OEM_PERIOD,
    Win32Key_fslash    = VK_OEM_2,
    Win32Key_rshift    = VK_RSHIFT,
    Win32Key_lctrl     = VK_LCONTROL,
    Win32Key_lalt      = VK_LMENU,
    Win32Key_space     = VK_SPACE,
    Win32Key_ralt      = VK_RMENU,
    Win32Key_rsuper    = VK_RWIN,
    Win32Key_menu      = VK_APPS,
    Win32Key_rctrl     = VK_RCONTROL
};

Key win32_keycode(WPARAM code)
{
    switch (code) {
    case Win32Key_escape:    return Key_escape;
    case Win32Key_F1:        return Key_F1;
    case Win32Key_F2:        return Key_F2;
    case Win32Key_F3:        return Key_F3;
    case Win32Key_F4:        return Key_F4;
    case Win32Key_F5:        return Key_F5;
    case Win32Key_F6:        return Key_F6;
    case Win32Key_F7:        return Key_F7;
    case Win32Key_F8:        return Key_F8;
    case Win32Key_F9:        return Key_F9;
    case Win32Key_F10:       return Key_F10;
    case Win32Key_F11:       return Key_F11;
    case Win32Key_F12:       return Key_F12;
    case Win32Key_tilde:     return Key_tilde;
    case Win32Key_1:         return Key_1;
    case Win32Key_2:         return Key_2;
    case Win32Key_3:         return Key_3;
    case Win32Key_4:         return Key_4;
    case Win32Key_5:         return Key_5;
    case Win32Key_6:         return Key_6;
    case Win32Key_7:         return Key_7;
    case Win32Key_8:         return Key_8;
    case Win32Key_9:         return Key_9;
    case Win32Key_0:         return Key_0;
    case Win32Key_hyphen:    return Key_hyphen;
    case Win32Key_plus:      return Key_plus;
    case Win32Key_backspace: return Key_backspace;
    case Win32Key_tab:       return Key_tab;
    case Win32Key_Q:         return Key_Q;
    case Win32Key_W:         return Key_W;
    case Win32Key_E:         return Key_E;
    case Win32Key_R:         return Key_R;
    case Win32Key_T:         return Key_T;
    case Win32Key_Y:         return Key_Y;
    case Win32Key_U:         return Key_U;
    case Win32Key_I:         return Key_I;
    case Win32Key_O:         return Key_O;
    case Win32Key_P:         return Key_P;
    case Win32Key_lbracket:  return Key_lbracket;
    case Win32Key_rbracket:  return Key_rbracket;
    case Win32Key_bslash:    return Key_bslash;
    case Win32Key_A:         return Key_A;
    case Win32Key_S:         return Key_S;
    case Win32Key_D:         return Key_D;
    case Win32Key_F:         return Key_F;
    case Win32Key_G:         return Key_G;
    case Win32Key_H:         return Key_H;
    case Win32Key_J:         return Key_J;
    case Win32Key_K:         return Key_K;
    case Win32Key_L:         return Key_L;
    case Win32Key_scolon:    return Key_scolon;
    case Win32Key_quote:     return Key_quote;
    case Win32Key_enter:     return Key_enter;
    case Win32Key_lshift:    return Key_lshift;
    case Win32Key_Z:         return Key_Z;
    case Win32Key_X:         return Key_X;
    case Win32Key_C:         return Key_C;
    case Win32Key_V:         return Key_V;
    case Win32Key_B:         return Key_B;
    case Win32Key_N:         return Key_N;
    case Win32Key_M:         return Key_M;
    case Win32Key_comma:     return Key_comma;
    case Win32Key_period:    return Key_period;
    case Win32Key_fslash:    return Key_fslash;
    case Win32Key_rshift:    return Key_rshift;
    case Win32Key_lctrl:     return Key_lctrl;
    case Win32Key_lalt:      return Key_lalt;
    case Win32Key_space:     return Key_space;
    case Win32Key_ralt:      return Key_ralt;
    case Win32Key_rsuper:    return Key_rsuper;
    case Win32Key_menu:      return Key_menu;
    case Win32Key_rctrl:     return Key_rctrl;
    };
    return Key_unknown;
}
