/**
 * file:    font.cpp
 * created: 2017-11-11
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

FontData g_font = {};
extern VulkanDevice *g_vulkan;

void init_fonts()
{
    usize font_size;
    FilePath font_path = resolve_file_path(GamePath_data, "fonts/Roboto-Regular.ttf", g_stack);
    if (font_path.absolute.size == 0) {
        return;
    }

    u8 *font_data = (u8*)read_file(font_path, &font_size, g_frame);

    u8 *bitmap = alloc_array(g_frame, u8, 1024*1024);
    stbtt_BakeFontBitmap(font_data, 0, 20.0f, bitmap,
                         1024, 1024, 0, 256, g_font.atlas);

    VkComponentMapping components = {};
    components.a = VK_COMPONENT_SWIZZLE_R;
    add_texture("font-regular", 1024, 1024, VK_FORMAT_R8_UNORM, bitmap, components);
}

void destroy_fonts()
{
}

