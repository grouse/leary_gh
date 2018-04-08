/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2018 - all rights reserved
 */

#include "leary.h"

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
#include "core/file.cpp"
#include "core/font.cpp"
#include "core/gui.cpp"
#include "core/collision.cpp"
#include "core/gfx_vulkan.cpp"

#include "core/serialize.cpp"

struct Terrain {
    struct Chunk {
        Array<Vertex> vertices;
        VulkanBuffer vbo;
    };
    Array<Chunk> chunks;

    PipelineID     pipeline;
    Material       *material;
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

    Terrain        terrain;
    Array<Entity>  entities;
    Physics        physics;

    Collision collision;
    DebugCollision debug_collision;
};


// NOTE(jesper): don't keep an address to these globals!!!!
extern Settings      g_settings;
extern PlatformState *g_platform;
extern Catalog       g_catalog;

Terrain        g_terrain;
Physics        g_physics;

VulkanDevice *g_vulkan;
GameState    *g_game;

void debug_add_texture(const char *name,
                       AssetID tid,
                       Material material,
                       PipelineID pipeline,
                       DebugOverlay *overlay)
{
    Texture *texture = find_texture(tid);
    if (texture == nullptr) {
        return;
    }

    DebugOverlayItem item = {};
    item.title            = name;
    item.type             = Debug_render_item;
    item.u.ritem.pipeline   = pipeline;
    item.u.ritem.constants  = create_push_constants(pipeline);

    Vector2 dim = Vector2{ (f32)texture->width, (f32)texture->height };
    dim.x = dim.x / g_settings.video.resolution.width;
    dim.y = dim.y / g_settings.video.resolution.height;

    f32 vertices[] = {
        0.0f, 0.0f,  0.0f, 0.0f,
        dim.x, 0.0f,  1.0f, 0.0f,
        dim.x, dim.y, 1.0f, 1.0f,

        dim.x, dim.y, 1.0f, 1.0f,
        0.0f,  dim.y, 0.0f, 1.0f,
        0.0f,  0.0f,  0.0f, 0.0f,
    };

    item.u.ritem.vbo = create_vbo(vertices, sizeof(vertices));
    item.u.ritem.vertex_count = 6;

    init_array(&item.u.ritem.descriptors, g_heap, 1);
    array_add(&item.u.ritem.descriptors, material.descriptor_set);
    array_add(&overlay->items, item);
}

Entity entities_add(EntityData data)
{
    Entity e   = {};
    e.id       = (i32)g_entities.count;
    e.position = data.position;
    e.scale    = data.scale;
    e.rotation = data.rotation;

    i32 i = (i32)array_add(&g_entities, e);
    g_entities[i].index = i;

    return e;
}


void init_entity_system()
{
    init_array(&g_entities, g_heap);

    init_array(&g_physics.velocities, g_heap);
    init_array(&g_physics.entities,   g_heap);
}


