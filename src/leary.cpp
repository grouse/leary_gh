/**
 * file:    leary.cpp
 * created: 2016-11-26
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2016-2018 - all rights reserved
 */

#include "build_config.h"

// TODO(jesper): replace with my own stuff
#include <stdint.h>
#include "core/types.h"

#include <inttypes.h>

#include <initializer_list>
#include <stdio.h>

// TODO(jesper): needed for swap, which we really don't need because none of our swaps
// benefit from move semantics, and we can just do ourselves anyway
#include <utility>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_ASSERT ASSERT
#include "external/stb/stb_truetype.h"
#include "external/alchemy/alchemy.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GlslangToSpv.h"


#if defined(__linux__)

    #define VK_USE_PLATFORM_XLIB_KHR
    #include <vulkan/vulkan.h>
#elif defined(_WIN32)
    #include <Windows.h>
    #include <shlwapi.h>
    #include <Shlobj.h>
    #include <xaudio2.h>
    #include <audioclient.h>
    #include <mmdeviceapi.h>
    #include <intrin.h>

    #define VK_USE_PLATFORM_WIN32_KHR
    #include <vulkan/vulkan.h>
#else
    #error "unsupported platform"
#endif

#include "platform/platform_debug.h"
#include "platform/thread.h"
#include "platform/platform_input.h"

// TODO(jesper): get rid of this and inline?
#include "leary_macros.h"

#include "core/log.h"
#include "core/maths.h"
#include "core/font.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_table.h"
#include "core/string.h"
#include "core/file.h"
#include "core/gfx_vulkan.h"
#include "core/assets.h"
#include "core/serialize.h"
#include "core/profiler.h"
#include "core/sound.h"
#include "core/random.h"
#include "core/gui.h"
#include "core/collision.h"

#include "leary.h"

#include "platform/platform.h"

#include "generated/type_info.h"

#if defined(__linux__)
    #include "platform/linux_leary.cpp"
#elif defined(_WIN32)
    #include "platform/win32_debug.cpp"
    #include "platform/win32_thread.cpp"
    #include "platform/win32_vulkan.cpp"
    #include "platform/win32_file.cpp"
    #include "platform/win32_input.cpp"
    #include "platform/win32_leary.cpp"
#else
    #error "unsupported platform"
#endif

#include "core/hash.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"
#include "core/hash_table.cpp"
#include "core/lexer.cpp"
#include "core/profiler.cpp"
#include "core/maths.cpp"
#include "core/random.cpp"
#include "core/assets.cpp"
#include "core/string.cpp"
#include "core/file.cpp"
#include "core/font.cpp"
#include "core/gui.cpp"
#include "core/collision.cpp"
#include "core/gfx_vulkan.cpp"
#include "core/sound.cpp"
#include "core/log.cpp"

#include "core/serialize.cpp"

struct Terrain {
    struct Chunk {
        Array<Vector3> points;
        Array<Vector3> normals;
        Array<Vector3> tangents;
        Array<Vector3> bitangents;
        Array<Vector2> uvs;

        struct {
            VulkanBuffer points;
            VulkanBuffer normals;
            VulkanBuffer tangents;
            VulkanBuffer bitangents;
            VulkanBuffer uvs;
        } vbo;
    };
    Array<Chunk> chunks;

    PipelineID     pipeline;
    Material       *material;
};

struct GameReloadState {
    Matrix4       screen_to_view;
    Matrix4       view_to_screen;

    PlatformState *platform;
    Settings      settings;
    Catalog       texture_catalog;

    VulkanDevice  *vulkan_device;
    GameState     *game;

    Terrain        terrain;
    Array<Entity>  entities;

    Collision collision;
    DebugCollision debug_collision;
};

DebugOverlay g_debug_overlay;

Array<RenderObject> g_render_queue;
Array<IndexRenderObject> g_indexed_render_queue;


// NOTE(jesper): don't keep an address to these globals!!!!
extern Settings      g_settings;
extern PlatformState *g_platform;
extern Catalog       g_catalog;

Terrain        g_terrain;

VulkanDevice *g_vulkan;
GameState    *g_game;
SoundData g_swing;

struct GuiTexture {
    StringView name;
    GfxTexture gfx_texture;
};

Array<GuiTexture> g_overlay_textures;


// TODO(jesper): this should be something like gui_icon or gui_texture API
void debug_add_texture(
    StringView name,
    GfxTexture texture,
    GfxDescriptorSet descriptor,
    PipelineID pipeline,
    DebugOverlay *overlay)
{
    Vector2 dim = Vector2{ (f32)texture.width, (f32)texture.height };
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

    VulkanBuffer vbo = create_vbo(vertices, sizeof(vertices));
    debug_add_texture(
        name,
        { (f32)texture.width, (f32)texture.height },
        vbo,
        descriptor,
        pipeline,
        overlay);
}

VulkanBuffer g_lines_vbo;
usize g_lines_vbo_offset = 0;
void *g_lines_vbo_mapped = nullptr;
i32 g_lines_vertex_count = 0;

void draw_line_segment(Vector3 origo, Vector3 vector, Vector4 color)
{
    if (g_lines_vbo_mapped == nullptr) {
        VkResult result = vkMapMemory(
            g_vulkan->handle,
            g_lines_vbo.memory,
            0, VK_WHOLE_SIZE, 0,
            &g_lines_vbo_mapped);
        ASSERT(result == VK_SUCCESS);

        g_lines_vertex_count = 0;
        g_lines_vbo_offset = 0;
    }


    struct LineVertex {
        Vector3 p0;
        Vector4 c0;
        Vector3 p1;
        Vector4 c1;
    };

    LineVertex vertex = { origo, color, origo + vector, color };

    ASSERT((g_lines_vbo_offset + sizeof vertex) < g_lines_vbo.size);
    memcpy((void*)((u8*)g_lines_vbo_mapped + g_lines_vbo_offset),
           &vertex,
           sizeof vertex);
    g_lines_vbo_offset += sizeof vertex;
    g_lines_vertex_count += 2;
}

void debug_add_texture(
    StringView name,
    Vector2 size,
    VulkanBuffer vbo,
    GfxDescriptorSet descriptor,
    PipelineID pipeline,
    DebugOverlay *overlay)
{
#if 0
    GuiTexture texture = {};
    texture.name = name;
    texture.vbo = vbo;
    texture.descriptor = descriptor;
    texture.pipeline = pipeline;

    array_add(&g_overlay_textures, texture);
#else
    DebugOverlayItem item  = {};
    item.title = name;
    item.type  = Debug_render_item;
    item.size  = size;
    item.u.ritem.pipeline  = pipeline;
    item.u.ritem.constants = create_push_constants(pipeline);

    item.u.ritem.vbo = vbo;
    item.u.ritem.vertex_count = 6;

    init_array(&item.u.ritem.descriptors, g_heap, 1);
    array_add(&item.u.ritem.descriptors, descriptor);
    array_add(&overlay->items, item);
#endif
}

