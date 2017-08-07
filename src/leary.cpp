/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016 - all rights reserved
 */

#include "leary.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb/stb_truetype.h"

#include "platform/platform.h"
#include "platform/platform_debug.h"
#include "platform/platform_file.h"
#include "platform/platform_input.h"

#include "core/allocator.cpp"
#include "core/array.cpp"
#include "core/tokenizer.cpp"
#include "core/profiling.cpp"
#include "core/math.cpp"
#include "core/random.cpp"
#include "core/assets.cpp"

#include "render/vulkan_device.cpp"

#include "core/serialize.cpp"

struct Entity {
    i32        id;
    i32        index;
    Vector3    position;
    Quaternion rotation = Quaternion::make({ 0.0f, 1.0f, 0.0f });
};

struct IndexRenderObject {
    i32            entity_id;
    VulkanPipeline pipeline;
    VulkanBuffer   vertices;
    VulkanBuffer   indices;
    i32            index_count;
    //Matrix4        transform;
    Material       *material;
};

// TODO(jesper): stick these in the pipeline so that we reduce the number of
// pipeline binds
struct RenderObject {
    VulkanPipeline pipeline;
    VulkanBuffer   vertices;
    i32            vertex_count;
    Matrix4        transform;
    Material       *material;
};

struct Camera {
    Matrix4             view;
    Matrix4             projection;
    VulkanUniformBuffer ubo;

    Vector3             position;
    f32 yaw   = 0.0f;
    f32 pitch = 0.0f;
    Quaternion rotation = Quaternion::make({ 0.0f, 1.0f, 0.0f }, 0.5f * PI);
};

struct Physics {
    Array<Vector3> velocities;
    Array<i32>     entities;
};

enum DebugOverlayItemType {
    Debug_allocator_stack,
    Debug_allocator_free_list,
    Debug_render_item,
    Debug_profile_timers,
    Debug_allocators
};

struct DebugRenderItem {
    Vector2                position;
    VulkanPipeline         *pipeline    = nullptr;
    VulkanTexture          *texture     = nullptr;
    Array<VkDescriptorSet> descriptors  = {};
    VulkanBuffer           vbo;
    i32                    vertex_count = 0;
    PushConstants          constants;
};

struct DebugOverlayItem {
    const char               *title;
    Vector2 tl, br;
    Array<DebugOverlayItem*> children  = {};
    bool                     collapsed = false;
    DebugOverlayItemType     type;
    union {
        void            *data;
        DebugRenderItem ritem = {};
    };
};

struct DebugOverlay {
    VulkanBuffer vbo;
    i32          vertex_count = 0;

    f32             fsize = 20.0f;
    stbtt_bakedchar font[256];

    struct {
        VulkanBuffer vbo;
        i32          vertex_count;
        Vector3      position;
    } texture;

    Array<DebugOverlayItem> items;
    Array<DebugRenderItem>  render_queue;
};

struct GameState {
    struct {
        VulkanPipeline basic2d;
        VulkanPipeline font;
        VulkanPipeline mesh;
        VulkanPipeline terrain;
    } pipelines;

    struct {
        VulkanTexture font;
        VulkanTexture cube;
        VulkanTexture player;
        VulkanTexture heightmap;
    } textures;

    struct {
        Material font;
        Material heightmap;
        Material phong;
        Material player;
    } materials;

    DebugOverlay overlay;

    Array<Entity> entities;
    Physics physics;

    Camera fp_camera;

    Array<RenderObject> render_objects;
    Array<IndexRenderObject> index_render_objects;

    Vector3 velocity = {};

    i32 *key_state;
};

struct GameReloadState {
    ProfileTimers profile_timers;
    ProfileTimers profile_timers_prev;

    Matrix4       screen_to_view;
    Matrix4       view_to_screen;

    VulkanDevice  *vulkan_device;
    GameState     *game;
};

VulkanDevice *g_vulkan;
GameState    *g_game;

// NOTE(jesper): don't keep an address to these globals!!!!
Matrix4      g_view_to_screen; // [-1  , 1]   -> [0, w]
Matrix4      g_screen_to_view; // [0, w] -> [-1  , 1]

void debug_add_texture(const char *name,
                       VulkanTexture *texture,
                       Material material,
                       VulkanPipeline *pipeline,
                       DebugOverlay *overlay)
{
    DebugOverlayItem item;
    item.title           = name;
    item.type            = Debug_render_item;
    item.ritem.pipeline  = pipeline;
    item.ritem.texture   = texture;
    item.ritem.constants = push_constants_create(pipeline->id);

    Matrix4 t = Matrix4::identity();
    t[0].x =  g_screen_to_view[0].x;
    t[1].y =  g_screen_to_view[1].y;

    Vector2 dim = { (f32)texture->width, (f32)texture->height };
    dim = t * dim;

    f32 vertices[] = {
        0.0f,  0.0f,  0.0f, 0.0f,
        dim.x, 0.0f,  1.0f, 0.0f,
        dim.x, dim.y, 1.0f, 1.0f,

        dim.x, dim.y, 1.0f, 1.0f,
        0.0f,  dim.y, 0.0f, 1.0f,
        0.0f,  0.0f,  0.0f, 0.0f,
    };

    item.ritem.vbo = buffer_create_vbo(vertices, sizeof(vertices) * sizeof(f32));
    item.ritem.vertex_count = 6;

    item.ritem.descriptors = array_create<VkDescriptorSet>(g_heap, 1);
    array_add(&item.ritem.descriptors, material.descriptor_set);
    array_add(&overlay->items, item);
}

