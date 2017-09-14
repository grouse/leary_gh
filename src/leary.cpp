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
#include "platform/platform_input.h"

#include "core/hash.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"
#include "core/hash_table.cpp"
#include "core/lexer.cpp"
#include "core/profiling.cpp"
#include "core/maths.cpp"
#include "core/random.cpp"
#include "core/assets.cpp"
#include "core/string.cpp"

#include "vulkan_render.cpp"

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
    VulkanBuffer   vbo;
    VulkanBuffer   ibo;
    i32            index_count;
    //Matrix4        transform;
    Material       *material;
};

// TODO(jesper): stick these in the pipeline so that we reduce the number of
// pipeline binds
struct RenderObject {
    VulkanPipeline pipeline;
    VulkanBuffer   vbo;
    i32            vertex_count;
    Matrix4        transform;
    Material       *material;
};

struct Terrain {
    struct Chunk {
        Array<Vertex> vertices;
        VulkanBuffer vbo;
    };
    Array<Chunk> chunks;
    VulkanPipeline *pipeline;
};

struct Camera {
    Matrix4             view;
    Matrix4             projection;
    VulkanUniformBuffer ubo;

    Vector3             position;
    f32 yaw   = 0.0f;
    f32 pitch = 0.0f;
    Quaternion rotation = Quaternion::make({ 1.0f, 0.0f, 0.0f }, 2.2f * PI);
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
    Array<VkDescriptorSet> descriptors  = {};
    VulkanBuffer           vbo;
    i32                    vertex_count = 0;
    PushConstants          constants;
};

struct DebugOverlayItem {
    const char               *title;
    Vector2 tl, br;
    Array<DebugOverlayItem*> children;
    bool                     collapsed;
    DebugOverlayItemType     type;
    union {
        void            *data;
        DebugRenderItem ritem;
    } u;
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
        Material font;
        Material heightmap;
        Material phong;
        Material player;
    } materials;

    DebugOverlay overlay;

    Array<Entity> entities;
    Physics physics;

    Camera fp_camera = {};

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

    PlatformState *platform;
    Settings      settings;
    Catalog       texture_catalog;

    VulkanDevice  *vulkan_device;
    GameState     *game;
};

VulkanDevice *g_vulkan;
GameState    *g_game;

// NOTE(jesper): don't keep an address to these globals!!!!
Matrix4      g_view_to_screen; // [-1  , 1]   -> [0, w]
Matrix4      g_screen_to_view; // [0, w] -> [-1  , 1]

extern Settings      g_settings;
extern PlatformState *g_platform;

Catalog      g_texture_catalog;
Terrain      g_terrain;

