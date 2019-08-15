/**
 * file:    leary.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#ifndef INTROSPECT
#define INTROSPECT
#endif

enum DebugOverlayItemType {
    Debug_allocator_stack,
    Debug_allocator_free_list,
    Debug_render_item,
    Debug_allocators
};

struct DebugRenderItem {
    Vector2                 position;
    PipelineID              pipeline;
    Array<GfxDescriptorSet> descriptors  = {};
    VulkanBuffer            vbo;
    i32                     vertex_count = 0;
    PushConstants           constants;
};

struct DebugOverlayItem {
    StringView               title;
    Array<DebugOverlayItem*> children;
    bool                     collapsed;
    DebugOverlayItemType     type;
    Vector2                  size;

    union {
        void            *data;
        DebugRenderItem ritem;
    } u;
};

struct DebugOverlay {
    bool show_allocators = false;
    bool show_profiler   = false;

    Array<DebugOverlayItem> items;
    Array<DebugRenderItem>  render_queue;
};

struct Entity {
    EntityID   id;
    Vector3    position;
    Vector3    scale;
    Quaternion rotation = Quaternion::make( Vector3{ 0.0f, 1.0f, 0.0f });
    MeshID mesh_id = ASSET_INVALID_ID;
    GfxDescriptorSet descriptor_set;
};

struct IndexRenderObject {
    i32            entity_id = -1;
    PipelineID     pipeline;
    struct {
        VulkanBuffer points;
        VulkanBuffer normals;
        VulkanBuffer tangents;
        VulkanBuffer bitangents;
        VulkanBuffer uvs;
    } vbo;
    VulkanBuffer   ibo;
    i32            index_count;
    //Matrix4        transform;
    Material       *material;
};

// TODO(jesper): stick these in the pipeline so that we reduce the number of
// pipeline binds
struct RenderObject {
    i32            entity_id = -1;
    PipelineID     pipeline;
    struct {
        VulkanBuffer points;
        VulkanBuffer normals;
        VulkanBuffer tangents;
        VulkanBuffer bitangents;
        VulkanBuffer uvs;
    } vbo;
    i32            vertex_count;
    Matrix4        transform;
    Material       *material;
};

struct Camera {
    Matrix4 projection;

    Vector3 position    = { 0.0f, 0.0f, 10.0f };
    Quaternion rotation = quat_from_euler({ 0.0f, 0.0f, 0.0f });

    VulkanUniformBuffer ubo;
};

enum CameraId {
    CAMERA_PLAYER,
    CAMERA_DEBUG,
    CAMERA_COUNT
};

struct GameState {
    struct {
        Material terrain;
        Material font;
        Material heightmap;
        Material phong;
        Material player;
    } materials;

    // TODO(jesper): I don't like this
    bool middle_mouse_pressed = false;
    bool right_mouse_pressed = false;

    DebugOverlay overlay;

    Camera cameras[CAMERA_COUNT] = {};
    CameraId active_camera = CAMERA_DEBUG;

    Array<RenderObject> render_objects;
    Array<IndexRenderObject> index_render_objects;

    Vector3 velocity = {};

    i32 *key_state;
};

extern GameState    *g_game;

INTROSPECT struct Resolution
{
    i32 width  = 1280;
    i32 height = 720;
};

INTROSPECT struct VideoSettings
{
    Resolution resolution;

    i16 fullscreen = 0;
    i16 vsync      = 1;
};

INTROSPECT struct Settings
{
    VideoSettings video;
};

Entity entities_add(EntityData data);

// TODO(jesper): this should be something like gui_icon or gui_texture API
void debug_add_texture(
    StringView name,
    GfxTexture texture,
    GfxDescriptorSet descriptor,
    PipelineID pipeline,
    DebugOverlay *overlay);

void debug_add_texture(
    StringView name,
    Vector2 size,
    VulkanBuffer vbo,
    GfxDescriptorSet descriptor,
    PipelineID pipeline,
    DebugOverlay *overlay);

void game_init();
void game_quit();
void* game_pre_reload();
void game_reload(void *game_state);
void game_input(InputEvent event);
void game_update_and_render(f32 dt);
void game_begin_frame();