Entity entities_add(Array<Entity> *entities, Vector3 pos)
{
    Entity e   = {};
    e.id       = (i32)entities->count;
    e.position = pos;

    i32 i = (i32)array_add(entities, e);
    (*entities)[i].index = i;

    return e;
}

Array<Entity> entities_create()
{
    return array_create<Entity>(g_heap);
}

Entity* entity_find(i32 id)
{
    for (i32 i = 0; i < g_game->entities.count; i++) {
        if (g_game->entities[i].id == id) {
            return &g_game->entities[i];
        }
    }

    DEBUG_ASSERT(false);
    return nullptr;
}

Physics physics_create()
{
    Physics p = {};

    p.velocities    = array_create<Vector3>(g_heap);
    p.entities      = array_create<i32>(g_heap);

    return p;
}

void physics_process(Physics *physics, f32 dt)
{
    for (i32 i = 0; i < physics->velocities.count; i++) {
        Entity *e    = entity_find(physics->entities[i]);
        e->position += physics->velocities[i] * dt;
    }
}

i32 physics_add(Physics *physics, Entity entity)
{
    array_add(&physics->velocities,    {});

    i32 id = (i32)array_add(&physics->entities, entity.id);
    return id;
}

void physics_remove(Physics *physics, i32 id)
{
    array_remove(&physics->velocities,    id);
    array_remove(&physics->entities,      id);
}