void debug_add_texture(
    const char *name,
    AssetID tid,
    Material material,
    PipelineID pipeline,
    DebugOverlay *overlay)
{
    TextureAsset *texture = find_texture(tid);
    if (texture == nullptr) {
        return;
    }

    Vector2 dim = Vector2{
        (f32)texture->gfx_texture.width / g_settings.video.resolution.width,
        (f32)texture->gfx_texture.height / g_settings.video.resolution.height
    };

    DebugOverlayItem item = {};
    item.title = name;
    item.type  = Debug_render_item;

    item.size  = {
        (f32)texture->gfx_texture.width,
        (f32)texture->gfx_texture.height
    };

    item.u.ritem.pipeline   = pipeline;
    item.u.ritem.constants  = create_push_constants(pipeline);

    f32 vertices[] = {
        0.0f, 0.0f,  0.0f, 0.0f,
        0.0f, dim.y,  0.0f, 1.0f,
        dim.x, dim.y, 1.0f, 1.0f,
        
        dim.x, dim.y, 1.0f, 1.0f,
        dim.x, 0.0f, 1.0f, 0.0f,
        0.0f,  0.0f,  0.0f, 0.0f,
    };
    

    item.u.ritem.vbo = create_vbo(vertices, sizeof(vertices));
    item.u.ritem.vertex_count = 6;

    init_array(&item.u.ritem.descriptors, g_heap, 1);
    array_add(&item.u.ritem.descriptors, material.descriptor_set);
    array_add(&overlay->items, item);
}

void init_entity_system()
{
    init_array(&g_entities, g_heap);
}


