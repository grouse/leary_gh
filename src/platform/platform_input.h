/**
 * file:    platform_input.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

enum Key {
    Key_escape,
    Key_F1,
    Key_F2,
    Key_F3,
    Key_F4,
    Key_F5,
    Key_F6,
    Key_F7,
    Key_F8,
    Key_F9,
    Key_F10,
    Key_F11,
    Key_F12,
    Key_tilde,
    Key_1,
    Key_2,
    Key_3,
    Key_4,
    Key_5,
    Key_6,
    Key_7,
    Key_8,
    Key_9,
    Key_0,
    Key_hyphen,
    Key_plus,
    Key_backspace,
    Key_tab,
    Key_Q,
    Key_W,
    Key_E,
    Key_R,
    Key_T,
    Key_Y,
    Key_U,
    Key_I,
    Key_O,
    Key_P,
    Key_lbracket,
    Key_rbracket,
    Key_bslash,
    Key_A,
    Key_S,
    Key_D,
    Key_F,
    Key_G,
    Key_H,
    Key_J,
    Key_K,
    Key_L,
    Key_scolon,
    Key_quote,
    Key_enter,
    Key_lshift,
    Key_Z,
    Key_X,
    Key_C,
    Key_V,
    Key_B,
    Key_N,
    Key_M,
    Key_comma,
    Key_period,
    Key_fslash,
    Key_rshift,
    Key_lctrl,
    Key_lalt,
    Key_space,
    Key_ralt,
    Key_rsuper,
    Key_menu,
    Key_rctrl,

    Key_left,
    Key_up,
    Key_right,
    Key_down,

    Key_unknown
};

enum InputType {
    InputType_key_release,
    InputType_key_press,
    InputType_mouse_wheel,
    InputType_mouse_move,
    InputType_mouse_press,
    InputType_mouse_release,
    InputType_middle_mouse_press,
    InputType_middle_mouse_release,
    InputType_right_mouse_press,
    InputType_right_mouse_release,
};

struct InputEvent {
    InputType type;
    union {
        struct {
            Key  code;
            bool repeated;
        } key;
        struct {
            f32 dx, dy;
            f32 x, y;
            i32 button;
        } mouse;
        f32 mouse_wheel;
    };
};