i32 physics_id(Physics *physics, i32 entity_id)
{
    // TODO(jesper): make a more performant look-up
    i32 id = -1;
    for (i32 i = 0; i < physics->entities.count; i++) {
        if (physics->entities[i] == entity_id) {
            id = i;
            break;
        }
    }

    DEBUG_ASSERT(id != -1);
    return id;
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

void game_init(PlatformState *platform)
{
    g_game = g_persistent->ialloc<GameState>();

    profile_init();


    f32 width = (f32)platform->settings.video.resolution.width;
    f32 height = (f32)platform->settings.video.resolution.height;
    f32 aspect = width / height;
    f32 vfov   = radians(45.0f);

    g_game->fp_camera.view       = Matrix4::identity();
    g_game->fp_camera.position   = Vector3{0.0f, 5.0f, 0.0f};
    g_game->fp_camera.projection = Matrix4::perspective(vfov, aspect, 0.1f, 100.0f);

    g_game->render_objects       = array_create<RenderObject>(g_persistent, 20);
    g_game->index_render_objects = array_create<IndexRenderObject>(g_persistent, 20);

    device_create(platform, &platform->settings);

    { // coordinate bases
        Matrix4 view = Matrix4::identity();
        view[0].x =  2.0f / width;
        view[3].x = -1.0f;
        view[1].y =  2.0f / height;
        view[3].y = -1.0f;
        g_screen_to_view = view;

        view[0].x = width / 2.0f;
        view[3].x = width / 2.0f;
        view[1].y = height / 2.0f;
        view[3].y = height / 2.0f;
        g_view_to_screen = view;
    }

    // create font atlas
    {
        usize font_size;
        char *font_path = platform_resolve_path(GamePath_data,
                                                "fonts/Roboto-Regular.ttf");
        u8 *font_data = (u8*)platform_file_read(font_path, &font_size);

        u8 *bitmap = g_frame->alloc_array<u8>(1024*1024);
        stbtt_BakeFontBitmap(font_data, 0, g_game->overlay.fsize, bitmap,
                             1024, 1024, 0, 256, g_game->overlay.font);

        VkComponentMapping components = {};
        components.a = VK_COMPONENT_SWIZZLE_R;
        g_game->textures.font = texture_create(1024, 1024,
                                             VK_FORMAT_R8_UNORM, bitmap,
                                             components);

        // TODO(jesper): this size is really wrong
        g_game->overlay.vbo = buffer_create_vbo(1024*1024);
    }

    {
    }

    {
        Texture dummy = texture_load_bmp("dummy.bmp");

        // TODO(jesper): texture file loading
        g_game->textures.cube = texture_create(dummy.width, dummy.height,
                                             dummy.format, dummy.data,
                                             VkComponentMapping{});
    }

    {
        Texture player = texture_load_bmp("player.bmp");

        // TODO(jesper): texture file loading
        g_game->textures.player = texture_create(player.width, player.height,
                                               player.format, player.data,
                                               VkComponentMapping{});
    }

    // create pipelines
    {
        g_game->pipelines.mesh    = pipeline_create_mesh();
        g_game->pipelines.basic2d = pipeline_create_basic2d();
        g_game->pipelines.font    = pipeline_create_font();
        g_game->pipelines.terrain = pipeline_create_terrain();
    }

    // create ubos
    {
        g_game->fp_camera.ubo = buffer_create_ubo(sizeof(Matrix4));

        Matrix4 view_projection = g_game->fp_camera.projection * g_game->fp_camera.view;
        buffer_data_ubo(g_game->fp_camera.ubo, &view_projection, 0, sizeof(view_projection));
    }

    // create materials
    {
        g_game->materials.font      = material_create(&g_game->pipelines.font, Material_basic2d);
        g_game->materials.heightmap = material_create(&g_game->pipelines.basic2d, Material_basic2d);
        g_game->materials.phong     = material_create(&g_game->pipelines.mesh, Material_phong);
        g_game->materials.player    = material_create(&g_game->pipelines.mesh, Material_phong);
    }

    // update descriptor sets
    {
        pipeline_set_ubo(&g_game->pipelines.mesh, ResourceSlot_mvp, &g_game->fp_camera.ubo);
        pipeline_set_ubo(&g_game->pipelines.terrain, ResourceSlot_mvp, &g_game->fp_camera.ubo);

        material_set_texture(&g_game->materials.font,   ResourceSlot_diffuse, &g_game->textures.font);
        material_set_texture(&g_game->materials.phong,  ResourceSlot_texture, &g_game->textures.cube);
        material_set_texture(&g_game->materials.player, ResourceSlot_texture, &g_game->textures.player);
    }

    {
        g_game->entities = entities_create();
        g_game->physics  = physics_create();
    }

    Random r = random_create(3);
    Mesh cube = load_mesh_obj("cube.obj");

    {
        f32 x = next_f32(&r) * 20.0f;
        f32 y = -1.0f;
        f32 z = next_f32(&r) * 20.0f;

        Entity player = entities_add(&g_game->entities, {x, y, z});
        i32 pid = physics_add(&g_game->physics, player);
        (void)pid;

        IndexRenderObject obj = {};
        obj.material = &g_game->materials.player;

        usize vertex_size = cube.vertices.count * sizeof(cube.vertices[0]);
        usize index_size  = cube.indices.count  * sizeof(cube.indices[0]);

        obj.entity_id   = player.id;
        obj.pipeline    = g_game->pipelines.mesh;
        obj.index_count = (i32)cube.indices.count;
        obj.vertices    = buffer_create_vbo(cube.vertices.data, vertex_size);
        obj.indices     = buffer_create_ibo(cube.indices.data, index_size);

        //obj.transform = translate(Matrix4::identity(), {x, y, z});
        array_add(&g_game->index_render_objects, obj);
    }

    for (i32 i = 0; i < 10; i++) {
        f32 x = next_f32(&r) * 20.0f;
        f32 y = -1.0f;
        f32 z = next_f32(&r) * 20.0f;

        Entity e = entities_add(&g_game->entities, {x, y, z});
        i32 pid  = physics_add(&g_game->physics, e);
        (void)pid;

        IndexRenderObject obj = {};
        obj.material = &g_game->materials.phong;

        usize vertex_size = cube.vertices.count * sizeof(cube.vertices[0]);
        usize index_size  = cube.indices.count  * sizeof(cube.indices[0]);

        obj.entity_id   = e.id;
        obj.pipeline    = g_game->pipelines.mesh;
        obj.index_count = (i32)cube.indices.count;
        obj.vertices    = buffer_create_vbo(cube.vertices.data, vertex_size);
        obj.indices     = buffer_create_ibo(cube.indices.data, index_size);

        //obj.transform = translate(Matrix4::identity(), {x, y, z});
        array_add(&g_game->index_render_objects, obj);
    }

    {
        Texture heightmap = texture_load_bmp("terrain.bmp");
        DEBUG_ASSERT(heightmap.size > 0);

        struct Texel {
            u8 r, g, b, a;
        };

        struct Vertex {
            f32 x, y, z;
        };

        u32  vc       = heightmap.height * heightmap.width;
        auto vertices = array_create<Vertex>(g_heap, vc);

        // TODO(jesper): move to settings/asset info/something
        Vector3 w = { 100.0f, 100.0f, 20.0f };
        f32 xx = w.x * 2.0f;
        f32 yy = w.y * 2.0f;
        f32 zz = w.z * 2.0f;

        Matrix4 to_world = Matrix4::identity();
        to_world[0].x = xx / (f32)heightmap.width;
        to_world[3].x = -w.x;

        to_world[1].y = yy / (f32)heightmap.height;
        to_world[3].y = -w.y;

        to_world[2].z = zz / 255.0f;
        to_world[3].z = -w.z;

        for (u32 i = 0; i < heightmap.height; i++) {
            for (u32 j = 0; j < heightmap.width; j++) {
                // TODO(jesper): scale i, j and t.r with appropriate values to
                // get it between [world_size.x, world_size.y, world_size.z]
                Texel   t = ((Texel*)heightmap.data)[i * heightmap.width + j];
                Vector3 v = to_world * Vector3{ (f32)j, (f32)i, (f32)t.r };
                Vertex vx = { v.x, v.z, v.y };
                array_add(&vertices, vx);
            }
        }

        u32 ic = (heightmap.height-1) * (heightmap.width-1) * 6;
        auto indices = array_create<u32>(g_heap, ic);
        for (u32 i = 0; i < heightmap.height-1; i++) {
            for (u32 j = 0; j < heightmap.width-1; j++) {
                u32 i0 = i     * heightmap.width + j;
                u32 i1 = i     * heightmap.width + (j+1);
                u32 i2 = (i+1) * heightmap.width + (j+1);

                u32 i3 = i     * heightmap.width + j;
                u32 i4 = (i+1) * heightmap.width + (j+1);
                u32 i5 = (i+1) * heightmap.width + (j);


                array_add(&indices, i0);
                array_add(&indices, i1);
                array_add(&indices, i2);

                array_add(&indices, i3);
                array_add(&indices, i4);
                array_add(&indices, i5);
            }
        }

        // TODO(jesper): this seems stupid
        Entity eterrain = entities_add(&g_game->entities, { 0.0f, 0.0f, 0.0f });

        IndexRenderObject ro = {};
        ro.entity_id      = eterrain.id;
        ro.pipeline       = g_game->pipelines.terrain;

        usize vertex_size = vertices.count * sizeof(vertices[0]);
        usize index_size  = indices.count  * sizeof(indices[0]);

        ro.index_count    = indices.count;
        ro.vertices       = buffer_create_vbo(vertices.data, vertex_size);
        ro.indices        = buffer_create_ibo(indices.data,  index_size);

        array_add(&g_game->index_render_objects, ro);


        // NOTE(jesper): for debug overlay
        g_game->textures.heightmap = texture_create(heightmap.width,
                                                  heightmap.height,
                                                  heightmap.format,
                                                  heightmap.data,
                                                  VkComponentMapping{});
        material_set_texture(&g_game->materials.heightmap, ResourceSlot_diffuse,
                             &g_game->textures.heightmap);

    }



    {
#if 0
        f32 vertices[] = {
            -100.0f, 0.0f, -100.0f,
            100.0f,  0.0f, -100.0f,
            100.0f,  0.0f, 100.0f,

            100.0f,  0.0f, 100.0f,
            -100.0f, 0.0f, 100.0f,
            -100.0f, 0.0f, -100.0f
        };

        RenderObject terrain = {};
        terrain.pipeline = g_game->pipelines.terrain;
        terrain.vertices = buffer_create_vbo(vertices, sizeof(vertices));
        terrain.vertex_count = sizeof(vertices) / (sizeof(vertices[0]) * 3);

        array_add(&g_game->render_objects, terrain);
#endif
    }

    g_game->key_state = g_persistent->alloc_array<i32>(0xFF);
    for (i32 i = 0; i < 0xFF; i++) {
        g_game->key_state[i] = InputType_key_release;
    }

    g_game->overlay.items        = array_create<DebugOverlayItem>(g_heap);
    g_game->overlay.render_queue = array_create<DebugRenderItem>(g_heap);
    {
        DebugOverlayItem allocators;
        allocators.title    = "Allocators";
        allocators.children = array_create<DebugOverlayItem*>(g_heap);
        allocators.type     = Debug_allocators;

        auto stack = g_heap->talloc<DebugOverlayItem>();
        stack->type  = Debug_allocator_stack;
        stack->title = "stack";
        stack->data  = (void*)g_stack;
        array_add(&allocators.children, stack);

        auto frame = g_heap->talloc<DebugOverlayItem>();
        frame->type  = Debug_allocator_stack;
        frame->title = "frame";
        frame->data  = (void*)g_frame;
        array_add(&allocators.children, frame);

        auto persistent = g_heap->talloc<DebugOverlayItem>();
        persistent->type  = Debug_allocator_stack;
        persistent->title = "persistent";
        persistent->data  = (void*)g_persistent;
        array_add(&allocators.children, persistent);

        auto free_list = g_heap->talloc<DebugOverlayItem>();
        free_list->type  = Debug_allocator_free_list;
        free_list->title = "free list";
        free_list->data  = (void*)g_heap;
        array_add(&allocators.children, free_list);

        array_add(&g_game->overlay.items, allocators);


        DebugOverlayItem timers;
        timers.title = "Profile Timers";
        timers.type  = Debug_profile_timers;
        array_add(&g_game->overlay.items, timers);

        debug_add_texture("Terrain", &g_game->textures.heightmap, g_game->materials.heightmap,
                          &g_game->pipelines.basic2d, &g_game->overlay);
    }
}

void game_quit(PlatformState *platform)
{
    // NOTE(jesper): disable raw mouse as soon as possible to ungrab the cursor
    // on Linux
    platform_set_raw_mouse(platform, false);

    vkQueueWaitIdle(g_vulkan->queue);

    for (int i = 0; i < g_game->overlay.items.count; i++) {
        DebugOverlayItem &item = g_game->overlay.items[i];
        if (item.type == Debug_render_item) {
            buffer_destroy(item.ritem.vbo);
        }
    }

    buffer_destroy(g_game->overlay.vbo);
    buffer_destroy(g_game->overlay.texture.vbo);

    texture_destroy(g_game->textures.font);
    texture_destroy(g_game->textures.cube);
    texture_destroy(g_game->textures.player);
    texture_destroy(g_game->textures.heightmap);

    material_destroy(g_game->materials.font);
    material_destroy(g_game->materials.heightmap);
    material_destroy(g_game->materials.phong);
    material_destroy(g_game->materials.player);

    pipeline_destroy(g_game->pipelines.basic2d);
    pipeline_destroy(g_game->pipelines.font);
    pipeline_destroy(g_game->pipelines.mesh);
    pipeline_destroy(g_game->pipelines.terrain);

    for (i32 i = 0; i < g_game->render_objects.count; i++) {
        buffer_destroy(g_game->render_objects[i].vertices);
    }

    for (i32 i = 0; i < g_game->index_render_objects.count; i++) {
        buffer_destroy(g_game->index_render_objects[i].vertices);
        buffer_destroy(g_game->index_render_objects[i].indices);
    }

    buffer_destroy_ubo(g_game->fp_camera.ubo);
    vulkan_destroy();

    platform_quit(platform);
}

void* game_pre_reload()
{
    auto state = g_frame->talloc<GameReloadState>();

    // TODO(jesper): I feel like this could be quite nicely preprocessed and
    // generated. look into
    state->profile_timers      = g_profile_timers;
    state->profile_timers_prev = g_profile_timers_prev;

    state->screen_to_view      = g_screen_to_view;
    state->view_to_screen      = g_view_to_screen;

    state->vulkan_device       = g_vulkan;
    state->game                = g_game;

    // NOTE(jesper): wait for the vulkan queues to be idle. Here for when I get
    // to shader and resource reloading - I don't even want to think about what
    // kind of fits graphics drivers will throw if we start recreating pipelines
    // in the middle of things
    vkQueueWaitIdle(g_vulkan->queue);

    return (void*)state;
}

void game_reload(void *s)
{
    // TODO(jesper): I feel like this could be quite nicely preprocessed and
    // generated. look into
    auto state = (GameReloadState*)s;
    g_game                = state->game;

    g_profile_timers      = state->profile_timers;
    g_profile_timers_prev = state->profile_timers_prev;

    g_screen_to_view      = state->screen_to_view;
    g_view_to_screen      = state->view_to_screen;

    g_vulkan              = state->vulkan_device;
    vulkan_load(g_vulkan->instance);

    g_frame->dealloc(state);
}

void game_input(PlatformState *platform, InputEvent event)
{
    switch (event.type) {
    case InputType_key_press: {
        if (event.key.repeated) {
            break;
        }

        g_game->key_state[event.key.code] = InputType_key_press;

        switch (event.key.code) {
        case Key_escape:
            game_quit(platform);
            break;
        case Key_W:
            // TODO(jesper): tweak movement speed when we have a sense of scale
            g_game->velocity.z = 3.0f;
            break;
        case Key_S:
            // TODO(jesper): tweak movement speed when we have a sense of scale
            g_game->velocity.z = -3.0f;
            break;
        case Key_A:
            // TODO(jesper): tweak movement speed when we have a sense of scale
            g_game->velocity.x = -3.0f;
            break;
        case Key_D:
            // TODO(jesper): tweak movement speed when we have a sense of scale
            g_game->velocity.x = 3.0f;
            break;
        case Key_F:
            for (int i = 0; i < g_game->overlay.items.count; i++) {
                g_game->overlay.items[i].collapsed = !g_game->overlay.items[i].collapsed;
            }
            break;
        case Key_left: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].x = 5.0f;
        } break;
        case Key_right: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].x = -5.0f;
        } break;
        case Key_up: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].z = 5.0f;
        } break;
        case Key_down: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].z = -5.0f;
        } break;
        case Key_C:
            platform_toggle_raw_mouse(platform);
            break;
        default:
            //DEBUG_LOG("unhandled key press: %d", event.key.code);
            break;
        }
    } break;
    case InputType_key_release: {
        if (event.key.repeated) {
            break;
        }

        g_game->key_state[event.key.code] = InputType_key_release;

        switch (event.key.code) {
        case Key_W:
            g_game->velocity.z = 0.0f;
            if (g_game->key_state[Key_S] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_S;
                e.key.repeated = false;

                game_input(platform, e);
            }
            break;
        case Key_S:
            g_game->velocity.z = 0.0f;
            if (g_game->key_state[Key_W] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_W;
                e.key.repeated = false;

                game_input(platform, e);
            }
            break;
        case Key_A:
            g_game->velocity.x = 0.0f;
            if (g_game->key_state[Key_D] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_D;
                e.key.repeated = false;

                game_input(platform, e);
            }
            break;
        case Key_D:
            g_game->velocity.x = 0.0f;
            if (g_game->key_state[Key_A] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_A;
                e.key.repeated = false;

                game_input(platform, e);
            }
            break;
        case Key_left: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].x = 0.0f;

            if (g_game->key_state[Key_right] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_right;
                e.key.repeated = false;

                game_input(platform, e);
            }
        } break;
        case Key_right: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].x = 0.0f;

            if (g_game->key_state[Key_left] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_left;
                e.key.repeated = false;

                game_input(platform, e);
            }
        } break;
        case Key_up: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].z = 0.0f;

            if (g_game->key_state[Key_down] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_down;
                e.key.repeated = false;

                game_input(platform, e);
            }
        } break;
        case Key_down: {
            i32 pid = physics_id(&g_game->physics, 0);
            g_game->physics.velocities[pid].z = 0.0f;

            if (g_game->key_state[Key_up] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_up;
                e.key.repeated = false;

                game_input(platform, e);
            }
        } break;
        default:
            //DEBUG_LOG("unhandled key release: %d", event.key.code);
            break;
        }
    } break;
    case InputType_mouse_move: {
        // TODO(jesper): move mouse sensitivity into settings
        g_game->fp_camera.yaw   += 0.001f * event.mouse.dx;
        g_game->fp_camera.pitch += 0.001f * event.mouse.dy;
    } break;
    case InputType_mouse_press: {
        Vector2 p = { event.mouse.x, event.mouse.y };
        for (u32 i = 0; i < g_game->overlay.items.count; i++) {
            auto &item = g_game->overlay.items[i];

            if (p.x >= item.tl.x && p.x <= item.br.x &&
                p.y >= item.tl.y && p.y <= item.br.y)
            {
                item.collapsed = !item.collapsed;
            }
        }
    } break;
    default:
        //DEBUG_LOG("unhandled input type: %d", event.type);
        break;
    }
}

