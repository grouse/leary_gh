/**
 * file:    font.cpp
 * created: 2017-11-11
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "vulkan_render.h"

struct FontData
{
    VulkanBuffer vbo;
    i32 vertex_count = 0;

    stbtt_bakedchar atlas[256];

    void *buffer = nullptr;
    usize offset = 0;
};

FontData g_font = {};
extern VulkanDevice *g_vulkan;

void init_fonts()
{
    usize font_size;
    char *font_path = resolve_path(GamePath_data, "fonts/Roboto-Regular.ttf", g_stack);
    if (font_path == nullptr) {
        return;
    }

    u8 *font_data = (u8*)read_file(font_path, &font_size, g_frame);

    u8 *bitmap = g_frame->alloc_array<u8>(1024*1024);
    stbtt_BakeFontBitmap(font_data, 0, 20.0f, bitmap,
                         1024, 1024, 0, 256, g_font.atlas);

    VkComponentMapping components = {};
    components.a = VK_COMPONENT_SWIZZLE_R;
    add_texture("font-regular", 1024, 1024, VK_FORMAT_R8_UNORM, bitmap, components);

    // TODO(jesper): this size is really wrong
    g_font.vbo = create_vbo( 1024 * 1024 );
}

void destroy_fonts()
{
    destroy_buffer(g_font.vbo);
}


void gui_frame_start()
{
    ASSERT(g_font.buffer == nullptr);

    VkResult result = vkMapMemory(
        g_vulkan->handle,
        g_font.vbo.memory,
        0, VK_WHOLE_SIZE, 0,
        &g_font.buffer);

    ASSERT(result == VK_SUCCESS);
}

void gui_render_text(VkCommandBuffer command)
{
    if (g_font.buffer != nullptr) {
        vkUnmapMemory(g_vulkan->handle, g_font.vbo.memory);
        g_font.buffer = nullptr;
    }

    VkDeviceSize offsets[] = { 0 };

    if (g_font.vertex_count == 0) {
        return;
    }

    VulkanPipeline &pipeline = g_vulkan->pipelines[Pipeline_font];

    vkCmdBindPipeline(
        command,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.handle);

    auto descriptors = create_array<VkDescriptorSet>(g_stack);
    array_add(&descriptors, g_game->materials.font.descriptor_set);

    vkCmdBindDescriptorSets(
        command,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline.layout,
        0,
        (i32)descriptors.count, descriptors.data,
        0, nullptr);

    Matrix4 t = Matrix4::identity();
    vkCmdPushConstants(
        command,
        pipeline.layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0, sizeof(t), &t);

    vkCmdBindVertexBuffers(command, 0, 1, &g_font.vbo.handle, offsets);
    vkCmdDraw(command, g_font.vertex_count, 1, 0, 0);

    g_font.vertex_count = 0;
    g_font.offset = 0;
}

void gui_textbox(StringView text, Vector2 *pos)
{
    i32 vertex_count = 0;

    usize vertices_size = sizeof(f32) * 24 * text.size;
    auto vertices = (f32*)g_frame->alloc(vertices_size);

    f32 bx = pos->x;

    i32 vi = 0;

    for (i32 i = 0; i < text.size; i++) {
        char c = text[i];

        if (c == '\n') {
            pos->y += 20.0f;
            pos->x  = bx;
            vertices_size -= sizeof(f32)*4*6;
            continue;
        }

        vertex_count += 6;

        stbtt_aligned_quad q = {};
        stbtt_GetBakedQuad(g_font.atlas, 1024, 1024, c, &pos->x, &pos->y, &q, 1);

        Vector2 tl = camera_from_screen(Vector2{q.x0, q.y0 + 15.0f});
        Vector2 tr = camera_from_screen(Vector2{q.x1, q.y0 + 15.0f});
        Vector2 br = camera_from_screen(Vector2{q.x1, q.y1 + 15.0f});
        Vector2 bl = camera_from_screen(Vector2{q.x0, q.y1 + 15.0f});

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

    g_font.vertex_count += vertex_count;

    memcpy((void*)((uptr)g_font.buffer + g_font.offset), vertices, vertices_size);
    g_font.offset += vertices_size;
}

