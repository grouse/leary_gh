/**
 * file:    win32_input.cpp
 * created: 2017-02-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

Key win32_keycode(WPARAM code)
{
    switch (code) {
    case   VK_ESCAPE:     return Key_escape;
    case   VK_F1:         return Key_F1;
    case   VK_F2:         return Key_F2;
    case   VK_F3:         return Key_F3;
    case   VK_F4:         return Key_F4;
    case   VK_F5:         return Key_F5;
    case   VK_F6:         return Key_F6;
    case   VK_F7:         return Key_F7;
    case   VK_F8:         return Key_F8;
    case   VK_F9:         return Key_F9;
    case   VK_F10:        return Key_F10;
    case   VK_F11:        return Key_F11;
    case   VK_F12:        return Key_F12;
    case   VK_OEM_3:      return Key_tilde;
    case   0x31:          return Key_1;
    case   0x32:          return Key_2;
    case   0x33:          return Key_3;
    case   0x34:          return Key_4;
    case   0x35:          return Key_5;
    case   0x36:          return Key_6;
    case   0x37:          return Key_7;
    case   0x38:          return Key_8;
    case   0x39:          return Key_9;
    case   0x30:          return Key_0;
    case   VK_OEM_MINUS:  return Key_hyphen;
    case   VK_OEM_PLUS:   return Key_plus;
    case   VK_BACK:       return Key_backspace;
    case   VK_TAB:        return Key_tab;
    case   0x51:          return Key_Q;
    case   0x57:          return Key_W;
    case   0x45:          return Key_E;
    case   0x52:          return Key_R;
    case   0x54:          return Key_T;
    case   0x59:          return Key_Y;
    case   0x55:          return Key_U;
    case   0x49:          return Key_I;
    case   0x4F:          return Key_O;
    case   0x50:          return Key_P;
    case   VK_OEM_4:      return Key_lbracket;
    case   VK_OEM_6:      return Key_rbracket;
    case   VK_OEM_5:      return Key_bslash;
    case   0x41:          return Key_A;
    case   0x53:          return Key_S;
    case   0x44:          return Key_D;
    case   0x46:          return Key_F;
    case   0x47:          return Key_G;
    case   0x48:          return Key_H;
    case   0x4A:          return Key_J;
    case   0x4B:          return Key_K;
    case   0x4C:          return Key_L;
    case   VK_OEM_1:      return Key_scolon;
    case   VK_OEM_7:      return Key_quote;
    case   VK_RETURN:     return Key_enter;
    case   VK_LSHIFT:     return Key_lshift;
    case   0x5A:          return Key_Z;
    case   0x58:          return Key_X;
    case   0x43:          return Key_C;
    case   0x56:          return Key_V;
    case   0x42:          return Key_B;
    case   0x4E:          return Key_N;
    case   0x4D:          return Key_M;
    case   VK_OEM_COMMA:  return Key_comma;
    case   VK_OEM_PERIOD: return Key_period;
    case   VK_OEM_2:      return Key_fslash;
    case   VK_RSHIFT:     return Key_rshift;
    case   VK_LCONTROL:   return Key_lctrl;
    case   VK_LMENU:      return Key_lalt;
    case   VK_SPACE:      return Key_space;
    case   VK_RMENU:      return Key_ralt;
    case   VK_RWIN:       return Key_rsuper;
    case   VK_APPS:       return Key_menu;
    case   VK_RCONTROL:   return Key_rctrl;
    case   VK_LEFT:       return Key_left;
    case   VK_RIGHT:      return Key_right;
    case   VK_UP:         return Key_up;
    case   VK_DOWN:       return Key_down;
    };
    return Key_unknown;
}