void debug_overlay_update(DebugOverlay *overlay, f32 dt)
{
    PROFILE_FUNCTION();
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    stbtt_bakedchar *font = overlay->font;
    i32 *vcount           = &overlay->vertex_count;

    usize offset = 0;
    Vector2 pos = { -1.0f, -1.0f };
    pos = g_view_to_screen * pos;

    *vcount = 0;

    void *mapped;
    vkMapMemory(g_vulkan->handle, overlay->vbo.memory,
                0, VK_WHOLE_SIZE, 0, &mapped);


    isize buffer_size = 1024*1024;
    char *buffer = (char*)g_stack->alloc(buffer_size);
    buffer[0] = '\0';

    f32 dt_ms = dt * 1000.0f;
    snprintf(buffer, buffer_size, "frametime: %f ms, %f fps\n",
             dt_ms, 1000.0f / dt_ms);
    render_font(font, buffer, &pos, vcount, mapped, &offset);

    for (int i = 0; i < overlay->items.count; i++) {
        DebugOverlayItem &item = overlay->items[i];

        {
            f32 base_x = pos.x;
            item.tl = pos;
            defer {
                pos.y   += overlay->fsize;
                item.br  = pos;

                pos.x    = base_x;
            };

            if (item.collapsed) {
                snprintf(buffer, buffer_size, "%s...", item.title);
                render_font(font, buffer, &pos, vcount, mapped, &offset);
                continue;
            }

            snprintf(buffer, buffer_size, "%s", item.title);
            render_font(font, buffer, &pos, vcount, mapped, &offset);
        }

        switch (item.type) {
        case Debug_allocators: {
            for (int i = 0; i < item.children.count; i++) {
                DebugOverlayItem &child = *item.children[i];

                switch (child.type) {
                case Debug_allocator_stack: {
                    StackAllocator *a = (StackAllocator*)child.data;
                    snprintf(buffer, buffer_size,
                             "  %s: { sp: %p, size: %ld, remaining: %ld }\n",
                             child.title, a->sp, a->size, a->remaining);
                    render_font(font, buffer, &pos, vcount, mapped, &offset);
                } break;
                case Debug_allocator_free_list: {
                    Allocator *a = (Allocator*)child.data;
                    snprintf(buffer, buffer_size,
                             "  %s: { size: %ld, remaining: %ld }\n",
                             child.title, a->size, a->remaining);
                    render_font(font, buffer, &pos, vcount, mapped, &offset);
                } break;
                default:
                    DEBUG_LOG("unhandled case: %d", item.type);
                    break;
                }
            }
        } break;
        case Debug_profile_timers: {
            f32 margin   = 10.0f;

            f32 hy  = pos.y;
            pos.y  += overlay->fsize;

            f32 base_x = pos.x;
            f32 base_y = pos.y;

            Vector2 c0, c1, c2, c3;
            c0.x = c1.x = c2.x = c3.x = pos.x + margin;
            c0.y = c1.y = c2.y = c3.y = hy;

            pos.x  = c0.x;


            ProfileTimers &timers = g_profile_timers_prev;
            for (int i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%s: ", timers.name[i]);
                render_font(font, buffer, &pos, vcount, mapped, &offset);

                if (pos.x >= c1.x) {
                    c1.x = pos.x;
                }

                pos.x  = c0.x;
                pos.y += overlay->fsize;
            }
            pos.y = base_y;

            c1.x = MAX(c0.x + 250.0f, c1.x) + margin;
            pos.x  = c1.x;
            for (int i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%" PRIu64, timers.cycles[i]);
                render_font(font, buffer, &pos, vcount, mapped, &offset);

                if (pos.x >= c2.x) {
                    c2.x = pos.x;
                }

                pos.x  = c1.x;
                pos.y += overlay->fsize;
            }
            pos.y = base_y;

            c2.x = MAX(c1.x + 100.0f, c2.x) + margin;
            pos.x  = c2.x;
            for (int i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%u", timers.calls[i]);
                render_font(font, buffer, &pos, vcount, mapped, &offset);

                if (pos.x >= c3.x) {
                    c3.x = pos.x;
                }

                pos.x  = c2.x;
                pos.y += overlay->fsize;
            }
            pos.y = base_y;

            c3.x = MAX(c2.x + 75.0f, c3.x) + margin;
            pos.x  = c3.x;
            for (int i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%f", timers.cycles[i] / (f32)timers.calls[i]);
                render_font(font, buffer, &pos, vcount, mapped, &offset);

                pos.x  = c3.x;
                pos.y += overlay->fsize;
            }

            render_font(font, "name",     &c0, vcount, mapped, &offset);
            render_font(font, "cycles",   &c1, vcount, mapped, &offset);
            render_font(font, "calls",    &c2, vcount, mapped, &offset);
            render_font(font, "cy/calls", &c3, vcount, mapped, &offset);

            pos.x = base_x;
        } break;
        case Debug_render_item: {
            item.ritem.position = g_screen_to_view * (pos + Vector2{10.0f, 0.0f});

            Matrix4 t = translate(Matrix4::identity(), item.ritem.position);
            set_push_constant(&item.ritem.constants, t);

            array_add(&overlay->render_queue, item.ritem);
        } break;
        default:
            DEBUG_LOG("unknown debug menu type: %d", item.type);
            break;
        }
    }

    vkUnmapMemory(g_vulkan->handle, overlay->vbo.memory);
}

