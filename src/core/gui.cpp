/**
 * file:    gui.cpp
 * created: 2018-01-31
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

static GuiRenderItem
gui_frame_render_item(Vector2 position, f32 width, f32 height, Vector4 color);


VulkanBuffer g_gui_vbo;
usize g_gui_vbo_offset = 0;
void* g_gui_vbo_map = nullptr;

Array<GuiRenderItem> g_gui_render_queue;
Array<GuiRenderItem> g_gui_tooltip_render_queue;
Array<InputEvent> g_gui_input_queue;

void init_gui()
{
    init_array(&g_gui_render_queue, g_frame);
    init_array(&g_gui_tooltip_render_queue, g_frame);
    init_array(&g_gui_input_queue, g_frame);

    g_gui_vbo = create_vbo(1024*1024);
}

void destroy_gui()
{
    destroy_buffer(g_gui_vbo);
}

GuiTextbox gui_textbox_data(StringView text, Vector4 color, Vector2 *pos)
{
    PROFILE_FUNCTION();

    GuiTextbox tb = {};
    tb.position = { F32_MAX, F32_MAX };
    tb.vertices = (GuiTextbox::Vertex*)((uptr)g_gui_vbo_map + g_gui_vbo_offset);
    tb.vertices_cap = g_gui_vbo.size - g_gui_vbo_offset;

    f32 bx = pos->x;

    for (i32 i = 0; i < text.size; i++) {
        char c = text[i];

        stbtt_aligned_quad q = {};

        {
            PROFILE_SCOPE(gui_textbox_GetBakedQuad);
            stbtt_GetBakedQuad(g_font.atlas, 1024, 1024, c, &pos->x, &pos->y, &q, 1);
        }

        {
            PROFILE_SCOPE(gui_textbox_widget_sizing);
            tb.position.x = min(tb.position.x, q.x0);
            tb.position.y = min(tb.position.y, q.y0);

            tb.size.x = max(tb.size.x, q.x1 - tb.position.x);
            tb.size.y = max(tb.size.y, q.y1 - tb.position.y);
        }

        GuiTextbox::Vertex v = {};
        v.position = { q.x0, q.y0 };
        v.uv       = { q.s0, q.t0 };
        v.color    = color;
        tb.vertices[tb.vertices_count++] = v;

        v.position = { q.x0, q.y1 };
        v.uv       = { q.s0, q.t1 };
        v.color    = color;
        tb.vertices[tb.vertices_count++] = v;

        v.position = { q.x1, q.y1 };
        v.uv       = { q.s1, q.t1 };
        v.color    = color;
        tb.vertices[tb.vertices_count++] = v;

        v.position = { q.x1, q.y1 };
        v.uv       = { q.s1, q.t1 };
        v.color    = color;
        tb.vertices[tb.vertices_count++] = v;

        v.position = { q.x1, q.y0 };
        v.uv       = { q.s1, q.t0 };
        v.color    = color;
        tb.vertices[tb.vertices_count++] = v;

        v.position = { q.x0, q.y0 };
        v.uv       = { q.s0, q.t0 };
        v.color    = color;
        tb.vertices[tb.vertices_count++] = v;
    }

    pos->x = bx;
    pos->y += 20.0f;

    return tb;
}

GuiRenderItem gui_textbox_render_item(GuiTextbox tb)
{
    PROFILE_FUNCTION();

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
    item.vertex_count = tb.vertices_count;

    Matrix4 t = matrix4_identity();

    t[0][0] = 2.0f / g_vulkan->resolution.x;
    t[1][1] = 2.0f / g_vulkan->resolution.y;

    t[3][0] = -1.0f;
    t[3][1] = -1.0f;

    item.constants.offset = 0;
    item.constants.size   = sizeof t;
    item.constants.data   = alloc(g_frame, item.constants.size);
    memcpy(item.constants.data, &t, sizeof t);

    tb.vertices_cap = tb.vertices_count;
    g_gui_vbo_offset += tb.vertices_count * sizeof tb.vertices[0];

    return item;
}


void gui_begin_frame()
{
    init_array(&g_gui_render_queue, g_frame);
    init_array(&g_gui_tooltip_render_queue, g_frame);
    init_array(&g_gui_input_queue, g_frame);

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

void gui_render_items(VkCommandBuffer command, Array<GuiRenderItem> items)
{
    PROFILE_FUNCTION();

    for (i32 i = 0; i < items.count; i++) {
        auto &item = items[i];

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

void gui_render(VkCommandBuffer command)
{
    PROFILE_FUNCTION();

    if (g_gui_vbo_map != nullptr) {
        vkUnmapMemory(g_vulkan->handle, g_gui_vbo.memory);
        g_gui_vbo_map = nullptr;
    }

    gui_render_items(command, g_gui_render_queue);
    gui_render_items(command, g_gui_tooltip_render_queue);
}

GuiTextbox gui_textbox(StringView text, Vector4 color, Vector2 *pos)
{
    PROFILE_FUNCTION();

    GuiTextbox tb = gui_textbox_data(text, color, pos);
    GuiRenderItem item = gui_textbox_render_item(tb);

    array_add(&g_gui_render_queue, item);
    return tb;
}

GuiTextbox gui_textbox(
    GuiFrame *frame,
    StringView text,
    Vector4 color,
    Vector2 *pos)
{
    PROFILE_FUNCTION();

    GuiTextbox tb = gui_textbox(text, color, pos);

    frame->position.x = min(frame->position.x, tb.position.x);
    frame->position.y = min(frame->position.y, tb.position.y);

    frame->width = max(
        frame->width,
        tb.position.x + tb.size.x - frame->position.x);

    frame->height = max(
        frame->height,
        tb.position.y + tb.size.y - frame->position.y);

    return tb;
}

GuiTextbox gui_textbox(
    GuiFrame *frame,
    StringView text,
    Vector4 color,
    Vector4 highlight,
    Vector2 *pos)
{
    PROFILE_FUNCTION();

    GuiTextbox tb = gui_textbox(frame, text, color, pos);
    if (is_mouse_over(tb)) {
        for (i32 i = 0; i < tb.vertices_count; i++) {
            tb.vertices[i].color = highlight;
        }
    }
    return tb;
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

bool is_pressed(GuiTextbox textbox)
{
    PROFILE_FUNCTION();

    Vector2 tl = textbox.position;
    Vector2 br = textbox.position + textbox.size;

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

bool is_mouse_over(GuiTextbox textbox)
{
    PROFILE_FUNCTION();

    Vector2 tl = textbox.position;
    Vector2 br = textbox.position + textbox.size;

    Vector2 mouse = get_mouse_position();

    if (mouse.x >= tl.x && mouse.x <= br.x &&
        mouse.y >= tl.y && mouse.y <= br.y)
    {
        return true;
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

GuiFrame gui_frame_begin(Vector2 position, Vector4 color)
{
    GuiFrame frame = {};
    frame.position     = position;
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

GuiRenderItem
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
    array_add(&vertices, bl);
    array_add(&vertices, br);

    array_add(&vertices, br);
    array_add(&vertices, tr);
    array_add(&vertices, tl);

    item.vbo          = g_gui_vbo;
    item.vbo_offset   = g_gui_vbo_offset;
    item.vertex_count = 6;

    Matrix4 t = translate(matrix4_identity(), camera_from_screen(position));
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

void gui_tooltip(StringView text, Vector4 fg, Vector4 bg)
{
    PROFILE_FUNCTION();

    Vector2 mouse_pos = get_mouse_position();

    GuiFrame frame = {};
    frame.position = mouse_pos;
    frame.color = bg;

    GuiTextbox tb = gui_textbox_data(text, fg, &mouse_pos);
    GuiRenderItem text_item = gui_textbox_render_item(tb);

    frame.position.x = min(frame.position.x, tb.position.x);
    frame.position.y = min(frame.position.y, tb.position.y);

    frame.width = max(
        frame.width,
        tb.position.x + tb.size.x - frame.position.x);

    frame.height = max(
        frame.height,
        tb.position.y + tb.size.y - frame.position.y);

    GuiRenderItem frame_item = gui_frame_render_item(
        frame.position,
        frame.width, frame.height,
        frame.color);

    array_add(&g_gui_tooltip_render_queue, frame_item);
    array_add(&g_gui_tooltip_render_queue, text_item);
}

