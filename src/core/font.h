/**
 * file:    font.h
 * created: 2018-01-31
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#ifndef LEARY_FONT_H
#define LEARY_FONT_H

struct FontData
{
    VulkanBuffer vbo;
    i32 vertex_count = 0;

    stbtt_bakedchar atlas[256];

    void *buffer = nullptr;
    usize offset = 0;
};

#endif // LEARY_FONT_H