void game_update(f32 dt)
{
    PROFILE_FUNCTION();

    Entity &player = g_game->entities[0];

    {
        Quaternion r = Quaternion::make({0.0f, 1.0f, 0.0f}, 1.5f * dt);
        player.rotation = player.rotation * r;

        r = Quaternion::make({1.0f, 0.0f, 0.0f}, 0.5f * dt);
        player.rotation = player.rotation * r;
    }


    physics_process(&g_game->physics, dt);


    {
        Vector3 &pos  = g_game->fp_camera.position;
        Matrix4 &view = g_game->fp_camera.view;

        Vector3 f = { view[0].z, view[1].z, view[2].z };
        Vector3 r = -Vector3{ view[0].x, view[1].x, view[2].x };

        pos += dt * (g_game->velocity.z * f + g_game->velocity.x * r);

        Quaternion qpitch = Quaternion::make(r, g_game->fp_camera.pitch);
        Quaternion qyaw   = Quaternion::make({0.0f, 1.0f, 0.0f}, g_game->fp_camera.yaw);

        g_game->fp_camera.pitch = g_game->fp_camera.yaw = 0.0f;

        Quaternion rq            = normalise(qpitch * qyaw);
        g_game->fp_camera.rotation = normalise(g_game->fp_camera.rotation * rq);

        Matrix4 rm = Matrix4::make(g_game->fp_camera.rotation);
        Matrix4 tm = translate(Matrix4::identity(), pos);
        view       = rm * tm;

        Matrix4 vp = g_game->fp_camera.projection * view;
        buffer_data_ubo(g_game->fp_camera.ubo, &vp, 0, sizeof(vp));
    }

}

