/**
 * file:    gui.cpp
 * created: 2018-01-31
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

#include "gui.h"
#include "gfx_vulkan.h"

struct DebugInfo
{
    const char* file;
    u32         line;
};

struct GuiRenderItem
{
    Vector2                position;
    VulkanBuffer           vbo;
    VkDeviceSize           vbo_offset;
    i32                    vertex_count;

    PipelineID              pipeline_id;
    Array<GfxDescriptorSet> descriptors;
    PushConstants           constants;

#if LEARY_DEBUG
    DebugInfo              debug_info;
#endif
};

struct GuiWidget
{
    Vector2 size     = { 0.0f, 0.0f };
    Vector2 position = { 0.0f, 0.0f };
    bool pressed     = false;
    bool hover       = false;
};

static GuiRenderItem
gui_frame_render_item(Vector2 position, f32 width, f32 height, Vector4 color);


VulkanBuffer g_gui_vbo;
usize g_gui_vbo_offset = 0;
void* g_gui_vbo_map = nullptr;

Array<GuiRenderItem> g_gui_render_queue;
Array<InputEvent> g_gui_input_queue;


void init_gui()
{
    init_array(&g_gui_render_queue, g_frame);
    init_array(&g_gui_input_queue, g_frame);

    g_gui_vbo = create_vbo(1024*1024);
}

void destroy_gui()
{
    destroy_buffer(g_gui_vbo);
}

void gui_begin_frame()
{
    g_gui_render_queue.count = g_gui_render_queue.capacity = 0;
    g_gui_input_queue.count  = g_gui_input_queue.capacity = 0;

    g_gui_vbo_offset = 0;

    ASSERT(g_gui_vbo_map == nullptr);

    VkResult result = vkMapMemory(
        g_vulkan->handle,
        g_gui_vbo.memory,
        0, VK_WHOLE_SIZE, 0,
        &g_gui_vbo_map);

    ASSERT(result == VK_SUCCESS);
}

void gui_input(InputEvent event)
{
    array_add(&g_gui_input_queue, event);
}

void gui_render(VkCommandBuffer command)
{
    PROFILE_FUNCTION();

    if (g_gui_vbo_map != nullptr) {
        vkUnmapMemory(g_vulkan->handle, g_gui_vbo.memory);
        g_gui_vbo_map = nullptr;
    }

    for (i32 i = 0; i < g_gui_render_queue.count; i++) {
        auto &item = g_gui_render_queue[i];

        ASSERT(item.pipeline_id < Pipeline_count);
        VulkanPipeline &pipeline = g_vulkan->pipelines[item.pipeline_id];

        vkCmdBindPipeline(
            command,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.handle);

        if (item.descriptors.count > 0) {
            gfx_bind_descriptors(
                command,
                pipeline.layout,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                item.descriptors);
        }

        vkCmdPushConstants(
            command,
            pipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            item.constants.offset,
            (u32)item.constants.size,
            item.constants.data);

        vkCmdBindVertexBuffers(command, 0, 1, &item.vbo.handle, &item.vbo_offset);
        vkCmdDraw(command, item.vertex_count, 1, 0, 0);
    }
}

GuiWidget gui_textbox(StringView text, Vector2 *pos)
{
    PROFILE_FUNCTION();

    GuiWidget widget = {};
    widget.position = { F32_MAX, F32_MAX };

    i32 vertex_count = 0;

    usize vertices_size = sizeof(f32) * 24 * text.size;
    auto vertices = (f32*)alloc(g_frame, vertices_size);
    f32 *v        = &vertices[0];

    f32 bx = pos->x;

    for (i32 i = 0; i < text.size; i++) {
        char c = text[i];

        vertex_count += 6;
        stbtt_aligned_quad q = {};

        {
            PROFILE_SCOPE(gui_textbox_GetBakedQuad);
            stbtt_GetBakedQuad(g_font.atlas, 1024, 1024, c, &pos->x, &pos->y, &q, 1);
        }

        {
            PROFILE_SCOPE(gui_textbox_widget_sizing);
            widget.position.x = min(widget.position.x, q.x0);
            widget.position.y = min(widget.position.y, q.y0);

            widget.size.x = max(widget.size.x, q.x1 - widget.position.x);
            widget.size.y = max(widget.size.y, q.y1 - widget.position.y);
        }

        Vector2 tl, tr, br, bl;
        tl = Vector2{q.x0, q.y0};
        tr = Vector2{q.x1, q.y0};
        br = Vector2{q.x1, q.y1};
        bl = Vector2{q.x0, q.y1};

        *v++ = tl.x;
        *v++ = tl.y;
        *v++ = q.s0;
        *v++ = q.t0;

        *v++ = tr.x;
        *v++ = tr.y;
        *v++ = q.s1;
        *v++ = q.t0;

        *v++ = br.x;
        *v++ = br.y;
        *v++ = q.s1;
        *v++ = q.t1;

        *v++ = br.x;
        *v++ = br.y;
        *v++ = q.s1;
        *v++ = q.t1;

        *v++ = bl.x;
        *v++ = bl.y;
        *v++ = q.s0;
        *v++ = q.t1;

        *v++ = tl.x;
        *v++ = tl.y;
        *v++ = q.s0;
        *v++ = q.t0;
    }

    pos->x = bx;
    pos->y += 20.0f;

    GuiRenderItem item = {};
    item.pipeline_id = Pipeline_font;

#if LEARY_DEBUG
    item.debug_info.file = __FILE__;
    item.debug_info.line = __LINE__;
#endif // LEARY_DEBUG

    init_array(&item.descriptors, g_frame);
    array_add(&item.descriptors, g_game->materials.font.descriptor_set);

    item.vbo          = g_gui_vbo;
    item.vbo_offset   = g_gui_vbo_offset;
    item.vertex_count = vertex_count;

    Matrix4 t = Matrix4::identity();

    t[0][0] = 2.0f / g_vulkan->resolution.x;
    t[1][1] = 2.0f / g_vulkan->resolution.y;

    t[3][0] = -1.0f;
    t[3][1] = -1.0f;

    item.constants.offset = 0;
    item.constants.size   = sizeof t;
    item.constants.data   = alloc(g_frame, item.constants.size);
    memcpy(item.constants.data, &t, sizeof t);

    ASSERT(g_gui_vbo_offset + vertices_size < g_gui_vbo.size);
    memcpy((void*)((uptr)g_gui_vbo_map + g_gui_vbo_offset), vertices, vertices_size);
    g_gui_vbo_offset += vertices_size;

    array_add(&g_gui_render_queue, item);

    return widget;
}

GuiWidget gui_textbox(GuiFrame *frame, StringView text, Vector2 *pos)
{
    GuiWidget w = gui_textbox(text, pos);

    frame->position.x = min(frame->position.x, w.position.x);
    frame->position.y = min(frame->position.y, w.position.y);
    frame->width      = max(frame->width,      w.position.x + w.size.x);
    frame->height     = max(frame->height,     w.position.y + w.size.y);

    return w;
}

bool is_pressed(GuiWidget widget)
{
    PROFILE_FUNCTION();

    Vector2 tl = widget.position;
    Vector2 br = widget.position + widget.size;

    for (auto event : g_gui_input_queue) {
        if (event.type == InputType_mouse_press) {
            if (event.mouse.x >= tl.x && event.mouse.x <= br.x &&
                event.mouse.y >= tl.y && event.mouse.y <= br.y)
            {
                return true;
            }
        }
    }

    return false;
}

GuiFrame gui_frame_begin(Vector4 color)
{
    GuiFrame frame = {};
    frame.color        = color;
    frame.render_index = array_add(&g_gui_render_queue, {});
    return frame;
}

void gui_frame_end(GuiFrame frame)
{
    auto item = gui_frame_render_item(
        frame.position,
        frame.width, frame.height,
        frame.color);
    g_gui_render_queue[frame.render_index] = item;
}

void gui_frame(Vector2 position, f32 width, f32 height, Vector4 color)
{
    auto item = gui_frame_render_item(position, width, height, color);
    array_add(&g_gui_render_queue, item);
}



static GuiRenderItem
gui_frame_render_item(Vector2 position, f32 width, f32 height, Vector4 color)
{
    PROFILE_FUNCTION();

    struct Vertex {
        Vector2 position;
        Vector4 color;
    };

    ASSERT(g_gui_vbo_offset < g_gui_vbo.size);

    GuiRenderItem item = {};
    item.pipeline_id = Pipeline_gui_basic;

#if LEARY_DEBUG
    item.debug_info.file = __FILE__;
    item.debug_info.line = __LINE__;
#endif // LEARY_DEBUG

    // TODO(jesper): fully understand why i need to multiple this by 2
    width  = width / g_vulkan->resolution.x * 2.0f;
    height = height / g_vulkan->resolution.y * 2.0f;

    Vertex tl, tr, br, bl;
    tl.position = Vector2{ 0.0f,  0.0f };
    tr.position = Vector2{ width, 0.0f };
    br.position = Vector2{ width, height };
    bl.position = Vector2{ 0.0f,  height };

    tl.color = tr.color = br.color = bl.color = color;

    Array<Vertex> vertices;
    init_array(&vertices, g_frame, 6);
    array_add(&vertices, tl);
    array_add(&vertices, tr);
    array_add(&vertices, br);

    array_add(&vertices, br);
    array_add(&vertices, bl);
    array_add(&vertices, tl);

    item.vbo          = g_gui_vbo;
    item.vbo_offset   = g_gui_vbo_offset;
    item.vertex_count = 6;

    Matrix4 t = translate(Matrix4::identity(), camera_from_screen(position));
    item.constants.offset = 0;
    item.constants.size   = sizeof t;
    item.constants.data   = alloc(g_frame, item.constants.size);
    memcpy(item.constants.data, &t, sizeof t);

    isize vertices_size = vertices.count * sizeof vertices[0];

    ASSERT(g_gui_vbo_offset + vertices_size < g_gui_vbo.size);
    memcpy((void*)((uptr)g_gui_vbo_map + g_gui_vbo_offset),
           vertices.data,
           vertices_size);

    g_gui_vbo_offset += vertices_size;
    return item;
}