void init_terrain()
{
    Texture *hm = find_texture("terrain.bmp");
    ASSERT(hm != nullptr);

    struct Texel {
        u8 r, g, b, a;
    };

    u32  vc       = hm->height * hm->width;
    auto vertices = create_array<Vertex>(g_persistent, vc);

    // TODO(jesper): move to settings/asset info/something
    Vector3 w = { 50.0f, 50.0f, 5.0f };
    f32 xx = w.x * 2.0f;
    f32 yy = w.y * 2.0f;
    f32 zz = w.z * 2.0f;

    Matrix4 to_world = Matrix4::identity();
    to_world[0].x = xx / (f32)hm->width;
    to_world[3].x = -w.x;

    to_world[1].y = zz / 255.0f;
    to_world[3].y = -w.z;

    to_world[2].z = yy / (f32)hm->height;
    to_world[3].z = -w.y;

    Vector2 uv_scale = { 1.0f, 1.0f };

    for (u32 i = 0; i < hm->height-1; i++) {
        for (u32 j = 0; j < hm->width-1; j++) {
            Texel t0   = ((Texel*)hm->data)[i     * hm->width + j];
            Texel t1   = ((Texel*)hm->data)[i     * hm->width + j+1];
            Texel t2   = ((Texel*)hm->data)[(i+1) * hm->width + j+1];
            Texel t3   = ((Texel*)hm->data)[(i+1) * hm->width + j];

            Vector3 v0 = to_world * Vector3{ (f32)j,   (f32)t0.r, (f32)i   };
            Vector3 v1 = to_world * Vector3{ (f32)j+1, (f32)t1.r, (f32)i   };
            Vector3 v2 = to_world * Vector3{ (f32)j+1, (f32)t2.r, (f32)i+1 };
            Vector3 v3 = to_world * Vector3{ (f32)j,   (f32)t3.r, (f32)i+1 };

            Vector3 n = surface_normal(v0, v1, v2);
            array_add(&vertices, { v0, n, uv_scale });
            array_add(&vertices, { v1, n, uv_scale });
            array_add(&vertices, { v2, n, uv_scale });

            n = surface_normal(v0, v2, v3);
            array_add(&vertices, { v0, n, uv_scale });
            array_add(&vertices, { v2, n, uv_scale });
            array_add(&vertices, { v3, n, uv_scale });
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
    t.chunks = create_array<Terrain::Chunk>(g_heap, 4);
    t.chunks.count = 4;

    f32 mid_x = (min_x + max_x) / 2.0f;
    f32 mid_z = (min_z + max_z) / 2.0f;
    for (i32 i = 0; i < t.chunks.count; i++) {
        t.chunks[i].vertices = create_array<Vertex>(g_heap);
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

    t.pipeline = Pipeline_terrain;
    t.material = &g_game->materials.terrain;
    for (i32 i = 0; i < t.chunks.count; i++) {
        Terrain::Chunk &c = t.chunks[i];

        usize vertex_size = c.vertices.count * sizeof(c.vertices[0]);
        c.vbo = create_vbo(c.vertices.data, vertex_size);
    }

    g_terrain = t;
    destroy_array(&vertices);
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

void init_debug_overlay()
{
    g_game->overlay.items        = create_array<DebugOverlayItem>(g_heap);
    g_game->overlay.render_queue = create_array<DebugRenderItem>(g_heap);

    DebugOverlayItem allocators = {};
    allocators.title    = "Allocators";
    allocators.children = create_array<DebugOverlayItem*>(g_heap);
    allocators.type     = Debug_allocators;

    auto stack = ialloc<DebugOverlayItem>(g_heap);
    stack->type  = Debug_allocator_stack;
    stack->title = "stack";
    stack->u.data  = (void*)g_stack;
    array_add(&allocators.children, stack);

    auto frame = ialloc<DebugOverlayItem>(g_heap);
    frame->type  = Debug_allocator_stack;
    frame->title = "frame";
    frame->u.data  = (void*)g_frame;
    array_add(&allocators.children, frame);

    auto persistent = ialloc<DebugOverlayItem>(g_heap);
    persistent->type  = Debug_allocator_stack;
    persistent->title = "persistent";
    persistent->u.data  = (void*)g_persistent;
    array_add(&allocators.children, persistent);

    auto debug_frame = ialloc<DebugOverlayItem>(g_heap);
    debug_frame->type  = Debug_allocator_stack;
    debug_frame->title = "debug_frame";
    debug_frame->u.data  = (void*)g_debug_frame;
    array_add(&allocators.children, debug_frame);

    auto free_list = ialloc<DebugOverlayItem>(g_heap);
    free_list->type  = Debug_allocator_free_list;
    free_list->title = "free list";
    free_list->u.data  = (void*)g_heap;
    array_add(&allocators.children, free_list);

    array_add(&g_game->overlay.items, allocators);


    DebugOverlayItem timers = {};
    timers.title = "Profile Timers";
    timers.type  = Debug_profile_timers;
    array_add(&g_game->overlay.items, timers);

    AssetID hmt = find_asset_id("terrain.bmp");
    debug_add_texture("Terrain", hmt, g_game->materials.heightmap,
                      Pipeline_basic2d, &g_game->overlay);
}

Entity* entity_find(i32 id)
{
    for (i32 i = 0; i < g_entities.count; i++) {
        if (g_entities[i].id == id) {
            return &g_entities[i];
        }
    }

    ASSERT(false);
    return nullptr;
}

void process_physics(f32 dt)
{
    PROFILE_FUNCTION();
    for (i32 i = 0; i < g_physics.velocities.count; i++) {
        Entity *e    = entity_find(g_physics.entities[i]);
        e->position += g_physics.velocities[i] * dt;
    }
}

i32 physics_add(Entity entity)
{
    array_add(&g_physics.velocities, {});

    i32 id = (i32)array_add(&g_physics.entities, entity.id);
    return id;
}

void physics_remove(Physics *physics, i32 id)
{
    array_remove(&physics->velocities,    id);
    array_remove(&physics->entities,      id);
}

i32 physics_id(i32 entity_id)
{
    // TODO(jesper): make a more performant look-up
    i32 id = -1;
    for (i32 i = 0; i < g_physics.entities.count; i++) {
        if (g_physics.entities[i] == entity_id) {
            id = i;
            break;
        }
    }

    ASSERT(id != -1);
    return id;
}

void game_init()
{
    init_profiling();

    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    g_game = ialloc<GameState>(g_persistent);

    { // cameras and coordinate bases
        f32 width = (f32)g_settings.video.resolution.width;
        f32 height = (f32)g_settings.video.resolution.height;
        f32 aspect = width / height;
        f32 vfov   = radians(45.0f);
        g_game->fp_camera.view       = Matrix4::identity();
        g_game->fp_camera.position   = Vector3{0.0f, 5.0f, 0.0f};
        g_game->fp_camera.projection = Matrix4::perspective(vfov, aspect, 0.1f, 10000.0f);
    }

    { // render objects
        init_array(&g_game->render_objects,       g_persistent, 20);
        init_array(&g_game->index_render_objects, g_persistent, 20);
    }

    init_vulkan();
    init_entity_system();
    init_catalog_system();
    init_fonts();
    init_gui();
    init_collision();

    { // update descriptor sets
        // TODO(jesper): figure out a way for this to be done automatically by
        // the asset loading system
        set_ubo(Pipeline_mesh,            ResourceSlot_mvp, &g_game->fp_camera.ubo);
        set_ubo(Pipeline_terrain,         ResourceSlot_mvp, &g_game->fp_camera.ubo);
        set_ubo(Pipeline_wireframe,       ResourceSlot_mvp, &g_game->fp_camera.ubo);
        set_ubo(Pipeline_wireframe_lines, ResourceSlot_mvp, &g_game->fp_camera.ubo);

        Texture *greybox = find_texture("greybox.bmp");
        Texture *font   = find_texture("font-regular");
        Texture *player = find_texture("player.bmp");

        set_texture(&g_game->materials.terrain, ResourceSlot_diffuse, greybox);
        set_texture(&g_game->materials.font,    ResourceSlot_diffuse, font);
        set_texture(&g_game->materials.phong,   ResourceSlot_diffuse, greybox);
        set_texture(&g_game->materials.player,  ResourceSlot_diffuse, player);
    }

    {
    }

    init_terrain();
    init_debug_overlay();

    g_game->key_state = ialloc_array<i32>(g_persistent, 256, InputType_key_release);
}

void game_quit()
{
    // NOTE(jesper): disable raw mouse as soon as possible to ungrab the cursor
    // on Linux
    platform_set_raw_mouse(false);

    vkQueueWaitIdle(g_vulkan->queue);

    destroy_fonts();
    destroy_gui();

    destroy_buffer(g_debug_collision.cube.vbo);
    destroy_buffer(g_debug_collision.sphere.vbo);
    destroy_buffer(g_debug_collision.sphere.ibo);

    for (auto &it : g_terrain.chunks) {
        destroy_buffer(it.vbo);
    }

    for (auto &item : g_game->overlay.items) {
        if (item.type == Debug_render_item) {
            destroy_buffer(item.u.ritem.vbo);
        }
    }

    for (auto &it : g_textures) {
        destroy_texture(&it);
    }

    destroy_buffer(g_game->overlay.texture.vbo);

    destroy_material(g_game->materials.terrain);
    destroy_material(g_game->materials.font);
    destroy_material(g_game->materials.heightmap);
    destroy_material(g_game->materials.phong);
    destroy_material(g_game->materials.player);

    for (auto &it : g_game->render_objects) {
        destroy_buffer(it.vbo);
    }

    for (auto &it : g_game->index_render_objects) {
        destroy_buffer(it.vbo);
        destroy_buffer(it.ibo);
    }

    destroy_buffer_ubo(g_game->fp_camera.ubo);
    destroy_vulkan();

    platform_quit();
}

void* game_pre_reload()
{
    auto state = ialloc<GameReloadState>(g_frame);

    // TODO(jesper): I feel like this could be quite nicely preprocessed and
    // generated. look into
    state->profile_timers      = g_profile_timers;
    state->profile_timers_prev = g_profile_timers_prev;

    state->texture_catalog     = g_catalog;
    state->settings            = g_settings;


    state->vulkan_device       = g_vulkan;
    state->game                = g_game;

    state->terrain = g_terrain;
    state->entities = g_entities;
    state->physics = g_physics;

    state->collision = g_collision;
    state->debug_collision = g_debug_collision;

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
    g_collision = state->collision;
    g_debug_collision = state->debug_collision;

    g_terrain  = state->terrain;
    g_entities = state->entities;
    g_physics  = state->physics;

    g_game                = state->game;

    g_profile_timers      = state->profile_timers;
    g_profile_timers_prev = state->profile_timers_prev;

    g_settings            = state->settings;
    g_catalog     = state->texture_catalog;


    g_vulkan              = state->vulkan_device;
    load_vulkan(g_vulkan->instance);

    dealloc(g_frame, state);
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
            for (auto &item : g_game->overlay.items) {
                item.collapsed = !item.collapsed;
            }
            break;
        case Key_left: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].x = 5.0f;
        } break;
        case Key_right: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].x = -5.0f;
        } break;
        case Key_up: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].z = 5.0f;
        } break;
        case Key_down: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].z = -5.0f;
        } break;
        case Key_C:
            platform_toggle_raw_mouse();
            break;
        default:
            //LOG("unhandled key press: %d", event.key.code);
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
            i32 pid = physics_id(0);
            g_physics.velocities[pid].x = 0.0f;

            if (g_game->key_state[Key_right] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_right;
                e.key.repeated = false;

                game_input(e);
            }
        } break;
        case Key_right: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].x = 0.0f;

            if (g_game->key_state[Key_left] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_left;
                e.key.repeated = false;

                game_input(e);
            }
        } break;
        case Key_up: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].z = 0.0f;

            if (g_game->key_state[Key_down] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_down;
                e.key.repeated = false;

                game_input(e);
            }
        } break;
        case Key_down: {
            i32 pid = physics_id(0);
            g_physics.velocities[pid].z = 0.0f;

            if (g_game->key_state[Key_up] == InputType_key_press) {
                InputEvent e;
                e.type         = InputType_key_press;
                e.key.code     = Key_up;
                e.key.repeated = false;

                game_input(e);
            }
        } break;
        default:
            //LOG("unhandled key release: %d", event.key.code);
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
        for (auto &item : g_game->overlay.items) {
            if (p.x >= item.tl.x && p.x <= item.br.x &&
                p.y >= item.tl.y && p.y <= item.br.y)
            {
                item.collapsed = !item.collapsed;
            }
        }
    } break;
    default:
        //LOG("unhandled input type: %d", event.type);
        break;
    }
}