void init_terrain()
{
    TextureData td = load_texture_bmp(
        resolve_file_path(GamePath_textures, "terrain.bmp", g_frame),
        g_frame);
    ASSERT(td.pixels != nullptr);

    struct Texel {
        u8 r, g, b, a;
    };

    struct Vertex {
        Vector3 p;
        Vector3 n;
        Vector3 t;
        Vector3 b;
        Vector2 uv;
    };

    u32  vc       = td.height * td.width;
    auto vertices = create_array<Vertex>(g_persistent, vc);

    // TODO(jesper): move to settings/asset info/something
    Vector3 w = { 50.0f, 5.0f, 50.0f };
    f32 inv_width  = 1.0f / td.width;
    f32 inv_height = 1.0f / td.height;
    f32 inv_depth  = 1.0f / 255.0f;

    f32 xx = w.x * 2.0f;
    f32 yy = w.y * 2.0f;
    f32 zz = w.z * 2.0f;

    f32 uv_scale = 1.0f / 12.0f;

    for (i32 i = 0; i < td.height-1; i++) {
        for (i32 j = 0; j < td.width-1; j++) {
            Texel t_tl = ((Texel*)td.pixels)[i     * td.width + j];
            Texel t_tr = ((Texel*)td.pixels)[i     * td.width + j+1];
            Texel t_br = ((Texel*)td.pixels)[(i+1) * td.width + j+1];
            Texel t_bl = ((Texel*)td.pixels)[(i+1) * td.width + j];
            
            Vector3 tl = Vector3{ (f32)i, (f32)t_tl.r, (f32)j };
            tl.x = tl.x * inv_width  * xx - w.x;
            tl.y = tl.y * inv_depth  * yy - w.y;
            tl.z = tl.z * inv_height * zz - w.z;

            Vector3 tr = Vector3{ (f32)i, (f32)t_tr.r, (f32)j+1 };
            tr.x = tr.x * inv_width  * xx - w.x;
            tr.y = tr.y * inv_depth  * yy - w.y;
            tr.z = tr.z * inv_height * zz - w.z;
            
            Vector3 br = Vector3{ (f32)i+1, (f32)t_br.r, (f32)j+1 };
            br.x = br.x * inv_width  * xx - w.x;
            br.y = br.y * inv_depth  * yy - w.y;
            br.z = br.z * inv_height * zz - w.z;
            
            Vector3 bl = Vector3{ (f32)i+1, (f32)t_bl.r, (f32)j };
            bl.x = bl.x * inv_width  * xx - w.x;
            bl.y = bl.y * inv_depth  * yy - w.y;
            bl.z = bl.z * inv_height * zz - w.z;
            

            Vector2 uv_tl = { (tl.x + w.x) * uv_scale, (tl.z + w.z) * uv_scale };
            Vector2 uv_tr = { (tr.x + w.x) * uv_scale, (tr.z + w.z) * uv_scale };
            Vector2 uv_br = { (br.x + w.x) * uv_scale, (br.z + w.z) * uv_scale };
            Vector2 uv_bl = { (bl.x + w.x) * uv_scale, (bl.z + w.z) * uv_scale };
            
            {
                Vector3 e0 = tl - tr;
                Vector3 e1 = br - tr;
                
                Vector2 duv0 = uv_tl - uv_tr;
                Vector2 duv1 = uv_br - uv_tr;
                
                Vector3 t = tangent(e0, e1, duv0, duv1);
                Vector3 b = bitangent(e0, e1, duv0, duv1);
                
                Vector3 n = normalise(cross(e0, e1));
                array_add(&vertices, { br, n, t, b, uv_br });
                array_add(&vertices, { tr, n, t, b, uv_tr });
                array_add(&vertices, { tl, n, t, b, uv_tl });
            }

            {
                Vector3 e0 = bl - br;
                Vector3 e1 = bl - tl;
                
                Vector2 duv0 = uv_bl - uv_br;
                Vector2 duv1 = uv_bl - uv_tl;
                
                Vector3 t = tangent(e0, e1, duv0, duv1);
                Vector3 b = bitangent(e0, e1, duv0, duv1);

                Vector3 n = normalise(cross(e0, e1));
                array_add(&vertices, { tl, n, t, b, uv_tl });
                array_add(&vertices, { bl, n, t, b, uv_bl });
                array_add(&vertices, { br, n, t, b, uv_br });
            }
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
        init_array(&t.chunks[i].points, g_heap);
        init_array(&t.chunks[i].normals, g_heap);
        init_array(&t.chunks[i].tangents, g_heap);
        init_array(&t.chunks[i].bitangents, g_heap);
        init_array(&t.chunks[i].uvs, g_heap);
    }

    for (i32 i = 0; i < vertices.count; i+= 3) {
        Vertex v0 = vertices[i];
        Vertex v1 = vertices[i+1];
        Vertex v2 = vertices[i+2];

        if (v0.p.x >= mid_x || v1.p.x >= mid_x || v2.p.x >= mid_x) {
            if (v0.p.z >= mid_z || v1.p.z >= mid_z || v2.p.z >= mid_z) {
                array_add(&t.chunks[0].points, v0.p);
                array_add(&t.chunks[0].points, v1.p);
                array_add(&t.chunks[0].points, v2.p);

                array_add(&t.chunks[0].normals, v0.n);
                array_add(&t.chunks[0].normals, v1.n);
                array_add(&t.chunks[0].normals, v2.n);

                array_add(&t.chunks[0].tangents, v0.t);
                array_add(&t.chunks[0].tangents, v1.t);
                array_add(&t.chunks[0].tangents, v2.t);

                array_add(&t.chunks[0].bitangents, v0.b);
                array_add(&t.chunks[0].bitangents, v1.b);
                array_add(&t.chunks[0].bitangents, v2.b);

                array_add(&t.chunks[0].uvs, v0.uv);
                array_add(&t.chunks[0].uvs, v1.uv);
                array_add(&t.chunks[0].uvs, v2.uv);
            } else {
                array_add(&t.chunks[1].points, v0.p);
                array_add(&t.chunks[1].points, v1.p);
                array_add(&t.chunks[1].points, v2.p);

                array_add(&t.chunks[1].normals, v0.n);
                array_add(&t.chunks[1].normals, v1.n);
                array_add(&t.chunks[1].normals, v2.n);

                array_add(&t.chunks[1].tangents, v0.t);
                array_add(&t.chunks[1].tangents, v1.t);
                array_add(&t.chunks[1].tangents, v2.t);

                array_add(&t.chunks[1].bitangents, v0.b);
                array_add(&t.chunks[1].bitangents, v1.b);
                array_add(&t.chunks[1].bitangents, v2.b);

                array_add(&t.chunks[1].uvs, v0.uv);
                array_add(&t.chunks[1].uvs, v1.uv);
                array_add(&t.chunks[1].uvs, v2.uv);
            }
        } else {
            if (v0.p.z >= mid_z || v1.p.z >= mid_z || v2.p.z >= mid_z) {
                array_add(&t.chunks[2].points, v0.p);
                array_add(&t.chunks[2].points, v1.p);
                array_add(&t.chunks[2].points, v2.p);

                array_add(&t.chunks[2].normals, v0.n);
                array_add(&t.chunks[2].normals, v1.n);
                array_add(&t.chunks[2].normals, v2.n);

                array_add(&t.chunks[2].tangents, v0.t);
                array_add(&t.chunks[2].tangents, v1.t);
                array_add(&t.chunks[2].tangents, v2.t);

                array_add(&t.chunks[2].bitangents, v0.b);
                array_add(&t.chunks[2].bitangents, v1.b);
                array_add(&t.chunks[2].bitangents, v2.b);

                array_add(&t.chunks[2].uvs, v0.uv);
                array_add(&t.chunks[2].uvs, v1.uv);
                array_add(&t.chunks[2].uvs, v2.uv);
            } else {
                array_add(&t.chunks[3].points, v0.p);
                array_add(&t.chunks[3].points, v1.p);
                array_add(&t.chunks[3].points, v2.p);

                array_add(&t.chunks[3].normals, v0.n);
                array_add(&t.chunks[3].normals, v1.n);
                array_add(&t.chunks[3].normals, v2.n);

                array_add(&t.chunks[3].tangents, v0.t);
                array_add(&t.chunks[3].tangents, v1.t);
                array_add(&t.chunks[3].tangents, v2.t);

                array_add(&t.chunks[3].bitangents, v0.b);
                array_add(&t.chunks[3].bitangents, v1.b);
                array_add(&t.chunks[3].bitangents, v2.b);

                array_add(&t.chunks[3].uvs, v0.uv);
                array_add(&t.chunks[3].uvs, v1.uv);
                array_add(&t.chunks[3].uvs, v2.uv);
            }
        }
    }

    t.pipeline = Pipeline_terrain;
    t.material = &g_game->materials.terrain;
    for (i32 i = 0; i < t.chunks.count; i++) {
        Terrain::Chunk &c = t.chunks[i];

        c.vbo.points     = create_vbo(c.points.data, c.points.count * sizeof c.points[0]);
        c.vbo.normals    = create_vbo(c.normals.data, c.normals.count * sizeof c.normals[0]);
        c.vbo.tangents   = create_vbo(c.tangents.data, c.tangents.count * sizeof c.tangents[0]);
        c.vbo.bitangents = create_vbo(c.bitangents.data, c.bitangents.count * sizeof c.bitangents[0]);
        c.vbo.uvs        = create_vbo(c.uvs.data, c.uvs.count * sizeof c.uvs[0]);
    }

    g_terrain = t;
    destroy_array(&vertices);

    TextureAsset *ta = find_texture("terrain.bmp");
    ASSERT(ta != nullptr);

    gfx_set_texture(
        g_game->materials.heightmap.pipeline,
        g_game->materials.heightmap.descriptor_set,
        0,
        ta->gfx_texture);
}

void init_debug_overlay()
{
    init_array(&g_overlay_textures, g_heap);

    g_game->overlay.items        = create_array<DebugOverlayItem>(g_heap);
    g_game->overlay.render_queue = create_array<DebugRenderItem>(g_heap);

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

void set_active_camera(CameraId camera)
{
    gfx_set_ubo(Pipeline_mesh,            0, &g_game->cameras[camera].ubo);
    gfx_set_ubo(Pipeline_terrain,         0, &g_game->cameras[camera].ubo);
    gfx_set_ubo(Pipeline_wireframe,       0, &g_game->cameras[camera].ubo);
    gfx_set_ubo(Pipeline_wireframe_lines, 0, &g_game->cameras[camera].ubo);
    gfx_set_ubo(Pipeline_line,            0, &g_game->cameras[camera].ubo);

    g_game->active_camera = camera;
}

void game_init()
{
    init_profiler();

    init_array(&g_render_queue, g_frame);
    init_array(&g_indexed_render_queue, g_frame);

    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    g_game = ialloc<GameState>(g_persistent);

    init_sound();
    init_vulkan();
    init_entity_system();
    init_catalog_system();

    // create materials
    {
        g_game->materials.terrain   = create_material(Pipeline_terrain, Material_phong);
        g_game->materials.font      = create_material(Pipeline_font,    Material_basic2d);
        g_game->materials.heightmap = create_material(Pipeline_basic2d, Material_basic2d);
        g_game->materials.phong     = create_material(Pipeline_mesh,    Material_phong);
        g_game->materials.player    = create_material(Pipeline_mesh,    Material_phong);
    }

    init_fonts();
    init_gui();
    init_collision();


    {
        u8 *pixels = (u8*)alloc(g_frame, 32*32*4*sizeof(u8));
        u8 *p = pixels;
        for (i32 i = 0; i < 32*32; i++) {
            *p++ = 255;
            *p++ = 0;
            *p++ = 0;
            *p++ = 255;
        }

        add_texture("dummy_nrm.bmp", 32, 32, VK_FORMAT_B8G8R8A8_SRGB, pixels, {});
    }

    g_lines_vbo = create_vbo(1024*1024);

    { // cameras and coordinate bases
        f32 width = (f32)g_settings.video.resolution.width;
        f32 height = (f32)g_settings.video.resolution.height;
        f32 aspect = width / height;
        f32 vfov   = radians(75.0f);
        g_game->cameras[CAMERA_PLAYER].projection = perspective(vfov, aspect);
        g_game->cameras[CAMERA_PLAYER].ubo = create_ubo(sizeof(Matrix4));

        g_game->cameras[CAMERA_DEBUG].projection = perspective(vfov, aspect);
        g_game->cameras[CAMERA_DEBUG].ubo = create_ubo(sizeof(Matrix4));
    }

    set_active_camera(CAMERA_PLAYER);

    { // update descriptor sets
        // TODO(jesper): figure out a way for this to be done automatically by
        // the asset loading system

        TextureAsset *greybox = find_texture("greybox.bmp");
        TextureAsset *font   = find_texture("font-regular");
        TextureAsset *player = find_texture("player.bmp");
        TextureAsset *grass = find_texture("grass_col.bmp");
        TextureAsset *grass_nrm = find_texture("grass_nrm.bmp");
        TextureAsset *stone = find_texture("cobble_col.bmp");
        TextureAsset *stone_nrm = find_texture("cobble_nrm.bmp");
        TextureAsset *terrain_map = find_texture("terrain_texture_map.bmp");
        TextureAsset *dummy_nrm = find_texture("dummy_nrm.bmp");

        // TODO(jesper): better error handling
        ASSERT(greybox != nullptr);
        ASSERT(font != nullptr);
        ASSERT(player != nullptr);
        ASSERT(grass != nullptr);
        ASSERT(grass_nrm != nullptr);
        ASSERT(stone != nullptr);
        ASSERT(stone_nrm != nullptr);
        ASSERT(terrain_map != nullptr);
        ASSERT(dummy_nrm != nullptr);

        gfx_set_texture(
            g_game->materials.terrain.pipeline,
            g_game->materials.terrain.descriptor_set,
            0,
            grass->gfx_texture);

        gfx_set_texture(
            g_game->materials.terrain.pipeline,
            g_game->materials.terrain.descriptor_set,
            1,
            grass_nrm->gfx_texture);

        gfx_set_texture(
            g_game->materials.terrain.pipeline,
            g_game->materials.terrain.descriptor_set,
            2,
            stone->gfx_texture);

        gfx_set_texture(
            g_game->materials.terrain.pipeline,
            g_game->materials.terrain.descriptor_set,
            3,
            stone_nrm->gfx_texture);

        gfx_set_texture(
            g_game->materials.terrain.pipeline,
            g_game->materials.terrain.descriptor_set,
            4,
            terrain_map->gfx_texture);

        gfx_set_texture(
            g_game->materials.font.pipeline,
            g_game->materials.font.descriptor_set,
            0,
            font->gfx_texture);

        gfx_set_texture(
            g_game->materials.phong.pipeline,
            g_game->materials.phong.descriptor_set,
            0,
            greybox->gfx_texture);

        gfx_set_texture(
            g_game->materials.phong.pipeline,
            g_game->materials.phong.descriptor_set,
            1,
            dummy_nrm->gfx_texture);

        gfx_set_texture(
            g_game->materials.player.pipeline,
            g_game->materials.player.descriptor_set,
            0,
            player->gfx_texture);
    }

    init_terrain();
    init_debug_overlay();
    
    ASSERT(ray_vs_sphere({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 3.0f }, 1.0f));
    ASSERT(ray_vs_sphere({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, -3.0f }, 1.0f));
    ASSERT(ray_vs_sphere({ 0.0f, 3.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 3.0f, 3.0f }, 1.0f));
    
    
    g_game->key_state = ialloc_array<i32>(g_persistent, 256, InputType_key_release);

    init_profiler_gui();

    SoundData sound = load_sound_wav(
        resolve_file_path(GamePath_data, "audio/wind1.wav", g_frame),
        g_heap);
    play_looping_sound(sound);

    g_swing = load_sound_wav(
        resolve_file_path(GamePath_data, "audio/swing.wav", g_frame),
        g_heap);
}

void game_quit()
{
    // NOTE(jesper): disable raw mouse as soon as possible to ungrab the cursor
    // on Linux
    platform_set_raw_mouse(false);

    gfx_flush_and_wait();

    destroy_fonts();
    destroy_gui();

    for (auto &it : g_textures) {
        gfx_destroy_texture(it.gfx_texture);
    }

    destroy_vulkan();
    platform_quit();
}

void* game_pre_reload()
{
    auto state = ialloc<GameReloadState>(g_frame);

    // TODO(jesper): I feel like this could be quite nicely preprocessed and
    // generated. look into
    state->texture_catalog = g_catalog;
    state->settings        = g_settings;
    state->vulkan_device   = g_vulkan;
    state->game            = g_game;
    state->terrain         = g_terrain;
    state->entities        = g_entities;
    state->collision       = g_collision;
    state->debug_collision = g_debug_collision;

    // NOTE(jesper): wait for the vulkan queues to be idle. Here for when I get
    // to shader and resource reloading - I don't even want to think about what
    // kind of fits graphics drivers will throw if we start recreating pipelines
    // in the middle of things
    gfx_flush_and_wait();

    return (void*)state;
}

void game_reload(void *game_state)
{
    // TODO(jesper): I feel like this could be quite nicely preprocessed and
    // generated. look into
    auto state = (GameReloadState*)game_state;

    g_collision       = state->collision;
    g_debug_collision = state->debug_collision;
    g_terrain         = state->terrain;
    g_entities        = state->entities;
    g_game            = state->game;
    g_settings        = state->settings;
    g_catalog         = state->texture_catalog;
    g_vulkan          = state->vulkan_device;

    load_vulkan(g_vulkan->instance);

    dealloc(g_frame, state);
}

struct Player {
    Vector3 movement = {};
    f32 speed = 10.0f;
};

Player g_player;

void game_input(InputEvent event)
{
    // TODO(jesper): only pass event to gui if it's not handled by the game?
    gui_input(event);
    
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
            case Key_F2:
                if (g_game->active_camera == CAMERA_PLAYER) {
                    set_active_camera(CAMERA_DEBUG);
                } else {
                    set_active_camera(CAMERA_PLAYER);
                }
                break;
            case Key_W:
                g_player.movement.z = -1.0f;
                break;
            case Key_S:
                g_player.movement.z = 1.0f;
                break;
            case Key_A:
                g_player.movement.x = -1.0f;
                break;
            case Key_D:
                g_player.movement.x = 1.0f;
                break;
            case Key_Q:
                g_player.movement.y = 1.0f;
                break;
            case Key_E:
                g_player.movement.y = -1.0f;
                break;
            case Key_up:
                // TODO(jesper): tweak movement speed when we have a sense of scale
                g_game->velocity.z = 3.0f;
                break;
            case Key_down:
                // TODO(jesper): tweak movement speed when we have a sense of scale
                g_game->velocity.z = -3.0f;
                break;
            case Key_left:
                // TODO(jesper): tweak movement speed when we have a sense of scale
                g_game->velocity.x = -3.0f;
                break;
            case Key_right:
                // TODO(jesper): tweak movement speed when we have a sense of scale
                g_game->velocity.x = 3.0f;
                break;
            case Key_R:
                play_sound(g_swing);
                break;
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
            case Key_up:
                g_game->velocity.z = 0.0f;
                if (g_game->key_state[Key_down] == InputType_key_press) {
                    g_game->velocity.z = -3.0f;
                }
                break;
            case Key_down:
                g_game->velocity.z = 0.0f;
                if (g_game->key_state[Key_up] == InputType_key_press) {
                    g_game->velocity.z = 3.0f;
                }
                break;
            case Key_left:
                g_game->velocity.x = 0.0f;
                if (g_game->key_state[Key_right] == InputType_key_press) {
                    g_game->velocity.x = 3.0f;
                }
                break;
            case Key_right:
                g_game->velocity.x = 0.0f;
                if (g_game->key_state[Key_left] == InputType_key_press) {
                    g_game->velocity.x = -3.0f;
                }
                break;
            case Key_W:
                g_player.movement.z = 0.0f;
                if (g_game->key_state[Key_S] == InputType_key_press) {
                    g_player.movement.z = 1.0f;
                }
                break;
            case Key_S:
                g_player.movement.z = 0.0f;
                if (g_game->key_state[Key_W] == InputType_key_press) {
                    g_player.movement.z = -1.0f;
                }
                break;
            case Key_A:
                g_player.movement.x = 0.0f;
                if (g_game->key_state[Key_D] == InputType_key_press) {
                    g_player.movement.x = 1.0f;
                }
                break;
            case Key_D:
                g_player.movement.x = 0.0f;
                if (g_game->key_state[Key_A] == InputType_key_press) {
                    g_player.movement.x = -1.0f;
                }
                break;
            case Key_Q:
                g_player.movement.y = 0.0f;
                if (g_game->key_state[Key_E] == InputType_key_press) {
                    g_player.movement.y = -1.0f;
                }
                break;
            case Key_E:
                g_player.movement.y = 0.0f;
                if (g_game->key_state[Key_Q] == InputType_key_press) {
                    g_player.movement.y = 1.0f;
                }
                break;
            default:
                //LOG("unhandled key release: %d", event.key.code);
                break;
            }
        } break;
    case InputType_mouse_move: {
            Camera *camera = &g_game->cameras[g_game->active_camera];
            
            Matrix4 rm = matrix4(camera->rotation);
            Vector3 camera_x = { rm[0].x, rm[1].x, rm[2].x };
            Vector3 camera_y = { rm[0].y, rm[1].y, rm[2].y };
            
            //if (g_game->active_camera == CAMERA_DEBUG)
            {
                if (g_game->middle_mouse_pressed) {
                    // TODO(jesper): move mouse sensitivity into settings
                    // TODO(jesper): this should probably be some kind of pixels to
                    // meters conversion?
                    f32 x = 0.1f * event.mouse.dx;
                    f32 y = -0.1f * event.mouse.dy;
                    
                    camera->position += camera_x * x + camera_y * y;
                } else if (g_game->right_mouse_pressed) {
                    // TODO(jesper): this code is fucked. figure it out future jesper
                    f32 yaw   = 0.01f * event.mouse.dx;
                    f32 pitch = -0.01f * event.mouse.dy;
                    
                    Quaternion dpitch = Quaternion::make(camera_x, pitch);
                    Quaternion dyaw   = Quaternion::make({0.0f, 1.0f, 0.0f}, yaw);
                    Quaternion drot   = normalise(dpitch * dyaw);
                    
                    camera->rotation = normalise(camera->rotation * drot);
                }
            }
        } break;
    case InputType_mouse_wheel: {
            //if (g_game->active_camera == CAMERA_DEBUG)
            {
                Camera *camera = &g_game->cameras[g_game->active_camera];
                
                Matrix4 rm = matrix4(camera->rotation);
                Vector3 camera_z = { rm[0].z, rm[1].z, rm[2].z };
                
                camera->position += camera_z * 0.01f * -event.mouse_wheel;
            }
        } break;
    case InputType_middle_mouse_press:
        // TODO(jesper): I don't like this
        g_game->middle_mouse_pressed = true;
        break;
    case InputType_middle_mouse_release:
        // TODO(jesper): I don't like this
        g_game->middle_mouse_pressed = false;
        break;
    case InputType_right_mouse_press:
        // TODO(jesper): I don't like this
        g_game->right_mouse_pressed = true;
        break;
    case InputType_right_mouse_release:
        // TODO(jesper): I don't like this
        g_game->right_mouse_pressed = false;
        break;
    default:
        //LOG("unhandled input type: %d", event.type);
        break;
    }
}

