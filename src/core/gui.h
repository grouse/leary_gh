/**
 * file:    gui.h
 * created: 2018-02-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#ifndef LEARY_GUI_H
#define LEARY_GUI_H

struct GuiFrame {
    i32 render_index;
    Vector2 position;
    f32 width;
    f32 height;
    Vector4 color;
};

#endif // LEARY_GUI_H