void game_render()
{
    PROFILE_FUNCTION();
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    u32 image_index = swapchain_acquire();


    VkDeviceSize offsets[] = { 0 };
    VkResult result;

    VkCommandBuffer command = command_buffer_begin();
    renderpass_begin(command, image_index);

    for (i32 i = 0; i < g_game->render_objects.count; i++) {
        RenderObject &object = g_game->render_objects[i];

        vkCmdBindPipeline(command,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          object.pipeline.handle);

        // TODO(jesper): bind material descriptor set if bound
        // TODO(jesper): only bind pipeline descriptor set if one exists, might
        // be such a special case that we should hardcode it?
        vkCmdBindDescriptorSets(command,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                object.pipeline.layout,
                                0,
                                1, &object.pipeline.descriptor_set,
                                0, nullptr);

        vkCmdBindVertexBuffers(command, 0, 1, &object.vertices.handle, offsets);

        vkCmdPushConstants(command, object.pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(object.transform), &object.transform);

        vkCmdDraw(command, object.vertex_count, 1, 0, 0);
    }

    for (i32 i = 0; i < g_game->index_render_objects.count; i++) {
        IndexRenderObject &object = g_game->index_render_objects[i];

        vkCmdBindPipeline(command,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          object.pipeline.handle);

        // TODO(jesper): bind material descriptor set if bound
        // TODO(jesper): only bind pipeline descriptor set if one exists, might
        // be such a special case that we should hardcode it?
        auto descriptors = array_create<VkDescriptorSet>(g_stack);
        defer { array_destroy(&descriptors); };

        array_add(&descriptors, object.pipeline.descriptor_set);


        if (object.material) {
            array_add(&descriptors, object.material->descriptor_set);
        }

        vkCmdBindDescriptorSets(command,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                object.pipeline.layout,
                                0,
                                (i32)descriptors.count, descriptors.data,
                                0, nullptr);

        vkCmdBindVertexBuffers(command, 0, 1, &object.vertices.handle, offsets);
        vkCmdBindIndexBuffer(command, object.indices.handle,
                             0, VK_INDEX_TYPE_UINT32);


        Entity *e = entity_find(object.entity_id);

        Matrix4 t = translate(Matrix4::identity(), e->position);
        Matrix4 r = Matrix4::make(e->rotation);
        t = t * r;

        vkCmdPushConstants(command, object.pipeline.layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t), &t);

        vkCmdDrawIndexed(command, object.index_count, 1, 0, 0, 0);
    }


    vkCmdBindPipeline(command,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      g_game->pipelines.font.handle);


    auto descriptors = array_create<VkDescriptorSet>(g_stack);
    array_add(&descriptors, g_game->materials.font.descriptor_set);

    vkCmdBindDescriptorSets(command,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            g_game->pipelines.font.layout,
                            0,
                            (i32)descriptors.count, descriptors.data,
                            0, nullptr);


    if (g_game->overlay.vertex_count > 0) {
        Matrix4 t = Matrix4::identity();
        vkCmdPushConstants(command, g_game->pipelines.font.layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t), &t);

        vkCmdBindVertexBuffers(command, 0, 1,
                               &g_game->overlay.vbo.handle, offsets);
        vkCmdDraw(command, g_game->overlay.vertex_count, 1, 0, 0);
    }

    for (int i = 0; i < g_game->overlay.render_queue.count; i++) {
        auto &item = g_game->overlay.render_queue[i];

        vkCmdBindPipeline(command,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          item.pipeline->handle);

        vkCmdBindDescriptorSets(command,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                item.pipeline->layout,
                                0,
                                item.descriptors.count,
                                item.descriptors.data,
                                0, nullptr);

        vkCmdPushConstants(command, item.pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           item.constants.offset,
                           item.constants.size,
                           item.constants.data);

        Matrix4 t = translate(Matrix4::identity(), item.position);
        vkCmdPushConstants(command, item.pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t), &t);

        vkCmdBindVertexBuffers(command, 0, 1, &item.vbo.handle, offsets);
        vkCmdDraw(command, item.vertex_count, 1, 0, 0);
    }
    g_game->overlay.render_queue.count = 0;

    renderpass_end(command);
    command_buffer_end(command, false);

    submit_semaphore_wait(g_vulkan->swapchain.available,
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    submit_semaphore_signal(g_vulkan->render_completed);
    submit_frame();

    present_semaphore(g_vulkan->render_completed);
    present_frame(image_index);


    PROFILE_START(vulkan_swap);
    result = vkQueueWaitIdle(g_vulkan->queue);
    DEBUG_ASSERT(result == VK_SUCCESS);
    PROFILE_END(vulkan_swap);
}

void game_update_and_render(f32 dt)
{
    game_update(dt);
    game_render();

    debug_overlay_update(&g_game->overlay, dt);

    g_frame->reset();
}
