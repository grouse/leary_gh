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
        LOG("unable to read file: %s", path);
        return texture;
    }

    LOG("Loading bmp: %s", path);
    LOG("-- file size: %llu bytes", size);


    ASSERT(file[0] == 'B' && file[1] == 'M');

    char *ptr = file;
    BitmapFileHeader *fh = (BitmapFileHeader*)ptr;

    if (fh->type != 0x4d42) {
        // TODO(jesper): support other bmp versions
        LOG_UNIMPLEMENTED();
        return Texture{};
    }

    ptr += sizeof(BitmapFileHeader);

    BitmapHeader *h = (BitmapHeader*)ptr;

    if (h->header_size != 40) {
        // TODO(jesper): support other bmp versions
        LOG_UNIMPLEMENTED();
        return Texture{};
    }

    if (h->header_size == 40) {
        LOG("-- version 3");
    }

    ASSERT(h->header_size == 40);
    ptr += sizeof(BitmapHeader);

    if (h->compression != 0) {
        // TODO(jesper): support compression
        LOG_UNIMPLEMENTED();
        return Texture{};
    }

    if (h->compression == 0) {
        LOG("-- uncompressed");
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
        LOG("-- bottom-up");
    }

    // NOTE(jesper): bmp's with bbp > 16 doesn't have a color palette
    // NOTE(jesper): and other bbps are untested atm
    if (h->bpp != 24) {
        // TODO(jesper): only 24 bpp is tested and supported
        LOG_UNIMPLEMENTED();
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

void add_vertex(Array<f32> *vertices, Vector3 p, Vector3 n, Vector2 uv)
{
    array_add(vertices, p.x);
    array_add(vertices, p.y);
    array_add(vertices, p.z);

    array_add(vertices, n.x);
    array_add(vertices, n.y);
    array_add(vertices, n.z);

    array_add(vertices, uv.x);
    array_add(vertices, uv.y);
}

void add_vertex(Array<f32> *vertices, Vector3 p)
{
    array_add(vertices, p.x);
    array_add(vertices, p.y);
    array_add(vertices, p.z);
}

Mesh load_mesh_obj(const char *filename)
{
    Mesh mesh = {};

    char *path = resolve_path(GamePath_models, filename, g_frame);
    if (path == nullptr) {
        LOG("unable to resolve path: %s", path);
        return mesh;
    }

    usize size;
    char *file = read_file(path, &size, g_frame);
    if (file == nullptr) {
        LOG("unable to read file: %s", path);
        return mesh;
    }

    char *end  = file + size;

    LOG("-- file size: %llu bytes", size);

    i32 num_faces   = 0;

    auto vectors = create_array<Vector3>(g_heap);
    auto normals = create_array<Vector3>(g_frame);
    auto uvs     = create_array<Vector2>(g_frame);

    char *ptr = file;
    while (ptr < end) {
        // texture coordinates
        if (ptr[0] == 'v' && ptr[1] == 't') {
            ptr += 3;

            Vector2 uv;
            sscanf(ptr, "%f %f", &uv.x, &uv.y);
            uv *= 2.0f;
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

            int expected = 0;
            if (normals.count > 0) expected += 3;
            if (uvs.count     > 0) expected += 3;


            // NOTE(jesper): only supporting triangulated meshes
            ASSERT(num_dividers == expected);
            num_faces++;
        }

        do ptr++;
        while (ptr < end && !is_newline(ptr[0]));

        do ptr++;
        while (ptr < end && is_newline(ptr[0]));
    }

    ASSERT(vectors.count > 0);
    ASSERT(num_faces> 0);

    LOG("-- vectors : %d", vectors.count);
    LOG("-- normals : %d", normals.count);
    LOG("-- uvs     : %d", uvs.count);
    LOG("-- faces   : %d", num_faces);

    bool has_normals = normals.count > 0;
    bool has_uvs     = uvs.count > 0;

    auto vertices = create_array<f32>(g_frame);

    ptr = file;

    if (has_normals && has_uvs) {
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

                add_vertex(&vertices, vectors[iv0], normals[in0], uvs[it0]);
                add_vertex(&vertices, vectors[iv1], normals[in1], uvs[it1]);
                add_vertex(&vertices, vectors[iv2], normals[in2], uvs[it2]);
            }

            do ptr++;
            while (ptr < end && !is_newline(ptr[0]));

            do ptr++;
            while (ptr < end && is_newline(ptr[0]));
        }
    } else {
        while (ptr < end) {
            if (ptr[0] == 'f') {
                do ptr++;
                while (ptr[0] == ' ');

                u32 iv0, iv1, iv2;
                sscanf(ptr, "%u %u %u", &iv0, &iv1, &iv2);

                // NOTE(jesper): objs are 1 indexed
                iv0--; iv1--; iv2--;

                add_vertex(&vertices, vectors[iv0]);
                add_vertex(&vertices, vectors[iv1]);
                add_vertex(&vertices, vectors[iv2]);
            }

            do ptr++;
            while (ptr < end && !is_newline(ptr[0]));

            do ptr++;
            while (ptr < end && is_newline(ptr[0]));
        }
    }

    mesh.vertices = create_array<f32>(g_persistent);
    mesh.indices  = create_array<u32>(g_persistent);

    if (has_normals && has_uvs) {
        u32 index = 0;
        for (i32 i = 0; i < vertices.count; i += 8) {
            bool unique = true;

            i32 j = 0;
            for (j = 0; j < mesh.vertices.count; j += 8) {

                if (vertices[i] == mesh.vertices[j] &&
                    vertices[i+1] == mesh.vertices[j+1] &&
                    vertices[i+2] == mesh.vertices[j+2] &&
                    vertices[i+3] == mesh.vertices[j+3] &&
                    vertices[i+4] == mesh.vertices[j+4] &&
                    vertices[i+5] == mesh.vertices[j+5] &&
                    vertices[i+6] == mesh.vertices[j+6] &&
                    vertices[i+7] == mesh.vertices[j+7])
                {
                    unique = false;
                    break;
                }
            }

            if (unique) {
                array_add(&mesh.indices, index++);

                array_add(&mesh.vertices, vertices[i]);
                array_add(&mesh.vertices, vertices[i+1]);
                array_add(&mesh.vertices, vertices[i+2]);
                array_add(&mesh.vertices, vertices[i+3]);
                array_add(&mesh.vertices, vertices[i+4]);
                array_add(&mesh.vertices, vertices[i+5]);
                array_add(&mesh.vertices, vertices[i+6]);
                array_add(&mesh.vertices, vertices[i+7]);
            } else {
                array_add(&mesh.indices, (u32)(j) / 8);
            }
        }
    } else {
        u32 index = 0;
        for (i32 i = 0; i < vertices.count; i += 3) {
            bool unique = true;

            i32 j = 0;
            for (j = 0; j < mesh.vertices.count; j += 3) {

                if (vertices[i] == mesh.vertices[j] &&
                    vertices[i+1] == mesh.vertices[j+1] &&
                    vertices[i+2] == mesh.vertices[j+2])
                {
                    unique = false;
                    break;
                }
            }

            if (unique) {
                array_add(&mesh.indices, index++);

                array_add(&mesh.vertices, vertices[i]);
                array_add(&mesh.vertices, vertices[i+1]);
                array_add(&mesh.vertices, vertices[i+2]);
            } else {
                array_add(&mesh.indices, (u32)(j) / 3);
            }
        }
    }


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
        LOG("unknown texture extension: %s", path.extension.bytes);
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
        LOG(Log_error, "invalid texture id: %d", id);
        return {};
    }

    TextureID *tid = table_find(&g_catalog.textures, id);
    if (tid == nullptr) {
        LOG(Log_error, "invalid asset id for texture: %d", id);
        return nullptr;
    }

    if (*tid >= g_textures.count) {
        LOG(Log_error, "invalid texture id for texture: %d", tid);
        return nullptr;
    }

    return &g_textures[*tid];
}