void debug_add_texture(const char *name,
                       i32 tid,
                       Material material,
                       VulkanPipeline *pipeline,
                       DebugOverlay *overlay)
{
    Texture texture = find_texture(tid);

    if (texture.id == TEXTURE_INVALID_ID) {
        return;
    }

    DebugOverlayItem item = {};
    item.title            = name;
    item.type             = Debug_render_item;
    item.u.ritem.pipeline   = pipeline;
    item.u.ritem.constants  = create_push_constants(pipeline->id);

    Matrix4 t = Matrix4::identity();
    t[0].x =  g_screen_to_view[0].x;
    t[1].y =  g_screen_to_view[1].y;

    Vector2 dim = { (f32)texture.width, (f32)texture.height };
    dim = t * dim;

    f32 vertices[] = {
        0.0f,  0.0f,  0.0f, 0.0f,
        dim.x, 0.0f,  1.0f, 0.0f,
        dim.x, dim.y, 1.0f, 1.0f,

        dim.x, dim.y, 1.0f, 1.0f,
        0.0f,  dim.y, 0.0f, 1.0f,
        0.0f,  0.0f,  0.0f, 0.0f,
    };

    item.u.ritem.vbo = create_vbo(vertices, sizeof(vertices) * sizeof(f32));
    item.u.ritem.vertex_count = 6;

    item.u.ritem.descriptors = array_create<VkDescriptorSet>(g_heap, 1);
    array_add(&item.u.ritem.descriptors, material.descriptor_set);
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
    PROFILE_FUNCTION();
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

void game_init()
{
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    g_game = g_persistent->ialloc<GameState>();

    init_profiling();

    init_vulkan();
    init_texture_catalog();

    f32 width = (f32)g_settings.video.resolution.width;
    f32 height = (f32)g_settings.video.resolution.height;
    f32 aspect = width / height;
    f32 vfov   = radians(45.0f);

    g_game->fp_camera.view       = Matrix4::identity();
    g_game->fp_camera.position   = Vector3{0.0f, 5.0f, 0.0f};
    g_game->fp_camera.projection = Matrix4::perspective(vfov, aspect, 0.1f, 10000.0f);

    g_game->render_objects       = array_create<RenderObject>(g_persistent, 20);
    g_game->index_render_objects = array_create<IndexRenderObject>(g_persistent, 20);


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


    // create pipelines
    {
        g_game->pipelines.mesh    = create_pipeline(Pipeline_mesh);
        g_game->pipelines.basic2d = create_pipeline(Pipeline_basic2d);
        g_game->pipelines.font    = create_pipeline(Pipeline_font);
        g_game->pipelines.terrain = create_pipeline(Pipeline_terrain);
    }

    // create ubos
    {
        g_game->fp_camera.ubo = create_ubo(sizeof(Matrix4));

        Matrix4 view_projection = g_game->fp_camera.projection * g_game->fp_camera.view;
        buffer_data(g_game->fp_camera.ubo, &view_projection, 0, sizeof(view_projection));
    }

    // create materials
    {
        g_game->materials.font      = create_material(&g_game->pipelines.font, Material_basic2d);
        g_game->materials.heightmap = create_material(&g_game->pipelines.basic2d, Material_basic2d);
        g_game->materials.phong     = create_material(&g_game->pipelines.mesh, Material_phong);
        g_game->materials.player    = create_material(&g_game->pipelines.mesh, Material_phong);
    }

    // update descriptor sets
    {
        set_ubo(&g_game->pipelines.mesh, ResourceSlot_mvp, &g_game->fp_camera.ubo);
        set_ubo(&g_game->pipelines.terrain, ResourceSlot_mvp, &g_game->fp_camera.ubo);

        Texture font   = find_texture("font-regular");
        Texture cube   = find_texture("dummy.bmp");
        Texture player = find_texture("player.bmp");

        set_texture(&g_game->materials.font,   ResourceSlot_diffuse, font);
        set_texture(&g_game->materials.phong,  ResourceSlot_diffuse, cube);
        set_texture(&g_game->materials.player, ResourceSlot_diffuse, player);
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
        obj.vbo         = create_vbo(cube.vertices.data, vertex_size);
        obj.ibo         = create_ibo(cube.indices.data, index_size);

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
        obj.vbo         = create_vbo(cube.vertices.data, vertex_size);
        obj.ibo         = create_ibo(cube.indices.data, index_size);

        //obj.transform = translate(Matrix4::identity(), {x, y, z});
        array_add(&g_game->index_render_objects, obj);
    }

    {
        Texture hm = find_texture("terrain.bmp");
        DEBUG_ASSERT(hm.id != TEXTURE_INVALID_ID);
        DEBUG_ASSERT(hm.size > 0);

        struct Texel {
            u8 r, g, b, a;
        };

        u32  vc       = hm.height * hm.width;
        auto vertices = array_create<Vertex>(g_persistent, vc);

        // TODO(jesper): move to settings/asset info/something
        Vector3 w = { 50.0f, 50.0f, 5.0f };
        f32 xx = w.x * 2.0f;
        f32 yy = w.y * 2.0f;
        f32 zz = w.z * 2.0f;

        Matrix4 to_world = Matrix4::identity();
        to_world[0].x = xx / (f32)hm.width;
        to_world[3].x = -w.x;

        to_world[1].y = zz / 255.0f;
        to_world[3].y = -w.z;

        to_world[2].z = yy / (f32)hm.height;
        to_world[3].z = -w.y;

        for (u32 i = 0; i < hm.height-1; i++) {
            for (u32 j = 0; j < hm.width-1; j++) {
                Texel t0   = ((Texel*)hm.data)[i     * hm.width + j];
                Texel t1   = ((Texel*)hm.data)[i     * hm.width + j+1];
                Texel t2   = ((Texel*)hm.data)[(i+1) * hm.width + j+1];
                Texel t3   = ((Texel*)hm.data)[(i+1) * hm.width + j];

                Vector3 v0 = to_world * Vector3{ (f32)j,   (f32)t0.r, (f32)i   };
                Vector3 v1 = to_world * Vector3{ (f32)j+1, (f32)t1.r, (f32)i   };
                Vector3 v2 = to_world * Vector3{ (f32)j+1, (f32)t2.r, (f32)i+1 };
                Vector3 v3 = to_world * Vector3{ (f32)j,   (f32)t3.r, (f32)i+1 };

                Vector3 n = surface_normal(v0, v1, v2);
                array_add(&vertices, {v0, n, {}});
                array_add(&vertices, {v1, n, {}});
                array_add(&vertices, {v2, n, {}});

                n = surface_normal(v0, v2, v3);
                array_add(&vertices, {v0, n, {}});
                array_add(&vertices, {v2, n, {}});
                array_add(&vertices, {v3, n, {}});
            }
        }

        // NOTE(jesper): this is redundant; we're transforming the heightmap
        // vertices to [-50,50] with to_world. Included for completeness when
        // moving to arbitrary meshes
        f32 min_x = F32_MAX, max_x = -F32_MAX;
        f32 min_z = F32_MAX, max_z = -F32_MAX;
        for (i32 i = 0; i < vertices.count; i++) {
            if (vertices[i].p.x < min_x) {
                min_x = vertices[i].p.x;
            } else if (vertices[i].p.x > max_x) {
                max_x = vertices[i].p.x;
            }

            if (vertices[i].p.z < min_z) {
                min_z = vertices[i].p.z;
            } else if (vertices[i].p.z > max_z) {
                max_z = vertices[i].p.z;
            }
        }

        Terrain t = {};
        t.chunks = array_create<Terrain::Chunk>(g_heap, 4);
        t.chunks.count = 4;

        f32 mid_x = (min_x + max_x) / 2.0f;
        f32 mid_z = (min_z + max_z) / 2.0f;
        for (i32 i = 0; i < t.chunks.count; i++) {
            t.chunks[i].vertices = array_create<Vertex>(g_heap);
        }

        for (i32 i = 0; i < vertices.count; i+= 3) {
            Vertex v0 = vertices[i];
            Vertex v1 = vertices[i+1];
            Vertex v2 = vertices[i+2];

            if (v0.p.x >= mid_x || v1.p.x >= mid_x || v2.p.x >= mid_x) {
                if (v0.p.z >= mid_z || v1.p.z >= mid_z || v2.p.z >= mid_z) {
                    array_add(&t.chunks[0].vertices, v0);
                    array_add(&t.chunks[0].vertices, v1);
                    array_add(&t.chunks[0].vertices, v2);
                } else {
                    array_add(&t.chunks[1].vertices, v0);
                    array_add(&t.chunks[1].vertices, v1);
                    array_add(&t.chunks[1].vertices, v2);
                }
            } else {
                if (v0.p.z >= mid_z || v1.p.z >= mid_z || v2.p.z >= mid_z) {
                    array_add(&t.chunks[2].vertices, v0);
                    array_add(&t.chunks[2].vertices, v1);
                    array_add(&t.chunks[2].vertices, v2);
                } else {
                    array_add(&t.chunks[3].vertices, v0);
                    array_add(&t.chunks[3].vertices, v1);
                    array_add(&t.chunks[3].vertices, v2);
                }
            }
        }

        t.pipeline = &g_game->pipelines.terrain;
        for (i32 i = 0; i < t.chunks.count; i++) {
            Terrain::Chunk &c = t.chunks[i];

            usize vertex_size = c.vertices.count * sizeof(c.vertices[0]);
            c.vbo      = create_vbo(c.vertices.data, vertex_size);
        }

        g_terrain = t;
        array_destroy(&vertices);
#if 0
        RenderObject ro = {};
        ro.pipeline       = g_game->pipelines.terrain;

        usize vertex_size = vertices.count * sizeof(vertices[0]);
        ro.vertex_count   = vertices.count;
        ro.vertices       = create_vbo(vertices.data, vertex_size);

        array_add(&g_game->render_objects, ro);
#endif
        set_texture(&g_game->materials.heightmap, ResourceSlot_diffuse, hm);

    }

    g_game->key_state = g_persistent->ialloc_array<i32>(256, InputType_key_release);

    g_game->overlay.items        = array_create<DebugOverlayItem>(g_heap);
    g_game->overlay.render_queue = array_create<DebugRenderItem>(g_heap);
    {
        DebugOverlayItem allocators = {};
        allocators.title    = "Allocators";
        allocators.children = array_create<DebugOverlayItem*>(g_heap);
        allocators.type     = Debug_allocators;

        auto stack = g_heap->talloc<DebugOverlayItem>();
        stack->type  = Debug_allocator_stack;
        stack->title = "stack";
        stack->u.data  = (void*)g_stack;
        array_add(&allocators.children, stack);

        auto frame = g_heap->talloc<DebugOverlayItem>();
        frame->type  = Debug_allocator_stack;
        frame->title = "frame";
        frame->u.data  = (void*)g_frame;
        array_add(&allocators.children, frame);

        auto persistent = g_heap->talloc<DebugOverlayItem>();
        persistent->type  = Debug_allocator_stack;
        persistent->title = "persistent";
        persistent->u.data  = (void*)g_persistent;
        array_add(&allocators.children, persistent);

        auto debug_frame = g_heap->talloc<DebugOverlayItem>();
        debug_frame->type  = Debug_allocator_stack;
        debug_frame->title = "debug_frame";
        debug_frame->u.data  = (void*)g_debug_frame;
        array_add(&allocators.children, debug_frame);

        auto free_list = g_heap->talloc<DebugOverlayItem>();
        free_list->type  = Debug_allocator_free_list;
        free_list->title = "free list";
        free_list->u.data  = (void*)g_heap;
        array_add(&allocators.children, free_list);

        array_add(&g_game->overlay.items, allocators);


        DebugOverlayItem timers = {};
        timers.title = "Profile Timers";
        timers.type  = Debug_profile_timers;
        array_add(&g_game->overlay.items, timers);

        i32 hmt = find_texture_id("terrain.bmp");
        debug_add_texture("Terrain", hmt, g_game->materials.heightmap,
                          &g_game->pipelines.basic2d, &g_game->overlay);
    }
}

void game_quit()
{
    // NOTE(jesper): disable raw mouse as soon as possible to ungrab the cursor
    // on Linux
    platform_set_raw_mouse(false);

    vkQueueWaitIdle(g_vulkan->queue);

    for (i32 i = 0; i < g_terrain.chunks.count; i++) {
        buffer_destroy(g_terrain.chunks[i].vbo);
    }

    for (int i = 0; i < g_game->overlay.items.count; i++) {
        DebugOverlayItem &item = g_game->overlay.items[i];
        if (item.type == Debug_render_item) {
            buffer_destroy(item.u.ritem.vbo);
        }
    }

    for (i32 i = 0; i < g_texture_catalog.textures.count; i++) {
        destroy_texture(&g_texture_catalog.textures[i]);
    }

    buffer_destroy(g_game->overlay.vbo);
    buffer_destroy(g_game->overlay.texture.vbo);

    destroy_material(g_game->materials.font);
    destroy_material(g_game->materials.heightmap);
    destroy_material(g_game->materials.phong);
    destroy_material(g_game->materials.player);

    destroy_pipeline(g_game->pipelines.basic2d);
    destroy_pipeline(g_game->pipelines.font);
    destroy_pipeline(g_game->pipelines.mesh);
    destroy_pipeline(g_game->pipelines.terrain);

    for (i32 i = 0; i < g_game->render_objects.count; i++) {
        buffer_destroy(g_game->render_objects[i].vbo);
    }

    for (i32 i = 0; i < g_game->index_render_objects.count; i++) {
        buffer_destroy(g_game->index_render_objects[i].vbo);
        buffer_destroy(g_game->index_render_objects[i].ibo);
    }

    buffer_destroy_ubo(g_game->fp_camera.ubo);
    destroy_vulkan();

    platform_quit();
}

void* game_pre_reload()
{
    auto state = g_frame->talloc<GameReloadState>();

    // TODO(jesper): I feel like this could be quite nicely preprocessed and
    // generated. look into
    state->profile_timers      = g_profile_timers;
    state->profile_timers_prev = g_profile_timers_prev;

    state->texture_catalog     = g_texture_catalog;
    state->settings            = g_settings;

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

    g_settings            = state->settings;
    g_texture_catalog     = state->texture_catalog;

    g_screen_to_view      = state->screen_to_view;
    g_view_to_screen      = state->view_to_screen;

    g_vulkan              = state->vulkan_device;
    load_vulkan(g_vulkan->instance);

    g_frame->dealloc(state);
}

void game_input(InputEvent event)
{
    switch (event.type) {
    case InputType_key_press: {
        if (event.key.repeated) {
            break;
        }

        g_game->key_state[event.key.code] = InputType_key_press;

        switch (event.key.code) {
        case Key_escape:
            game_quit();
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
            platform_toggle_raw_mouse();
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

                game_input(e);
            }
            break;
        case Key_S:
            g_game->velocity.z = 0.0f;
            if (g_game->key_state[Key_W] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_W;
                e.key.repeated = false;

                game_input(e);
            }
            break;
        case Key_A:
            g_game->velocity.x = 0.0f;
            if (g_game->key_state[Key_D] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_D;
                e.key.repeated = false;

                game_input(e);
            }
            break;
        case Key_D:
            g_game->velocity.x = 0.0f;
            if (g_game->key_state[Key_A] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_A;
                e.key.repeated = false;

                game_input(e);
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

                game_input(e);
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

                game_input(e);
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

                game_input(e);
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

                game_input(e);
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

    process_texture_catalog();

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
            for (int c = 0; c < item.children.count; c++) {
                DebugOverlayItem &child = *item.children[c];

                switch (child.type) {
                case Debug_allocator_stack: {
                    StackAllocator *a = (StackAllocator*)child.u.data;
                    snprintf(buffer, buffer_size,
                             "  %s: { sp: %p, size: %zd, remaining: %zd }\n",
                             child.title, a->sp, a->size, a->remaining);
                    render_font(font, buffer, &pos, vcount, mapped, &offset);
                } break;
                case Debug_allocator_free_list: {
                    Allocator *a = (Allocator*)child.u.data;
                    snprintf(buffer, buffer_size,
                             "  %s: { size: %zd, remaining: %zd }\n",
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
            for (int j = 0; j < timers.count; j++) {
                snprintf(buffer, buffer_size, "%s: ", timers.name[j]);
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
            for (int j = 0; j < timers.count; j++) {
                snprintf(buffer, buffer_size, "%" PRIu64, timers.cycles[j]);
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
            for (int j = 0; j < timers.count; j++) {
                snprintf(buffer, buffer_size, "%u", timers.calls[j]);
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
            for (int j = 0; j < timers.count; j++) {
                snprintf(buffer, buffer_size, "%f", timers.cycles[j] / (f32)timers.calls[j]);
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
            item.u.ritem.position = g_screen_to_view * (pos + Vector2{10.0f, 0.0f});

            Matrix4 t = translate(Matrix4::identity(), item.u.ritem.position);
            set_push_constant(&item.u.ritem.constants, t);

            array_add(&overlay->render_queue, item.u.ritem);
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
        buffer_data(g_game->fp_camera.ubo, &vp, 0, sizeof(vp));
    }

}

void render_terrain(VkCommandBuffer command)
{
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindPipeline(command,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      g_terrain.pipeline->handle);

    vkCmdBindDescriptorSets(command,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            g_terrain.pipeline->layout,
                            0,
                            1, &g_terrain.pipeline->descriptor_set,
                            0, nullptr);

    for (i32 i = 0; i < g_terrain.chunks.count; i++) {
        Terrain::Chunk &c = g_terrain.chunks[i];

        vkCmdBindVertexBuffers(command, 0, 1, &c.vbo.handle, offsets);
        vkCmdDraw(command, (u32)c.vertices.count, 1, 0, 0);
    }
}

void game_render()
{
    PROFILE_FUNCTION();
    void *sp = g_stack->sp;
    defer { g_stack->reset(sp); };

    u32 image_index = acquire_swapchain();


    VkDeviceSize offsets[] = { 0 };
    VkResult result;

    VkCommandBuffer command = begin_cmd_buffer();
    begin_renderpass(command, image_index);

    render_terrain(command);


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

        vkCmdBindVertexBuffers(command, 0, 1, &object.vbo.handle, offsets);

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

        vkCmdBindVertexBuffers(command, 0, 1, &object.vbo.handle, offsets);
        vkCmdBindIndexBuffer(command, object.ibo.handle,
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
                                (u32)item.descriptors.count,
                                item.descriptors.data,
                                0, nullptr);

        vkCmdPushConstants(command, item.pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           item.constants.offset,
                           (u32)item.constants.size,
                           item.constants.data);

        Matrix4 t = translate(Matrix4::identity(), item.position);
        vkCmdPushConstants(command, item.pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t), &t);

        vkCmdBindVertexBuffers(command, 0, 1, &item.vbo.handle, offsets);
        vkCmdDraw(command, item.vertex_count, 1, 0, 0);
    }
    g_game->overlay.render_queue.count = 0;

    end_renderpass(command);
    end_cmd_buffer(command, false);

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
    g_debug_frame->reset();
}