void process_debug_overlay(DebugOverlay *overlay, f32 dt)
{
    PROFILE_FUNCTION();

    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };


    Vector4 fc = unpack_rgba(0x2A282ACC);

    Vector2 pos = screen_from_camera( Vector2{ -1.0f, -1.0f });
    //gui_frame({ 300.0f, 0.0f }, 200.0f, 30.0f, fc);

    isize buffer_size = 1024*1024;
    char *buffer = (char*)alloc(g_stack, buffer_size);
    buffer[0] = '\0';

    GuiFrame frame = gui_frame_begin(fc);

    f32 dt_ms = dt * 1000.0f;
    snprintf(buffer, buffer_size, "frametime: %f ms, %f fps\n",
             dt_ms, 1000.0f / dt_ms);
    gui_textbox(&frame, buffer, &pos);


    for (auto &item : overlay->items) {
        {
            f32 base_x = pos.x;
            item.tl = pos;
            defer {
                pos.y   += 20.0f;
                item.br  = pos;

                pos.x    = base_x;
            };

            if (item.collapsed) {
                snprintf(buffer, buffer_size, "%s...", item.title);
                gui_textbox(&frame, buffer, &pos);
                continue;
            }

            snprintf(buffer, buffer_size, "%s", item.title);
            gui_textbox(&frame, buffer, &pos);
        }

        switch (item.type) {
        case Debug_allocators: {
            for (int c = 0; c < item.children.count; c++) {
                DebugOverlayItem &child = *item.children[c];

                switch (child.type) {
                case Debug_allocator_stack: {
                    Allocator *a = (Allocator*)child.u.data;
                    snprintf(buffer, buffer_size,
                             "  %s: { sp: %p, size: %zd, remaining: %zd }\n",
                             child.title, a->sp, a->size, a->remaining);
                    gui_textbox(&frame, buffer, &pos);
                } break;
                case Debug_allocator_free_list: {
                    Allocator *a = (Allocator*)child.u.data;
                    snprintf(buffer, buffer_size,
                             "  %s: { size: %zd, remaining: %zd }\n",
                             child.title, a->size, a->remaining);
                    gui_textbox(&frame, buffer, &pos);
                } break;
                default:
                    LOG("unhandled case: %d", item.type);
                    break;
                }
            }
        } break;
        case Debug_profile_timers: {
            f32 margin   = 10.0f;

            f32 hy  = pos.y;
            pos.y  += 20.0f;

            f32 base_x = pos.x;
            f32 base_y = pos.y;

            Vector2 c0, c1, c2, c3;
            c0.x = c1.x = c2.x = c3.x = pos.x + margin;
            c0.y = c1.y = c2.y = c3.y = hy;

            pos.x  = c0.x;

            ProfileTimers &timers = g_profile_timers_prev;
            for (i32 i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%s: ", timers.name[i]);
                gui_textbox(&frame, buffer, &pos);

                if (pos.x >= c1.x) {
                    c1.x = pos.x;
                }

                pos.x  = c0.x;
                pos.y += 20.0f;
            }
            pos.y = base_y;

            c1.x = MAX(c0.x + 250.0f, c1.x) + margin;
            pos.x  = c1.x;
            for (i32 i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%" PRIu64, timers.cycles[i]);
                gui_textbox(&frame, buffer, &pos);

                if (pos.x >= c2.x) {
                    c2.x = pos.x;
                }

                pos.x  = c1.x;
                pos.y += 20.0f;
            }
            pos.y = base_y;

            c2.x = MAX(c1.x + 100.0f, c2.x) + margin;
            pos.x  = c2.x;
            for (i32 i = 0; i < timers.count; i++) {
                gui_textbox(&frame, buffer, &pos);

                if (pos.x >= c3.x) {
                    c3.x = pos.x;
                }

                pos.x  = c2.x;
                pos.y += 20.0f;
            }
            pos.y = base_y;

            c3.x = MAX(c2.x + 75.0f, c3.x) + margin;
            pos.x  = c3.x;
            for (int i = 0; i < timers.count; i++) {
                snprintf(buffer, buffer_size, "%f", timers.cycles[i] / (f32)timers.calls[i]);
                gui_textbox(&frame, buffer, &pos);

                pos.x  = c3.x;
                pos.y += 20.0f;
            }

            gui_textbox(&frame, "name",     &c0);
            gui_textbox(&frame, "cycles",   &c1);
            gui_textbox(&frame, "calls",    &c2);
            gui_textbox(&frame, "cy/calls", &c3);

            pos.x = base_x;
        } break;
        case Debug_render_item: {
            item.u.ritem.position = camera_from_screen(pos + Vector2{10.0f, 0.0f});

            Matrix4 t = translate(Matrix4::identity(), item.u.ritem.position);
            set_push_constant(&item.u.ritem.constants, t);

            array_add(&overlay->render_queue, item.u.ritem);
        } break;
        default:
            LOG("unknown debug menu type: %d", item.type);
            break;
        }
    }

    gui_frame_end(frame);
}

