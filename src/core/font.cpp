/**
 * file:    font.cpp
 * created: 2017-11-11
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

extern Matrix4 g_screen_to_view;

void init_fonts()
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

void render_font(stbtt_bakedchar *font,
                 const char *str,
                 Vector2 *pos,
                 i32 *out_vertex_count,
                 void *buffer, usize *offset)
{
    i32 vertex_count = 0;

    usize text_length = strlen(str);
    if (text_length == 0) return;

    usize vertices_size = sizeof(f32)*24*text_length;
    auto vertices = (f32*)g_frame->alloc(vertices_size);

    Matrix4 t = g_screen_to_view;

    f32 bx = pos->x;

    i32 vi = 0;
    while (*str) {
        char c = *str++;
        if (c == '\n') {
            pos->y += 20.0f;
            pos->x  = bx;
            vertices_size -= sizeof(f32)*4*6;
            continue;
        }

        vertex_count += 6;

        stbtt_aligned_quad q = {};
        stbtt_GetBakedQuad(font, 1024, 1024, c, &pos->x, &pos->y, &q, 1);

        Vector2 tl = t * Vector2{q.x0, q.y0 + 15.0f};
        Vector2 tr = t * Vector2{q.x1, q.y0 + 15.0f};
        Vector2 br = t * Vector2{q.x1, q.y1 + 15.0f};
        Vector2 bl = t * Vector2{q.x0, q.y1 + 15.0f};

        vertices[vi++] = tl.x;
        vertices[vi++] = tl.y;
        vertices[vi++] = q.s0;
        vertices[vi++] = q.t0;

        vertices[vi++] = tr.x;
        vertices[vi++] = tr.y;
        vertices[vi++] = q.s1;
        vertices[vi++] = q.t0;

        vertices[vi++] = br.x;
        vertices[vi++] = br.y;
        vertices[vi++] = q.s1;
        vertices[vi++] = q.t1;

        vertices[vi++] = br.x;
        vertices[vi++] = br.y;
        vertices[vi++] = q.s1;
        vertices[vi++] = q.t1;

        vertices[vi++] = bl.x;
        vertices[vi++] = bl.y;
        vertices[vi++] = q.s0;
        vertices[vi++] = q.t1;

        vertices[vi++] = tl.x;
        vertices[vi++] = tl.y;
        vertices[vi++] = q.s0;
        vertices[vi++] = q.t0;
    }

    *out_vertex_count += vertex_count;

    memcpy((void*)((uptr)buffer + *offset), vertices, vertices_size);
    *offset += vertices_size;
}