void process_debug_overlay(DebugOverlay *overlay, f32 dt)
{
    (void)overlay;
    PROFILE_FUNCTION();

    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    GuiTextbox textbox;

    f32 margin  = 10.0f;
    Vector4 fg  = linear_from_sRGB({ 1.0f, 1.0f, 1.0f, 1.0f });
    Vector4 hl  = linear_from_sRGB(unpack_rgba(0xffff30ff));
    Vector4 bg  = linear_from_sRGB(unpack_rgba(0x2A282ACC));
    Vector4 bg_tooltip = linear_from_sRGB(unpack_rgba(0x000000FF));

    Vector2 pos = screen_from_camera( Vector2{ -1.0f, -1.0f });

    isize buffer_size = 1024*1024;
    char *buffer = (char*)alloc(g_stack, buffer_size);
    buffer[0] = '\0';

    GuiFrame frame = gui_frame_begin(bg);

    f32 dt_ms = dt * 1000.0f;
    snprintf(buffer, buffer_size, "cpu time: %f ms, %f fps",
             dt_ms, 1000.0f / dt_ms);
    gui_textbox(&frame, buffer, fg, &pos);

    snprintf(buffer, buffer_size, "gpu time: %f ms, %f fps",
             g_vulkan->gpu_time, 1000.0f / g_vulkan->gpu_time);
    gui_textbox(&frame, buffer, fg, &pos);
    
    EntityID player_id = find_entity_id("player.ent");
    Entity player = g_entities[player_id];
    
    snprintf(buffer, buffer_size, "player: %f, %f, %f",
             player.position.x, player.position.y, player.position.z);
    gui_textbox(&frame, buffer, fg, hl, &pos);
    
    Camera camera = g_game->cameras[CAMERA_DEBUG];
    snprintf(buffer, buffer_size, "debug camera: %f, %f, %f",
             camera.position.x, camera.position.y, camera.position.z);
    gui_textbox(&frame, buffer, fg, hl, &pos);
    
    camera = g_game->cameras[CAMERA_PLAYER];
    snprintf(buffer, buffer_size, "player camera: %f, %f, %f",
             camera.position.x, camera.position.y, camera.position.z);
    gui_textbox(&frame, buffer, fg, hl, &pos);
    
    
    

    textbox = gui_textbox(&frame, "Profiler", fg, hl, &pos);
    if (is_pressed(textbox)) {
        g_debug_overlay.show_profiler = !g_debug_overlay.show_profiler;
    }

    if (g_debug_overlay.show_profiler) {
        PROFILE_SCOPE(profiler_gui);

        f32 base_x = pos.x;
        f32 base_y = pos.y + 20.0f;

        Vector2 c0, c1, c2;
        c0.x = c1.x = c2.x = pos.x + margin;
        c0.y = c1.y = c2.y = pos.y;

        pos.x = c0.x;
        pos.y = base_y;

        Array<ProfileTimer> &timers = g_profile_timers;
        for (i32 i = 0; i < timers.count; i++) {
            snprintf(buffer, buffer_size, "%s: ", timers[i].name);
            GuiTextbox tb = gui_textbox(&frame, buffer, fg, &pos);

            if (is_mouse_over(tb)) {
                snprintf(buffer, buffer_size, "%s:%d", timers[i].file, timers[i].line);
                gui_tooltip(buffer, fg, bg_tooltip);
            }

            c1.x = max(c1.x, tb.size.x);
        }
        c1.x  = max(c0.x + 250.0f, c1.x) + margin;

        pos.x = c1.x;
        pos.y = base_y;

        for (i32 i = 0; i < timers.count; i++) {
            snprintf(buffer, buffer_size, "%" PRIu64, timers[i].duration);
            GuiTextbox tb = gui_textbox(&frame, buffer, fg, &pos);

            c2.x = max(c2.x, tb.size.x);
        }
        c2.x = max(c1.x + 250.0f, c2.x) + margin;

        pos.x = c2.x;
        pos.y = base_y;

        for (i32 i = 0; i < timers.count; i++) {
            snprintf(buffer, buffer_size, "%" PRIu64, timers[i].calls);
            gui_textbox(&frame, buffer, fg, &pos);
        }

        gui_textbox(&frame, "name",          fg, &c0);
        gui_textbox(&frame, "duration (cy)", fg, &c1);
        gui_textbox(&frame, "calls (#)",     fg, &c2);

        pos.x = base_x;
    }

    textbox = gui_textbox(&frame, "Allocators", fg, hl, &pos);
    if (is_pressed(textbox)) {
        g_debug_overlay.show_allocators = !g_debug_overlay.show_allocators;
    }

    if (g_debug_overlay.show_allocators) {
        f32 base_x = pos.x;
        pos.x += margin;

        snprintf(buffer, buffer_size,
                 "g_stack: { sp: %p, size: %zd, remaining: %zd }",
                 g_stack->sp, g_stack->size, g_stack->remaining);
        gui_textbox(&frame, buffer, fg, &pos);

        snprintf(buffer, buffer_size,
                 "g_frame: { sp: %p, size: %zd, remaining: %zd }",
                 g_frame->sp, g_frame->size, g_frame->remaining);
        gui_textbox(&frame, buffer, fg, &pos);

        snprintf(buffer, buffer_size,
                 "g_debug_frame: { sp: %p, size: %zd, remaining: %zd }",
                 g_debug_frame->sp, g_debug_frame->size, g_debug_frame->remaining);
        gui_textbox(&frame, buffer, fg, &pos);

        snprintf(buffer, buffer_size,
                 "g_persistent: { sp: %p, size: %zd, remaining: %zd }",
                 g_persistent->sp, g_persistent->size, g_persistent->remaining);
        gui_textbox(&frame, buffer, fg, &pos);

        snprintf(buffer, buffer_size,
                 "g_heap: { size: %zd, remaining: %zd }",
                 g_heap->size, g_heap->remaining);
        gui_textbox(&frame, buffer, fg, &pos);

        pos.x = base_x;
    }


    for (auto &item : overlay->items) {
        snprintf(buffer, buffer_size, "%s", item.title.bytes);
        GuiTextbox tb = gui_textbox(&frame, buffer, fg, hl, &pos);

        if (is_pressed(tb)) {
            item.collapsed = !item.collapsed;
        }

        if (item.collapsed) {
            continue;
        }

        switch (item.type) {
        case Debug_render_item: {
            GuiRenderItem ritem;
            ritem.position     = item.u.ritem.position;
            ritem.vbo          = item.u.ritem.vbo;
            ritem.vbo_offset   = 0;
            ritem.vertex_count = item.u.ritem.vertex_count;
            ritem.descriptors  = item.u.ritem.descriptors;
            ritem.pipeline_id  = item.u.ritem.pipeline;

            Matrix4 t = translate(matrix4_identity(), camera_from_screen(pos));

            ritem.constants.offset = 0;
            ritem.constants.size   = sizeof t;
            ritem.constants.data   = alloc(g_frame, ritem.constants.size);
            memcpy(ritem.constants.data, &t, sizeof t);

            array_add(&g_gui_render_queue, ritem);
        } break;
        default:
            LOG("unknown debug menu type: %d", item.type);
            break;
        }

        frame.width  = max(frame.width,  pos.x + item.size.x);
        frame.height = max(frame.height, pos.y + item.size.y);
        pos.y += item.size.y / 2.0f;
    }

    gui_frame_end(frame);
}