Texture* find_texture(const char *name)
{
    AssetID *id = table_find(&g_catalog.assets, name);
    if (id == nullptr || *id == ASSET_INVALID_ID) {
        LOG(Log_error, "unable to find texture with name: %s", name);
        return nullptr;
    }

    TextureID *tid = table_find(&g_catalog.textures, *id);
    if (tid == nullptr || *tid == ASSET_INVALID_ID) {
        LOG(Log_error, "unable to find texture with name: %s", name);
        return nullptr;
    }

    return &g_textures[*tid];
}

EntityData parse_entity_data(Path p)
{
    usize size;
    char *fp = read_file(p.absolute.bytes, &size, g_frame);

    if (fp == nullptr) {
        LOG(Log_error, "unable to read entity file: %s", p.absolute.bytes);
        return {};
    }

    EntityData data = {};

    Token t;
    Lexer l = create_lexer(fp, size);

    t = next_token(&l);
    if (t.type != Token::hash) {
        LOG(Log_error, "parse error in %s: expected version declaration");
        return {};
    }

    t = next_token(&l);
    if (t.type != Token::identifier || !is_identifier(t, "version")) {
        LOG(Log_error, "parse error in %s: expected version declaration", p);
        return {};
    }

    t = next_token(&l);
    if (t.type != Token::number) {
        LOG(Log_error, "parse error in %s: expected version number", p);
        return {};
    }

    i64 version = read_i64(t);
    ASSERT(version == 1);

    t = next_token(&l);
    if (t.type != Token::identifier) {
        LOG(Log_error, "parse error in %s: expected identifier", p);
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
            LOG(Log_error, "parse error %s:%d: unknown identifier: %.*s",
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
    obj.pipeline    = Pipeline_mesh;
    obj.index_count = cube.indices.count;
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
            LOG(Log_error,
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
        LOG("asset not found in catalogue system: %s\n",
            path.filename.bytes);
        return;
    }

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

        for (auto &p : files) {
            catalog_process_t **func = table_find(&g_catalog.processes,
                                                  p.extension.bytes);

            if (func == nullptr) {
                LOG(Log_error,
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
