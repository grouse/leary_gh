/**
 * file:    gui.h
 * created: 2018-02-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

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

struct GuiFrame
{
    i32 render_index;
    Vector2 position;
    f32 width;
    f32 height;
    Vector4 color;
};

struct GuiWidget
{
    Vector2 size     = { 0.0f, 0.0f };
    Vector2 position = { 0.0f, 0.0f };
};

struct GuiTextbox
{
    struct Vertex {
        Vector2 position;
        Vector2 uv;
        Vector4 color;
    };

    Vector2 size     = { 0.0f, 0.0f };
    Vector2 position = { 0.0f, 0.0f };
    Vertex *vertices;
    i32 vertices_count = 0;
    i32 vertices_cap = 0;
};

bool is_mouse_over(GuiTextbox textbox);