void game_update(f32 dt)
{
    PROFILE_FUNCTION();

    EntityID nrm_test = find_entity_id("nrm_test.ent");
    if (nrm_test != ASSET_INVALID_ID) {
        Entity &e = g_entities[nrm_test.id];

        Quaternion r = Quaternion::make({0.0f, 0.0f, 1.0f}, 1.5f * dt);
        e.rotation = e.rotation * r;
    }

    EntityID player_id = find_entity_id("player.ent");
    Entity &player = g_entities[player_id];

    if (g_game->active_camera == CAMERA_DEBUG) {
        Camera *player_camera = &g_game->cameras[CAMERA_PLAYER];

        Matrix4 player_rm = matrix4(player_camera->rotation);
        Vector3 player_camera_x = { player_rm[0].x, player_rm[1].x, player_rm[2].x };
        Vector3 player_camera_y = { player_rm[0].y, player_rm[1].y, player_rm[2].y };
        Vector3 player_camera_z = { player_rm[0].z, player_rm[1].z, player_rm[2].z };

        Vector3 position = player.position + player_camera->position;
        draw_line_segment(position, player_camera_x, { 1.0f, 0.0f, 0.0f, 1.0f });
        draw_line_segment(position, player_camera_y, { 0.0f, 1.0f, 0.0f, 1.0f });
        draw_line_segment(position, player_camera_z, { 0.0f, 0.0f, 1.0f, 1.0f });

        Camera *debug_camera = &g_game->cameras[CAMERA_DEBUG];

        Matrix4 debug_rm = matrix4(g_game->cameras[CAMERA_DEBUG].rotation);
        Vector3 debug_camera_z = { debug_rm[0].z, debug_rm[1].z, debug_rm[2].z };

        Vector3 world_x = { 1.0f, 0.0f, 0.0f };
        Vector3 world_y = { 0.0f, 1.0f, 0.0f };
        Vector3 world_z = { 0.0f, 0.0f, 1.0f };

        position = debug_camera->position + debug_camera_z * -2.0f;
        draw_line_segment(position, world_x, { 1.0f, 0.0f, 0.0f, 1.0f });
        draw_line_segment(position, world_y, { 0.0f, 1.0f, 0.0f, 1.0f });
        draw_line_segment(position, world_z, { 0.0f, 0.0f, 1.0f, 1.0f });
    }

    process_catalog_system();


    if (false) {
        Quaternion r = Quaternion::make({0.0f, 1.0f, 0.0f}, 1.5f * dt);
        player.rotation = player.rotation * r;

        r = Quaternion::make({1.0f, 0.0f, 0.0f}, 0.5f * dt);
        player.rotation = player.rotation * r;
    }

    {
        Matrix4 rm = matrix4(g_game->cameras[CAMERA_PLAYER].rotation);
        Vector3 camera_x = { rm[0].x, rm[1].x, rm[2].x };
        Vector3 camera_y = { rm[0].y, rm[1].y, rm[2].y };
        Vector3 camera_z = { rm[0].z, rm[1].z, rm[2].z };

        Vector3 player_movement =
            g_player.movement.x * camera_x +
            g_player.movement.y * camera_y +
            g_player.movement.z * camera_z;
        //player_movement.y = 0.0f;
        if (length_sq(player_movement) > 0.0f) {
            player_movement = normalise(player_movement);
            player.position += dt * player_movement * g_player.speed;
        }
    }

    {
        Camera *camera = &g_game->cameras[g_game->active_camera];

        Vector3 camera_p = camera->position;
        if (g_game->active_camera == CAMERA_PLAYER) {
            camera_p += player.position;
        }

        Matrix4 rm = matrix4(camera->rotation);
        Matrix4 tm = translate(matrix4_identity(), -camera_p);
        Matrix4 view = rm * tm;

        Matrix4 vp = camera->projection * view;
        buffer_data(camera->ubo, &vp, 0, sizeof vp);
    }

    if (g_game->key_state[Key_J] == InputType_key_press)
        g_collision.spheres[0].position.x -= 1.0f * dt;

    if (g_game->key_state[Key_L] == InputType_key_press)
        g_collision.spheres[0].position.x += 1.0f * dt;

    if (g_game->key_state[Key_U] == InputType_key_press)
        g_collision.spheres[0].position.y -= 1.0f * dt;

    if (g_game->key_state[Key_O] == InputType_key_press)
        g_collision.spheres[0].position.y += 1.0f * dt;

    if (g_game->key_state[Key_K] == InputType_key_press)
        g_collision.spheres[0].position.z += 1.0f * dt;

    if (g_game->key_state[Key_I] == InputType_key_press)
        g_collision.spheres[0].position.z -= 1.0f * dt;

    process_collision();
}

