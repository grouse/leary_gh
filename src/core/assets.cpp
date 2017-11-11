/**
 * file:    assets.cpp
 * created: 2017-04-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "vulkan_render.h"


struct Vertex {
    Vector3 p;
    Vector3 n;
    Vector2 uv;
};

bool operator == (Vertex &lhs, Vertex &rhs)
{
    return memcmp(&lhs, &rhs, sizeof(Vertex)) == 0;
}

struct Mesh {
    Array<Vertex> vertices;
    Array<u32>    indices;
};

extern Array<Entity> g_entities;

Array<Texture> g_textures;
Catalog        g_catalog;

// NOTE(jesper): only Microsoft BMP version 3 is supported
PACKED(struct BitmapFileHeader {
    u16 type;
    u32 size;
    u16 reserved0;
    u16 reserved1;
    u32 offset;
});

PACKED(struct BitmapHeader {
    u32 header_size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bpp;
    u32 compression;
    u32 bmp_size;
    i32 res_horiz;
    i32 res_vert;
    u32 colors_used;
    u32 colors_important;
});


Texture load_texture_bmp(const char *path)
{
    // TODO(jesper): make this path work with String struct
    Texture texture = {};

    usize size;
    char *file = read_file(path, &size, g_frame);
    if (file == nullptr) {
        DEBUG_LOG("unable to read file: %s", path);
        return texture;
    }

    DEBUG_LOG("Loading bmp: %s", path);
    DEBUG_LOG("-- file size: %llu bytes", size);


    assert(file[0] == 'B' && file[1] == 'M');

    char *ptr = file;
    BitmapFileHeader *fh = (BitmapFileHeader*)ptr;

    if (fh->type != 0x4d42) {
        // TODO(jesper): support other bmp versions
        DEBUG_UNIMPLEMENTED();
        return Texture{};
    }

    ptr += sizeof(BitmapFileHeader);

    BitmapHeader *h = (BitmapHeader*)ptr;

    if (h->header_size != 40) {
        // TODO(jesper): support other bmp versions
        DEBUG_UNIMPLEMENTED();
        return Texture{};
    }

    if (h->header_size == 40) {
        DEBUG_LOG("-- version 3");
    }

    assert(h->header_size == 40);
    ptr += sizeof(BitmapHeader);

    if (h->compression != 0) {
        // TODO(jesper): support compression
        DEBUG_UNIMPLEMENTED();
        return Texture{};
    }

    if (h->compression == 0) {
        DEBUG_LOG("-- uncompressed");
    }


    if (h->colors_used == 0 && h->bpp < 16) {
        h->colors_used = 1 << h->bpp;
    }

    if (h->colors_important == 0) {
        h->colors_important = h->colors_used;
    }

    bool flip = true;
    if (h->height < 0) {
        flip = false;
        h->height = -h->height;
    }

    if (flip) {
        DEBUG_LOG("-- bottom-up");
    }

    // NOTE(jesper): bmp's with bbp > 16 doesn't have a color palette
    // NOTE(jesper): and other bbps are untested atm
    if (h->bpp != 24) {
        // TODO(jesper): only 24 bpp is tested and supported
        DEBUG_UNIMPLEMENTED();
        return Texture{};
    }

    u8 channels = 4;

    texture.format = VK_FORMAT_B8G8R8A8_UNORM;
    texture.width  = h->width;
    texture.height = h->height;
    texture.size   = h->width * h->height * channels;
    texture.data   = malloc(texture.size);

    bool alpha = false;

    u8 *src = (u8*)ptr;
    u8 *dst = (u8*)texture.data;

    for (i32 i = 0; i < h->height; i++) {
        for (i32 j = 0; j < h->width; j++) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;

            if (alpha) {
                *dst++ = *src++;
            } else {
                *dst++ = 255;
            }
        }
    }

    // NOTE(jesper): flip bottom-up textures
    if (flip) {
        dst = (u8*)texture.data;
        for (i32 i = 0; i < h->height >> 1; i++) {
            u8 *p1 = &dst[i * h->width * channels];
            u8 *p2 = &dst[(h->height - 1 - i) * h->width * channels];
            for (i32 j = 0; j < h->width * channels; j++) {
                u8 tmp = p1[j];
                p1[j] = p2[j];
                p2[j] = tmp;
            }
        }
    }

    return texture;
}

Mesh load_mesh_obj(const char *filename)
{
    Mesh mesh = {};

    char *path = resolve_path(GamePath_models, filename, g_frame);
    if (path == nullptr) {
        DEBUG_LOG("unable to resolve path: %s", path);
        return mesh;
    }

    usize size;
    char *file = read_file(path, &size, g_frame);
    if (file == nullptr) {
        DEBUG_LOG("unable to read file: %s", path);
        return mesh;
    }

    char *end  = file + size;

    DEBUG_LOG("-- file size: %llu bytes", size);

    i32 num_faces   = 0;

    auto vectors = array_create<Vector3>(g_heap);
    auto normals = array_create<Vector3>(g_frame);
    auto uvs     = array_create<Vector2>(g_frame);

    char *ptr = file;
    while (ptr < end) {
        // texture coordinates
        if (ptr[0] == 'v' && ptr[1] == 't') {
            ptr += 3;

            Vector2 uv;
            sscanf(ptr, "%f %f", &uv.x, &uv.y);
            array_add(&uvs, uv);
        // normals
        } else if (ptr[0] == 'v' && ptr[1] == 'n') {
            ptr += 3;

            Vector3 n;
            sscanf(ptr, "%f %f %f", &n.x, &n.y, &n.z);
            array_add(&normals, n);
        // vertices
        } else if (ptr[0] == 'v') {
            ptr += 2;

            Vector3 v;
            sscanf(ptr, "%f %f %f", &v.x, &v.y, &v.z);
            array_add(&vectors, v);
        // faces
        } else if (ptr[0] == 'f') {
            do ptr++;
            while (ptr[0] == ' ');

            i32 num_dividers = 0;
            char *face = ptr;
            do {
                if (*face++ == '/') {
                    num_dividers++;
                }
            } while (face < end && face[0] != '\n' && face[0] != '\r');

            // NOTE(jesper): only supporting triangulated meshes
            assert(num_dividers == 6);
            num_faces++;
        }

        do ptr++;
        while (ptr < end && !is_newline(ptr[0]));

        do ptr++;
        while (ptr < end && is_newline(ptr[0]));
    }

    assert(vectors.count > 0);
    assert(normals.count > 0);
    assert(uvs.count> 0);
    assert(num_faces> 0);

    DEBUG_LOG("-- vectors : %d", vectors.count);
    DEBUG_LOG("-- normals : %d", normals.count);
    DEBUG_LOG("-- uvs     : %d", uvs.count);
    DEBUG_LOG("-- faces   : %d", num_faces);

    auto vertices = array_create<Vertex>(g_frame);

    ptr = file;
    while (ptr < end) {
        if (ptr[0] == 'f') {
            do ptr++;
            while (ptr[0] == ' ');

            u32 iv0, iv1, iv2;
            u32 it0, it1, it2;
            u32 in0, in1, in2;

            sscanf(ptr, "%u/%u/%u %u/%u/%u %u/%u/%u",
                   &iv0, &it0, &in0, &iv1, &it1, &in1, &iv2, &it2, &in2);

            // NOTE(jesper): objs are 1 indexed
            iv0--; iv1--; iv2--;
            it0--; it1--; it2--;
            in0--; in1--; in2--;

            Vertex v0;
            v0.p  = vectors[iv0];
            v0.n  = normals[in0];
            v0.uv = uvs[it0];

            Vertex v1;
            v1.p  = vectors[iv1];
            v1.n  = normals[in1];
            v1.uv = uvs[it1];

            Vertex v2;
            v2.p  = vectors[iv2];
            v2.n  = normals[in2];
            v2.uv = uvs[it2];

            array_add(&vertices, v0);
            array_add(&vertices, v1);
            array_add(&vertices, v2);
        }

        do ptr++;
        while (ptr < end && !is_newline(ptr[0]));

        do ptr++;
        while (ptr < end && is_newline(ptr[0]));
    }

    mesh.vertices = array_create<Vertex>(g_persistent);
    mesh.indices  = array_create<u32>(g_persistent);

    i32 j = 0;
    for (i32 i = 0; i < vertices.count; i++) {
        bool unique = true;
        for (j = 0; j < mesh.vertices.count; j++) {
            if (vertices[i] == mesh.vertices[j]) {
                unique = false;
                break;
            }
        }

        if (unique) {
            u32 index = (u32)array_add(&mesh.vertices, vertices[i]);
            array_add(&mesh.indices, index);
        } else {
            u32 index = (u32)j;
            array_add(&mesh.indices, index);
        }
    }

#if 0
    for (i32 i = 0; i < mesh.vertices.count; i++) {
        DEBUG_LOG("vertex[%d]", i);
        DEBUG_LOG("vector = { %f, %f, %f }",
                  mesh.vertices[i].vector.x,
                  mesh.vertices[i].vector.y,
                  mesh.vertices[i].vector.z);
        DEBUG_LOG("normal = { %f, %f, %f }",
                  mesh.vertices[i].normal.x,
                  mesh.vertices[i].normal.y,
                  mesh.vertices[i].normal.z);
        DEBUG_LOG("uv = { %f, %f }",
                  mesh.vertices[i].uv.x,
                  mesh.vertices[i].uv.y);
    }

    for (i32 i = 0; i < mesh.indices.count; i += 3) {
        DEBUG_LOG("triangle: %u, %u, %u",
                  mesh.indices[i+0],
                  mesh.indices[i+1],
                  mesh.indices[i+2]);
    }
#endif

    return mesh;
}

extern Catalog g_catalog;

Texture load_texture(Path path)
{
    Texture t = {};
    u32 ehash = hash32(path.extension.bytes);

    // TODO(jesper): this isn't constexpr because C++ constexpr is like the
    // village idiot everyone smiles at and tries to ignore and go on about
    // their day.
    u32 bmp_hash = hash32("bmp");
    // This is also why this isn't a switch case.
    if (ehash == bmp_hash) {
        t = load_texture_bmp(path.absolute.bytes);
    } else {
        DEBUG_LOG("unknown texture extension: %s", path.extension.bytes);
    }

    return t;
}


Texture* add_texture(const char *name,
                     u32 width, u32 height,
                     VkFormat format, void *pixels,
                     VkComponentMapping components)
{
    Texture t = {};
    t.width    = width;
    t.height   = height;
    t.format   = format;
    t.data     = pixels;
    t.asset_id = g_catalog.next_asset_id++;

    init_vk_texture(&t, components);

    TextureID texture_id = (TextureID)array_add(&g_textures, t);

    table_add(&g_catalog.assets,   name,       t.asset_id);
    table_add(&g_catalog.textures, t.asset_id, texture_id);

    return &g_textures[texture_id];
}

Texture* add_texture(Path path)
{
    Texture t = load_texture(path);
    if (t.data == nullptr) {
        return nullptr;
    }

    t.asset_id = g_catalog.next_asset_id++;

    init_vk_texture(&t, VkComponentMapping{});

    TextureID texture_id = (TextureID)array_add(&g_textures, t);

    table_add( & g_catalog.assets,   path.filename.bytes, t.asset_id);
    table_add( & g_catalog.textures, t.asset_id,          texture_id);

    return &g_textures[texture_id];
}

AssetID find_asset_id(const char *name)
{
    i32 *id = table_find(&g_catalog.assets, name);
    if (id == nullptr) {
        return ASSET_INVALID_ID;
    }

    return *id;
}

Texture* find_texture(AssetID id)
{
    if (id == ASSET_INVALID_ID) {
        DEBUG_LOG(Log_error, "invalid texture id: %d", id);
        return {};
    }

    TextureID *tid = table_find(&g_catalog.textures, id);
    if (tid == nullptr) {
        DEBUG_LOG(Log_error, "invalid asset id for texture: %d", id);
        return nullptr;
    }

    if (*tid >= g_textures.count) {
        DEBUG_LOG(Log_error, "invalid texture id for texture: %d", tid);
        return nullptr;
    }

    return &g_textures[*tid];
}

Texture* find_texture(const char *name)
{
    AssetID *id = table_find(&g_catalog.assets, name);
    if (id == nullptr || *id == ASSET_INVALID_ID) {
        DEBUG_LOG(Log_error, "unable to find texture with name: %s", name);
        return nullptr;
    }

    TextureID *tid = table_find(&g_catalog.textures, *id);
    if (tid == nullptr || *tid == ASSET_INVALID_ID) {
        DEBUG_LOG(Log_error, "unable to find texture with name: %s", name);
        return nullptr;
    }

    return &g_textures[*tid];
}

EntityData parse_entity_data(Path p)
{
    usize size;
    char *fp = read_file(p.absolute.bytes, &size, g_frame);

    if (fp == nullptr) {
        DEBUG_LOG(Log_error, "unable to read entity file: %s", p.absolute.bytes);
        return {};
    }

    EntityData data = {};

    Token t;
    Lexer l = create_lexer(fp, size);

    t = next_token(&l);
    if (t.type != Token::hash) {
        DEBUG_LOG(Log_error, "parse error in %s: expected version declaration");
        return {};
    }

    t = next_token(&l);
    if (t.type != Token::identifier || !is_identifier(t, "version")) {
        DEBUG_LOG(Log_error, "parse error in %s: expected version declaration", p);
        return {};
    }

    t = next_token(&l);
    if (t.type != Token::number) {
        DEBUG_LOG(Log_error, "parse error in %s: expected version number", p);
        return {};
    }

    i64 version = read_i64(t);
    assert(version == 1);

    t = next_token(&l);
    if (t.type != Token::identifier) {
        DEBUG_LOG(Log_error, "parse error in %s: expected identifier", p);
        return {};
    }

    while (t.type != Token::eof) {
        if (is_identifier(t, "position")) {

            t = next_token(&l);
            data.position.x = read_f32(t);

            do t = next_token(&l);
            while (t.type != Token::comma);

            t = next_token(&l);
            data.position.y = read_f32(t);

            do t = next_token(&l);
            while (t.type != Token::comma);

            t = next_token(&l);
            data.position.z = read_f32(t);

            do t = next_token(&l);
            while (t.type != Token::semicolon);
        } else {
            DEBUG_LOG(Log_error, "parse error %s:%d: unknown identifier: %.*s",
                      p, l.line_number, t.length, t.str);
            return {};
        }

        t = next_token(&l);
    }

    data.valid = true;
    return data;
}

i32 add_entity(Path p)
{
    EntityData data = parse_entity_data(p);
    Entity e = entities_add(data.position);

    // TODO(jesper): determine if entity has physics enabled
    i32 pid = physics_add(e);
    (void)pid;

    // TODO(jesper): find mesh in mesh table
    Mesh cube = load_mesh_obj("cube.obj");

    IndexRenderObject obj = {};
    obj.material = &g_game->materials.phong;

    usize vertex_size = cube.vertices.count * sizeof(cube.vertices[0]);
    usize index_size  = cube.indices.count  * sizeof(cube.indices[0]);

    obj.entity_id   = e.id;
    obj.pipeline    = g_game->pipelines.mesh;
    obj.index_count = (i32)cube.indices.count;
    obj.vbo         = create_vbo(cube.vertices.data, vertex_size);
    obj.ibo         = create_ibo(cube.indices.data, index_size);

    array_add(&g_game->index_render_objects, obj);

    AssetID asset_id = g_catalog.next_asset_id++;

    table_add(&g_catalog.assets,   p.filename.bytes, asset_id);
    table_add(&g_catalog.entities, asset_id,         e.id);

    return 1;
}

i32 add_entity(const char *p)
{
    return add_entity(create_path(p));
}

void process_catalog_system()
{
    PROFILE_FUNCTION();

    lock_mutex(&g_catalog.mutex);
    // TODO(jesper): we could potentially make this multi-threaded if we ensure
    // that we have thread safe versions of update_vk_texture, currently that'd
    // mean handling multi-threaded command buffer creation and submission
    for (i32 i = 0; i < g_catalog.process_queue.count; i++) {
        Path &p = g_catalog.process_queue[i];

        catalog_process_t **func = table_find(&g_catalog.processes, p.extension.bytes);
        if (func == nullptr) {
            DEBUG_LOG(Log_error,
                      "could not find process function for extension: %.*s",
                      p.extension.length, p.extension.bytes);
            continue;
        }

        (*func)(p);
    }

    g_catalog.process_queue.count = 0;
    unlock_mutex(&g_catalog.mutex);
}

CATALOG_CALLBACK(catalog_thread_proc)
{
    i32 *id = table_find(&g_catalog.assets, path.filename.bytes);
    if (id == nullptr || *id == ASSET_INVALID_ID) {
        DEBUG_LOG(Log_warning, "did not find asset in catalog system: %s",
                  path.filename.bytes);
        // TODO(jesper): support creation of new assets?
        return;
    }

    printf("asset modified: %s\n", path.filename.bytes);

    lock_mutex(&g_catalog.mutex);
    array_add(&g_catalog.process_queue, path);
    unlock_mutex(&g_catalog.mutex);
}

CATALOG_PROCESS_FUNC(catalog_process_bmp)
{
    AssetID id = find_asset_id(path.filename.bytes);
    if (id == ASSET_INVALID_ID) {
        add_texture(path);
        return;
    }

    Texture *t = find_texture(id);
    if (t != nullptr) {
        Texture n = load_texture(path);
        if (n.data != nullptr) {
            update_vk_texture(t, n);
        }
    }
}


CATALOG_PROCESS_FUNC(catalog_process_entity)
{
    AssetID id = find_asset_id(path.filename.bytes);
    if (id == ASSET_INVALID_ID) {
        add_entity(path);
        return;
    }

    EntityID *eid = table_find(&g_catalog.entities, id);
    if (eid && *eid != ASSET_INVALID_ID) {
        Entity &e = g_entities[*eid];

        EntityData data = parse_entity_data(path);
        e.position = data.position;
    }
}

void init_catalog_system()
{
    g_catalog = {};
    init_array(&g_catalog.folders,       g_heap);

    init_table(&g_catalog.assets,        g_heap);
    init_table(&g_catalog.textures,      g_heap);
    init_table(&g_catalog.entities,      g_heap);

    init_table(&g_catalog.processes,     g_heap);
    init_array(&g_catalog.process_queue, g_heap);

    init_mutex(&g_catalog.mutex);

    array_add(&g_catalog.folders, resolve_path(GamePath_data, "textures", g_persistent));
    table_add(&g_catalog.processes, "bmp", catalog_process_bmp);

    array_add(&g_catalog.folders, resolve_path(GamePath_data, "entities", g_persistent));
    table_add(&g_catalog.processes, "ent", catalog_process_entity);

    init_array(&g_textures, g_heap);

    for (i32 i = 0; i < g_catalog.folders.count; i++) {
        Array<Path> files = list_files(g_catalog.folders[i], g_heap);
        for (i32 i = 0; i < files.count; i++) {
            Path &p = files[i];

            catalog_process_t **func = table_find(&g_catalog.processes,
                                                  p.extension.bytes);

            if (func == nullptr) {
                DEBUG_LOG(Log_error,
                          "could not find process function for extension: %.*s",
                          p.extension.length, p.extension.bytes);
                continue;
            }

            (*func)(p);

        }
    }

    // TODO(jesper): we can use 1 thread for all folders with inotify
    create_catalog_thread(g_catalog.folders, &catalog_thread_proc);
}