void game_update(f32 dt)
{
    PROFILE_FUNCTION();

    process_catalog_system();

    Entity &player = g_entities[0];

    if (false) {
        Quaternion r = Quaternion::make({0.0f, 1.0f, 0.0f}, 1.5f * dt);
        player.rotation = player.rotation * r;

        r = Quaternion::make({1.0f, 0.0f, 0.0f}, 0.5f * dt);
        player.rotation = player.rotation * r;
    }

    if (g_game->key_state[Key_J] == InputType_key_press)
        g_collision.spheres[0].position.x -= 1.0f * dt;

    if (g_game->key_state[Key_L] == InputType_key_press)
        g_collision.spheres[0].position.x += 1.0f * dt;

    if (g_game->key_state[Key_U] == InputType_key_press)
        g_collision.spheres[0].position.y += 1.0f * dt;

    if (g_game->key_state[Key_O] == InputType_key_press)
        g_collision.spheres[0].position.y -= 1.0f * dt;

    if (g_game->key_state[Key_K] == InputType_key_press)
        g_collision.spheres[0].position.z += 1.0f * dt;

    if (g_game->key_state[Key_I] == InputType_key_press)
        g_collision.spheres[0].position.z -= 1.0f * dt;


    process_physics(dt);
    process_collision();

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

    VulkanPipeline &pipeline = g_vulkan->pipelines[g_terrain.pipeline];

    vkCmdBindPipeline(command,
                      VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline.handle);

    auto descriptors = create_array<GfxDescriptorSet>(g_stack);
    array_add(&descriptors, pipeline.descriptor_set);
    array_add(&descriptors, g_terrain.material->descriptor_set);
    gfx_bind_descriptors(
        command,
        pipeline.layout,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        descriptors);

    for (auto &c : g_terrain.chunks) {
        vkCmdBindVertexBuffers(command, 0, 1, &c.vbo.handle, offsets);
        vkCmdDraw(command, (u32)c.vertices.count, 1, 0, 0);
    }
}