void render_terrain(VkCommandBuffer command)
{

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
        VkBuffer vertex_buffers[] = {
            c.vbo.points.handle,
            c.vbo.normals.handle,
            c.vbo.tangents.handle,
            c.vbo.bitangents.handle,
            c.vbo.uvs.handle
        };

        VkDeviceSize offsets[] = { 0, 0, 0, 0, 0 };

        static_assert(ARRAY_SIZE(vertex_buffers) == ARRAY_SIZE(offsets));

        vkCmdBindVertexBuffers(
            command,
            0,
            ARRAY_SIZE(vertex_buffers),
            vertex_buffers,
            offsets);

        vkCmdDraw(command, (u32)c.points.count, 1, 0, 0);
    }
}

void game_render()
{
    PROFILE_FUNCTION();
    void *sp = g_stack->sp;
    defer { reset(g_stack, sp); };

    GfxFrame frame = gfx_begin_frame();

    render_terrain(frame.cmd);

    auto descriptors = create_array<GfxDescriptorSet>(g_frame);

    if (g_lines_vbo_mapped != nullptr) {
        vkUnmapMemory(g_vulkan->handle, g_lines_vbo.memory);
        g_lines_vbo_mapped = nullptr;

        VulkanPipeline &pipeline = g_vulkan->pipelines[Pipeline_line];

        vkCmdBindPipeline(
            frame.cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.handle);

        array_add(&descriptors, pipeline.descriptor_set);

        gfx_bind_descriptors(
            frame.cmd,
            pipeline.layout,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            descriptors);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &g_lines_vbo.handle, offsets);
        vkCmdDraw(frame.cmd, g_lines_vertex_count, 1, 0, 0);
    }

    for (i32 i = 0; i < g_entities.count; i++) {
        Entity &e = g_entities[i];

        if (e.mesh_id != ASSET_INVALID_ID) {
            reset_array_count(&descriptors);

            Mesh *mesh = find_mesh(e.mesh_id);
            ASSERT(mesh != nullptr);

            VulkanPipeline &pipeline = g_vulkan->pipelines[Pipeline_mesh];

            vkCmdBindPipeline(
                frame.cmd,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.handle);

            array_add(&descriptors, pipeline.descriptor_set);
            // TODO(jesper): move this into entity
            //array_add(&descriptors, g_game->materials.phong.descriptor_set);
            array_add(&descriptors, e.descriptor_set);

            gfx_bind_descriptors(
                frame.cmd,
                pipeline.layout,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                descriptors);

            VkBuffer vertex_buffers[] = {
                mesh->vbo.points.handle,
                mesh->vbo.normals.handle,
                mesh->vbo.tangents.handle,
                mesh->vbo.bitangents.handle,
                mesh->vbo.uvs.handle
            };

            VkDeviceSize offsets[] = { 0, 0, 0, 0, 0 };

            static_assert(ARRAY_SIZE(vertex_buffers) == ARRAY_SIZE(offsets));

            vkCmdBindVertexBuffers(
                frame.cmd,
                0,
                ARRAY_SIZE(vertex_buffers),
                vertex_buffers,
                offsets);

            Matrix4 t = translate(matrix4_identity(), e.position);
            Matrix4 r = matrix4(e.rotation);

            Matrix4 srt = scale(t, e.scale);
            srt = srt * r;

            vkCmdPushConstants(
                frame.cmd,
                pipeline.layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(srt), &srt);

            if (mesh->ibo.handle != VK_NULL_HANDLE) {
                vkCmdBindIndexBuffer(
                    frame.cmd,
                    mesh->ibo.handle,
                    0,
                    VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(frame.cmd, mesh->element_count, 1, 0, 0, 0);

            } else {
                vkCmdDraw(frame.cmd, mesh->element_count, 1, 0, 0);
            }
        }
    }

    for (auto &object : g_render_queue) {
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

        VkBuffer vertex_buffers[] = {
            object.vbo.points.handle,
            object.vbo.normals.handle,
            object.vbo.tangents.handle,
            object.vbo.bitangents.handle,
            object.vbo.uvs.handle
        };

        VkDeviceSize offsets[] = { 0, 0, 0, 0, 0 };

        static_assert(ARRAY_SIZE(vertex_buffers) == ARRAY_SIZE(offsets));

        vkCmdBindVertexBuffers(
            frame.cmd,
            0,
            ARRAY_SIZE(vertex_buffers),
            vertex_buffers,
            offsets);

        if (object.entity_id != -1) {
            Entity *e = entity_find(object.entity_id);

            Matrix4 t = translate(matrix4_identity(), e->position);
            Matrix4 r = matrix4(e->rotation);

            Matrix4 srt = scale(t, e->scale);
            srt = srt * r;

            vkCmdPushConstants(
                frame.cmd,
                pipeline.layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(srt), &srt);
        } else {
            vkCmdPushConstants(
                frame.cmd,
                pipeline.layout,
                VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(object.transform), &object.transform);
        }

        vkCmdDraw(frame.cmd, object.vertex_count, 1, 0, 0);
    }

    for (auto &object : g_indexed_render_queue) {
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

        VkBuffer vertex_buffers[] = {
            object.vbo.points.handle,
            object.vbo.normals.handle,
            object.vbo.tangents.handle,
            object.vbo.bitangents.handle,
            object.vbo.uvs.handle
        };

        VkDeviceSize offsets[] = { 0, 0, 0, 0, 0 };

        static_assert(ARRAY_SIZE(vertex_buffers) == ARRAY_SIZE(offsets));

        vkCmdBindVertexBuffers(
            frame.cmd,
            0,
            ARRAY_SIZE(vertex_buffers),
            vertex_buffers,
            offsets);

        vkCmdBindIndexBuffer(frame.cmd, object.ibo.handle,
                             0, VK_INDEX_TYPE_UINT32);


        Entity *e = entity_find(object.entity_id);

        Matrix4 t = translate(matrix4_identity(), e->position);
        Matrix4 r = matrix4(e->rotation);

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

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &g_debug_collision.cube.vbo.handle, offsets);
        for (auto &c : g_collision.aabbs) {
            pc.transform = translate(matrix4_identity(), c.position);
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
            pc.transform = translate(matrix4_identity(), c.position);
            pc.transform = scale(pc.transform, c.radius);

            if (c.colliding ) {
                pc.color = { 1.0f, 0.0f, 0.0f };
            } else {
                pc.color = { 0.0f, 1.0f, 0.0f };
            }

            vkCmdPushConstants(frame.cmd, pipeline.layout,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(pc), &pc);
            vkCmdDrawIndexed(frame.cmd, g_debug_collision.sphere.vertex_count, 1, 0, 0, 0);
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

        Matrix4 t = translate(matrix4_identity(), item.position);
        vkCmdPushConstants(frame.cmd, pipeline.layout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t), &t);

        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(frame.cmd, 0, 1, &item.vbo.handle, offsets);
        vkCmdDraw(frame.cmd, item.vertex_count, 1, 0, 0);
    }
    g_game->overlay.render_queue.count = 0;

    gui_render(frame.cmd);

    gfx_end_frame();
}

void game_begin_frame()
{
    profiler_begin_frame();
    gui_begin_frame();
}

void game_update_and_render(f32 dt)
{
    init_array(&g_render_queue, g_frame);
    init_array(&g_indexed_render_queue, g_frame);

    game_update(dt);
    process_debug_overlay(&g_game->overlay, dt);

    game_render();

    reset(g_frame, nullptr);
    reset(g_debug_frame, nullptr);
}
