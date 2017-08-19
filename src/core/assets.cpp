/**
 * file:    assets.cpp
 * created: 2017-04-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "vulkan_render.h"

struct Vertex {
    Vector3 vector;
    Vector3 normal;
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

struct Catalog {
    const char *folder;

    // NOTE: mapping between texture name and texture id
    HashTable<const char*, i32> table;
    Array<Texture> textures;

    Mutex mutex;
    Array<Path> process_queue;
};


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

Texture texture_load_bmp(const char *path)
{
    // TODO(jesper): make this path work with String struct
    Texture texture = {};

    usize size;
    char *file = platform_file_read(path, &size);
    if (file == nullptr) {
        DEBUG_LOG("unable to read file: %s", path);
        return texture;
    }
    defer { free(file); };

    DEBUG_LOG("Loading bmp: %s", path);
    DEBUG_LOG("-- file size: %llu bytes", size);


    DEBUG_ASSERT(file[0] == 'B' && file[1] == 'M');

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

    DEBUG_ASSERT(h->header_size == 40);
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

    char *path = platform_resolve_path(GamePath_models, filename);
    if (path == nullptr) {
        DEBUG_LOG("unable to resolve path: %s", path);
        return mesh;
    }
    defer { free(path); };

    usize size;
    char *file = platform_file_read(path, &size);
    if (file == nullptr) {
        DEBUG_LOG("unable to read file: %s", path);
        return mesh;
    }
    defer { free(file); };

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
            DEBUG_ASSERT(num_dividers == 6);
            num_faces++;
        }

        do ptr++;
        while (ptr < end && !is_newline(ptr[0]));

        do ptr++;
        while (ptr < end && is_newline(ptr[0]));
    }

    DEBUG_ASSERT(vectors.count > 0);
    DEBUG_ASSERT(normals.count > 0);
    DEBUG_ASSERT(uvs.count> 0);
    DEBUG_ASSERT(num_faces> 0);

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
            v0.vector = vectors[iv0];
            v0.normal = normals[in0];
            v0.uv     = uvs[it0];

            Vertex v1;
            v1.vector = vectors[iv1];
            v1.normal = normals[in1];
            v1.uv     = uvs[it1];

            Vertex v2;
            v2.vector = vectors[iv2];
            v2.normal = normals[in2];
            v2.uv     = uvs[it2];

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

extern Catalog g_texture_catalog;
Texture* add_texture(const char *name,
                     u32 width, u32 height,
                     VkFormat format, void *pixels,
                     VkComponentMapping components)
{
    Texture t = {};
    t.width   = width;
    t.height  = height;
    t.format  = format;
    t.data    = pixels;

    i32 id = g_texture_catalog.textures.count;
    t.id   = id;

    init_vk_texture(&t, components);

    array_add(&g_texture_catalog.textures, t);
    table_add(&g_texture_catalog.table, name, id);

    return &g_texture_catalog.textures[id];
}

Texture* add_texture(Path path)
{
    Texture t = {};
    u64 ehash = hash64(path.extension.bytes);
    switch (ehash) {
    case hash64("bmp"):
        t = texture_load_bmp(path.absolute.bytes);
        break;
    default:
        DEBUG_LOG("unknown texture extension: %s", path.extension.bytes);
        return nullptr;
    }

    i32 id = g_texture_catalog.textures.count;
    t.id   = id;

    init_vk_texture(&t, VkComponentMapping{});

    array_add(&g_texture_catalog.textures, t);
    table_add(&g_texture_catalog.table, path.filename.bytes, id);
    return &g_texture_catalog.textures[id];
}

i32 find_texture_id(const char *name)
{
    i32 *id = table_find(&g_texture_catalog.table, name);
    if (id == nullptr) {
        return TEXTURE_INVALID_ID;
    }

    return *id;
}

Texture find_texture(i32 id)
{
    if (id == TEXTURE_INVALID_ID || id >= g_texture_catalog.textures.count) {
        return Texture{};
    }

    return g_texture_catalog.textures[id];
}

Texture find_texture(const char *name)
{
    i32 *id = table_find(&g_texture_catalog.table, name);
    if (id == nullptr) {
        return Texture{};
    }

    return g_texture_catalog.textures[*id];
}

CATALOG_CALLBACK(texture_catalog_process)
{
    printf("texture modified: %s\n", path.filename.bytes);
    i32 id = find_texture_id(path.filename.bytes);
    if (id == TEXTURE_INVALID_ID) {
        // TODO(jesper): support creation of new textures
        return;
    }

    lock_mutex(&g_texture_catalog.mutex);
    array_add(&g_texture_catalog.process_queue, path);
    unlock_mutex(&g_texture_catalog.mutex);
}

void init_texture_catalog()
{
    g_texture_catalog = {};
    g_texture_catalog.folder = platform_resolve_path(GamePath_textures, "");
    g_texture_catalog.textures.allocator      = g_heap;
    g_texture_catalog.process_queue.allocator = g_heap;
    init_table(&g_texture_catalog.table, g_heap);

    Array<Path> files = list_files(g_texture_catalog.folder, g_heap);
    for (i32 i = 0; i < files.count; i++) {
        add_texture(files[i]);
    }

    // TODO(jesper): we can use 1 thread for all folders with inotify
    create_catalog_thread(g_texture_catalog.folder, &texture_catalog_process);
}