void game_render()
{
    PROFILE_FUNCTION();
    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    GfxFrame frame = gfx_begin_frame();

    VkDeviceSize offsets[] = { 0 };

    render_terrain(frame.cmd);

    for (auto &object : g_game->render_objects) {
        VulkanPipeline &pipeline = g_vulkan->pipelines[object.pipeline];

        vkCmdBindPipeline(frame.cmd,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.handle);

        // TODO(jesper): bind material descriptor set if bound
        // TODO(jesper): only bind pipeline descriptor set if one exists, might
        // be such a special case that we should hardcode it?
        gfx_bind_descriptor(
            frame.cmd,
            pipeline.layout,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.descriptor_set);

        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &object.vbo.handle, offsets);

        vkCmdPushConstants(
            frame.cmd,
            pipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(object.transform), &object.transform);

        vkCmdDraw(frame.cmd, object.vertex_count, 1, 0, 0);
    }

    auto descriptors = create_array<GfxDescriptorSet>(g_frame);
    for (auto &object : g_game->index_render_objects) {
        reset_array_count(&descriptors);

        VulkanPipeline &pipeline = g_vulkan->pipelines[object.pipeline];

        vkCmdBindPipeline(frame.cmd,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.handle);


        array_add(&descriptors, pipeline.descriptor_set);

        if (object.material) {
            array_add(&descriptors, object.material->descriptor_set);
        }

        gfx_bind_descriptors(
            frame.cmd,
            pipeline.layout,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            descriptors);

        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &object.vbo.handle, offsets);
        vkCmdBindIndexBuffer(frame.cmd, object.ibo.handle,
                             0, VK_INDEX_TYPE_UINT32);


        Entity *e = entity_find(object.entity_id);

        Matrix4 t = translate(Matrix4::identity(), e->position);
        Matrix4 r = Matrix4::make(e->rotation);

        Matrix4 srt = scale(t, e->scale);
        srt = srt * r;

        vkCmdPushConstants(
            frame.cmd,
            pipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0, sizeof(srt), &srt);

        vkCmdDrawIndexed(frame.cmd, object.index_count, 1, 0, 0, 0);
    }


    // collidables
    if (g_debug_collision.render_collidables) {
        reset_array_count(&descriptors);

        VulkanPipeline pipeline = g_vulkan->pipelines[Pipeline_wireframe_lines];

        vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.handle);

        array_add(&descriptors, pipeline.descriptor_set);
        gfx_bind_descriptors(
            frame.cmd,
            pipeline.layout,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            descriptors);

        struct {
            Matrix4 transform;
            Vector3 color;
        } pc;

        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &g_debug_collision.cube.vbo.handle, offsets);
        for (auto &c : g_collision.aabbs) {
            pc.transform = translate(Matrix4::identity(), c.position);
            pc.transform = scale(pc.transform, c.scale);

            if (c.colliding ) {
                pc.color = { 1.0f, 0.0f, 0.0f };
            } else {
                pc.color = { 0.0f, 1.0f, 0.0f };
            }

            vkCmdPushConstants(frame.cmd, pipeline.layout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(pc), &pc);
            vkCmdDraw(frame.cmd, g_debug_collision.cube.vertex_count, 1, 0, 0);
        }


        pipeline = g_vulkan->pipelines[Pipeline_wireframe];
        vkCmdBindPipeline(frame.cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.handle);

        // TODO(jesper): needed? same descriptors as last pipeline bind
        gfx_bind_descriptors(
            frame.cmd,
            pipeline.layout,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            descriptors);

        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &g_debug_collision.sphere.vbo.handle, offsets);
        vkCmdBindIndexBuffer(frame.cmd, g_debug_collision.sphere.ibo.handle, 0, VK_INDEX_TYPE_UINT32);

        for (auto &c : g_collision.spheres) {
            pc.transform = translate(Matrix4::identity(), c.position);
            pc.transform = scale(pc.transform, c.radius);

            if (c.colliding ) {
                pc.color = { 1.0f, 0.0f, 0.0f };
            } else {
                pc.color = { 0.0f, 1.0f, 0.0f };
            }

            vkCmdPushConstants(frame.cmd, pipeline.layout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(pc), &pc);
            vkCmdDrawIndexed(frame.cmd, g_debug_collision.sphere.index_count, 1, 0, 0, 0);
        }

    }

    // debug overlay items
    for (auto &item : g_game->overlay.render_queue) {
        VulkanPipeline &pipeline = g_vulkan->pipelines[item.pipeline];

        vkCmdBindPipeline(frame.cmd,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.handle);

        gfx_bind_descriptors(
            frame.cmd,
            pipeline.layout,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            item.descriptors);

        // TODO(jesper): proper push constants support
#if 0
        vkCmdPushConstants(command, pipeline.layout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           item.constants.offset,
                           (u32)item.constants.size,
                           item.constants.data);
#endif

        Matrix4 t = translate(Matrix4::identity(), item.position);
        vkCmdPushConstants(frame.cmd, pipeline.layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t), &t);

        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &item.vbo.handle, offsets);
        vkCmdDraw(frame.cmd, item.vertex_count, 1, 0, 0);
    }
    g_game->overlay.render_queue.count = 0;

    gui_render(frame.cmd);

    gfx_end_frame();
}

void game_update_and_render(f32 dt)
{
    gui_frame_start();

    game_update(dt);
    process_debug_overlay(&g_game->overlay, dt);

    game_render();

    reset(g_frame, nullptr);
    reset(g_frame, nullptr);
}
