/**
 * file:    font.cpp
 * created: 2017-11-11
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */


void init_fonts()
{
    // create font atlas
    {
        usize font_size;
        char *font_path = resolve_path(GamePath_data, "fonts/Roboto-Regular.ttf", g_stack);
        if (font_path != nullptr) {
            u8 *font_data = (u8*)read_file(font_path, &font_size, g_frame);

            u8 *bitmap = g_frame->alloc_array<u8>(1024*1024);
            stbtt_BakeFontBitmap(font_data, 0, g_game->overlay.fsize, bitmap,
                                 1024, 1024, 0, 256, g_game->overlay.font);

            VkComponentMapping components = {};
            components.a = VK_COMPONENT_SWIZZLE_R;
            add_texture("font-regular", 1024, 1024, VK_FORMAT_R8_UNORM, bitmap,
                        components);

            // TODO(jesper): this size is really wrong
            g_game->overlay.vbo = create_vbo(1024*1024);
        }
    }
}

