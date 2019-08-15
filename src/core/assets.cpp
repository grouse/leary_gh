/**
 * file:    assets.cpp
 * created: 2017-04-03
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

struct Vertex {
    Vector3 p;
    Vector3 n;
    Vector2 uv;
};

bool operator == (Vertex &lhs, Vertex &rhs)
{
    return memcmp(&lhs, &rhs, sizeof(Vertex)) == 0;
}


Array<TextureAsset> g_textures;
Array<Mesh>    g_meshes;
Array<Entity>  g_entities;
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

PACKED(struct WAVHeader {
       char chunk_id[4];
       u32  chunk_size;
       char format[4];
       char subchunk1_id[4];
       u32  subchunk1_size;
       u16  audio_format;
       u16  num_channels;
       u32  sample_rate;
       u32  byte_rate;
       u16  block_align;
       u16  bits_per_sample;
       char subchunk2_id[4];
       u32  subchunk2_size;
});

SoundData load_sound_wav(FilePathView path, Allocator *allocator)
{
    SoundData sound = {};

    usize size;
    char *file = read_file(path, &size, g_frame);

    WAVHeader *header = (WAVHeader*)file;

    sound.sample_rate = header->sample_rate;
    sound.num_channels = header->num_channels;
    sound.bits_per_sample = header->bits_per_sample;
    sound.samples = (i16*)alloc(allocator, header->subchunk2_size);
    sound.num_samples = header->subchunk2_size / ((sound.num_channels * sound.bits_per_sample) / 8);
    memcpy(sound.samples, file + sizeof *header, header->subchunk2_size);

    if (sound.sample_rate != 48000) {
        LOG("unsupported sample rate: %d", sound.sample_rate);
        return {};
    }

    if (sound.bits_per_sample != 16) {
        LOG("unsupported bits per sample: %d", sound.bits_per_sample);
        return {};
    }

    return sound;
}

TextureData load_texture_bmp(FilePathView path, Allocator *allocator)
{
    // TODO(jesper): make this path work with String struct
    TextureData texture = {};

    usize size;
    char *file = read_file(path, &size, g_heap);
    defer { dealloc(g_heap, file); };

    if (file == nullptr) {
        LOG("unable to read file: %s", path.absolute.bytes);
        return texture;
    }

    LOG("Loading bmp: %s", path.absolute.bytes);
    LOG("-- file size: %llu bytes", size);


    ASSERT(file[0] == 'B' && file[1] == 'M');

    char *ptr = file;
    BitmapFileHeader *fh = (BitmapFileHeader*)ptr;

    if (fh->type != 0x4d42) {
        // TODO(jesper): support other bmp versions
        LOG_UNIMPLEMENTED();
        return {};
    }

    ptr += sizeof(BitmapFileHeader);

    BitmapHeader *h = (BitmapHeader*)ptr;

    if (h->header_size != 40) {
        // TODO(jesper): support other bmp versions
        LOG_UNIMPLEMENTED();
        return {};
    }

    if (h->header_size == 40) {
        LOG("-- version 3");
    }

    ASSERT(h->header_size == 40);
    ptr += sizeof(BitmapHeader);

    if (h->compression != 0) {
        // TODO(jesper): support compression
        LOG_UNIMPLEMENTED();
        return {};
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
        return {};
    }

    u8 channels = 4;

    texture.format = VK_FORMAT_B8G8R8A8_SRGB;
    texture.width  = h->width;
    texture.height = h->height;
    texture.size   = h->width * h->height * channels;
    texture.pixels = alloc(allocator, texture.size);

    bool alpha = false;

    u8 *src = (u8*)ptr;
    u8 *dst = (u8*)texture.pixels;

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
        dst = (u8*)texture.pixels;
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

Mesh load_mesh_obj(FilePathView path)
{
    Mesh mesh = {};

    usize size;
    char *file = read_file(path, &size, g_frame);
    if (file == nullptr) {
        LOG("unable to read file: %s", path.absolute.bytes);
        return mesh;
    }

    char *end  = file + size;

    LOG(" loading mesh: %.*s", path.filename.size, path.filename.bytes);
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

    init_array(&mesh.indices, g_heap);
    init_array(&mesh.points, g_heap);

    if (has_normals) {
        init_array(&mesh.normals, g_heap);
    }

    if (has_uvs) {
        init_array(&mesh.uvs, g_heap);
    }


    if (has_normals && has_uvs) {
        u32 index = 0;
        for (i32 i = 0; i < vertices.count; i += 8) {
            Vector3 point  = { vertices[i], vertices[i+1], vertices[i+2] };
            Vector3 normal = { vertices[i+3], vertices[i+4], vertices[i+5] };
            Vector2 uv     = { vertices[i+6], vertices[i+7] };

            for (i32 j = 0; j < mesh.points.count; j++) {
                if (mesh.points[j].x == point.x &&
                    mesh.points[j].y == point.y &&
                    mesh.points[j].z == point.z &&
                    mesh.normals[j].x == normal.x &&
                    mesh.normals[j].y == normal.y &&
                    mesh.normals[j].z == normal.z &&
                    mesh.uvs[j].u == uv.u &&
                    mesh.uvs[j].v == uv.v)
                {
                    array_add(&mesh.indices, (u32)j);
                    goto merged0;
                }
            }

            array_add(&mesh.indices, index++);
            array_add(&mesh.points, point);
            array_add(&mesh.normals, normal);
            array_add(&mesh.uvs, uv);
merged0:
            continue;
        }
    } else {
        u32 index = 0;
        for (i32 i = 0; i < vertices.count; i += 3) {
            Vector3 point  = { vertices[i], vertices[i+1], vertices[i+2] };

            for (i32 j = 0; j < mesh.points.count; j++) {
                if (mesh.points[j].x == point.x &&
                    mesh.points[j].y == point.y &&
                    mesh.points[j].z == point.z)
                {
                    array_add(&mesh.indices, (u32)j);
                    goto merged1;
                }
            }

            array_add(&mesh.indices, index++);
            array_add(&mesh.points, point);
merged1:
            continue;
        }
    }

    return mesh;
}

extern Catalog g_catalog;

TextureAsset* add_texture(
    StringView name,
    u32 width,
    u32 height,
    VkFormat format,
    void *pixels,
    VkComponentMapping components)
{
    TextureAsset ta = {};
    ta.asset_id = g_catalog.next_asset_id++;

    ta.gfx_texture = gfx_create_texture(
        width, height,
        1,
        format,
        components,
        pixels);

    TextureID texture_id = (TextureID)array_add(&g_textures, ta);

    map_add(&g_catalog.assets,   name,        ta.asset_id);
    map_add(&g_catalog.textures, ta.asset_id, texture_id);

    return &g_textures[texture_id];
}

void update_mesh(MeshID id, Mesh mesh)
{
    (void)id;
    (void)mesh;
#if 0
    Mesh *existing = find_mesh(id);
    ASSERT(existing != nullptr);

    gfx_update_buffer(
        &existing->vbo,
        mesh.vertices.data,
        mesh.vertices.count * sizeof mesh.vertices[0]);

    if (mesh.indices.count > 0) {
        usize indices_size = mesh.indices.count  * sizeof mesh.indices[0];
        if (existing->indices.count > 0) {
            gfx_update_buffer(
                &existing->vbo,
                mesh.vertices.data,
                indices_size);
        } else {
            existing->ibo = create_ibo(mesh.indices.data, indices_size);
        }

        existing->element_count = mesh.indices.count;
    } else {
        existing->element_count = mesh.vertices.count;
    }
#endif
}

i32 add_mesh(Mesh mesh, StringView name)
{
    if (mesh.indices.count > 0) {
        mesh.element_count = mesh.indices.count;
        mesh.ibo = create_ibo(mesh.indices.data, mesh.indices.count * sizeof mesh.indices[0]);

        mesh.vbo.points = create_vbo(mesh.points.data, mesh.points.count * sizeof mesh.points[0]);
        mesh.vbo.normals = create_vbo(mesh.normals.data, mesh.normals.count * sizeof mesh.normals[0]);
        mesh.vbo.tangents = create_vbo(mesh.tangents.data, mesh.tangents.count * sizeof mesh.tangents[0]);
        mesh.vbo.bitangents = create_vbo(mesh.bitangents.data, mesh.bitangents.count * sizeof mesh.bitangents[0]);
        mesh.vbo.uvs = create_vbo(mesh.uvs.data, mesh.uvs.count * sizeof mesh.uvs[0]);
    } else {
        mesh.element_count = mesh.points.count;

        mesh.vbo.points = create_vbo(mesh.points.data, mesh.points.count * sizeof mesh.points[0]);
        mesh.vbo.normals = create_vbo(mesh.normals.data, mesh.normals.count * sizeof mesh.normals[0]);
        mesh.vbo.tangents = create_vbo(mesh.tangents.data, mesh.tangents.count * sizeof mesh.tangents[0]);
        mesh.vbo.bitangents = create_vbo(mesh.bitangents.data, mesh.bitangents.count * sizeof mesh.bitangents[0]);
        mesh.vbo.uvs = create_vbo(mesh.uvs.data, mesh.uvs.count * sizeof mesh.uvs[0]);
    }

    mesh.asset_id = g_catalog.next_asset_id++;
    MeshID mesh_id = (MeshID)array_add(&g_meshes, mesh);

    map_add(&g_catalog.assets, name, mesh.asset_id);
    map_add(&g_catalog.meshes, mesh.asset_id, mesh_id);

    return mesh_id;
}


Mesh* add_mesh_obj(FilePath path)
{
    Mesh m = load_mesh_obj(path);
    if (m.points.count == 0) {
        return nullptr;
    }

    i32 mesh_id = add_mesh(m, path.filename);
    return &g_meshes[mesh_id];
}

AssetID find_asset_id(StringView name)
{
    AssetID *id = map_find(&g_catalog.assets, name);
    if (id == nullptr) {
        return ASSET_INVALID_ID;
    }

    return *id;
}

TextureAsset* find_texture(AssetID id)
{
    if (id == ASSET_INVALID_ID) {
        LOG_ERROR("invalid texture id: %d", (i32)id);
        return {};
    }

    TextureID *tid = map_find(&g_catalog.textures, id);
    if (tid == nullptr) {
        LOG_ERROR("invalid asset id for texture: %d", (i32)id);
        return nullptr;
    }

    if (*tid >= g_textures.count) {
        LOG_ERROR("invalid texture id for texture: %d", (i32)*tid);
        return nullptr;
    }

    return &g_textures[*tid];
}

TextureAsset* find_texture(StringView name)
{
    AssetID *id = map_find(&g_catalog.assets, name);
    if (id == nullptr || *id == ASSET_INVALID_ID) {
        LOG_ERROR("unable to find texture with name: %s", name.bytes);
        return nullptr;
    }

    TextureID *tid = map_find(&g_catalog.textures, *id);
    if (tid == nullptr || *tid == ASSET_INVALID_ID) {
        LOG_ERROR("unable to find texture with name: %s", name.bytes);
        return nullptr;
    }

    return &g_textures[*tid];
}

MeshID find_mesh_id(StringView name)
{
    AssetID *id = map_find(&g_catalog.assets, name);
    if (id == nullptr || *id == ASSET_INVALID_ID) {
        LOG_ERROR("unable to find mesh with name: %s", name.bytes);
        return ASSET_INVALID_ID;
    }

    MeshID *mid = map_find(&g_catalog.meshes, *id);
    if (mid == nullptr || *mid == ASSET_INVALID_ID) {
        LOG_ERROR("unable to find mesh with name: %s", name.bytes);
        return ASSET_INVALID_ID;
    }

    return *mid;
}

Mesh* find_mesh(StringView name)
{
    MeshID mid = find_mesh_id(name);
    if (mid == ASSET_INVALID_ID) {
        return nullptr;
    }

    if (mid >= g_meshes.count) {
        LOG_ERROR("mesh id is out of bounds");
        return nullptr;
    }

    return &g_meshes[mid];
}

Mesh* find_mesh(MeshID mesh_id)
{
    if (mesh_id == ASSET_INVALID_ID) {
        return nullptr;
    }

    if (mesh_id >= g_meshes.count) {
        LOG_ERROR("mesh id is out of bounds");
        return nullptr;
    }

    return &g_meshes[mesh_id];
}

Vector3 parse_vector3(FilePathView path, Lexer *lexer)
{
    Vector3 v;
    Token x, y, z;

    x = next_token(lexer);
    v.x = read_f32(x);

    if (eat_until(path, lexer, Token::comma) == false) {
        return {};
    }

    y = next_token(lexer);
    v.y = read_f32(y);

    if (eat_until(path, lexer, Token::comma) == false) {
        return {};
    }

    z = next_token(lexer);
    v.z = read_f32(z);

    if (eat_until(path, lexer, Token::semicolon) == false) {
        return {};
    }

    return v;
}

Vector4 parse_vector4(FilePathView path, Lexer *lexer)
{
    Vector4 v;
    Token x, y, z, w;

    x = next_token(lexer);
    v.x = read_f32(x);

    if (eat_until(path, lexer, Token::comma) == false) {
        return {};
    }

    y = next_token(lexer);
    v.y = read_f32(y);

    if (eat_until(path, lexer, Token::comma) == false) {
        return {};
    }

    z = next_token(lexer);
    v.z = read_f32(z);

    if (eat_until(path, lexer, Token::comma) == false) {
        return {};
    }

    w = next_token(lexer);
    v.w = read_f32(w);


    if (eat_until(path, lexer, Token::semicolon) == false) {
        return {};
    }

    return v;
}


EntityData parse_entity_data(FilePath p)
{
    usize size;
    char *fp = read_file(p.absolute.bytes, &size, g_frame);

    if (fp == nullptr) {
        LOG_ERROR("unable to read entity file: %s", p.absolute.bytes);
        return {};
    }

    EntityData data = {};

    Token t;
    Lexer l = create_lexer(fp, size);

    t = next_token(&l);
    if (t.type != Token::hash) {
        PARSE_ERROR(p, l, "expected version declaration");
        return {};
    }

    t = next_token(&l);
    if (t.type != Token::identifier || !is_identifier(t, "version")) {
        PARSE_ERROR(p, l, "expected version declaration");
        return {};
    }

    t = next_token(&l);
    if (t.type != Token::number) {
        PARSE_ERROR(p, l, "expected version number");
        return {};
    }

    i64 version = read_i64(t);

    t = next_token(&l);
    if (t.type != Token::identifier) {
        PARSE_ERROR(p, l, "expected identifier");
        return {};
    }

    if (version < 2) {
        data.mesh = create_string(g_frame, "cube.obj");
    }

    init_array(&data.textures, g_heap);
    if (version < 5) {
        array_add(&data.textures, create_string(g_frame, "greybox.bmp"));
    }

    data.scale    = { 1.0f, 1.0f, 1.0f };
    data.rotation = Quaternion::make( Vector3{ 0.0f, 1.0f, 0.0f } );

    while (t.type != Token::eof) {
        if (is_identifier(t, "position")) {
            data.position = parse_vector3(p, &l);
        } else if (version >= 2 && is_identifier(t, "mesh")) {
            Token m = next_token(&l);

            if (eat_until(p, &l, &t, Token::semicolon) == false) {
                return {};
            }

            i32 length = (i32)(t.str - m.str);
            data.mesh = create_string(g_frame, StringView{ m.str, length+1 });
        } else if (version >= 3 && is_identifier(t, "scale")) {
            data.scale = parse_vector3(p, &l);
        } else if (version >= 4 && is_identifier(t, "rotation")) {
            t = next_token(&l);
            if (is_identifier(t, "quaternion")) {
                data.rotation = Quaternion::make(parse_vector4(p, &l));
            } else if (is_identifier(t, "euler")) {
                Vector3 euler = parse_vector3(p, &l);
                euler.x = radian_from_degree(euler.x);
                euler.y = radian_from_degree(euler.y);
                euler.z = radian_from_degree(euler.z);
                data.rotation = quat_from_euler(euler);
            } else {
                PARSE_ERROR_F(
                    p, l,
                    "expected \"quaternion\" or \"euler\" after \"rotation\", got %.*s",
                    t.length, t.str);

                if (eat_until(p, &l, Token::semicolon) == false) {
                    return {};
                }
            }
        } else if (version >= 5 && is_identifier(t, "texture")) {
            Token m = next_token(&l);

            if (eat_until(p, &l, &t, Token::semicolon) == false) {
                return {};
            }

            i32 length = (i32)(t.str - m.str);
            array_add(&data.textures, create_string(g_frame, StringView{ m.str, length+1 }));
        } else {
            PARSE_ERROR_F(p, l, "unknown identifier: %.*s", t.length, t.str);
            return {};
        }

        t = next_token(&l);
    }

    data.valid = true;

    if (data.mesh.size > 0) {
        data.mesh_id = find_mesh_id(data.mesh);
        data.valid = data.mesh_id != ASSET_INVALID_ID;
    }

    return data;
}

EntityID find_entity_id(StringView name)
{
    AssetID *asset_id = map_find(&g_catalog.assets, name);
    if (asset_id == nullptr) {
        LOG_ERROR("Invalid entity: %s", name.bytes);
        return ASSET_INVALID_ID;
    }

    EntityID *entity_id = map_find(&g_catalog.entities, *asset_id);
    if (entity_id == nullptr) {
        return ASSET_INVALID_ID;
    }

    return *entity_id;
}

void process_catalog_system()
{
    PROFILE_FUNCTION();

    lock_mutex(&g_catalog.mutex);
    // TODO(jesper): we could potentially make this multi-threaded if we ensure
    // that we have thread safe versions of update_vk_texture, currently that'd
    // mean handling multi-threaded command buffer creation and submission
    for (i32 i = 0; i < g_catalog.process_queue.count; i++) {
        FilePath &p = g_catalog.process_queue[i];

        catalog_process_t **func = map_find(&g_catalog.processes, p.extension);
        if (func == nullptr) {
            LOG_ERROR("could not find process function for extension: %.*s",
                      p.extension.size,
                      p.extension.bytes);
            continue;
        }

        (*func)(p);
    }

    array_clear(&g_catalog.process_queue);

    g_catalog.process_queue.count = 0;
    unlock_mutex(&g_catalog.mutex);
}

CATALOG_CALLBACK(catalog_thread_proc)
{
    AssetID *id = map_find(&g_catalog.assets, path.filename);
    if (id == nullptr || *id == ASSET_INVALID_ID) {
        LOG("asset not found in catalogue system: %s\n",
            path.filename.bytes);
        return;
    }

    lock_mutex(&g_catalog.mutex);
    defer { unlock_mutex(&g_catalog.mutex); };

    for (auto p : g_catalog.process_queue) {
        if (p == path) {
            return;
        }
    }

    array_add(&g_catalog.process_queue, path);
}

CATALOG_PROCESS_FUNC(catalog_process_bmp)
{
    AssetID id = find_asset_id(path.filename.bytes);
    if (id == ASSET_INVALID_ID) {
        TextureData t = load_texture_bmp(path, g_heap);
        defer { dealloc(g_heap, t.pixels); };

        if (t.pixels != nullptr) {
            u32 mip_levels = (u32)floor(log2(max(t.width, t.height))) + 1;

            TextureAsset ta = {};
            ta.asset_id = g_catalog.next_asset_id++;
            ta.gfx_texture = gfx_create_texture(
                t.width,
                t.height,
                mip_levels,
                t.format,
                VkComponentMapping{},
                t.pixels);

            TextureID texture_id = (TextureID)array_add(&g_textures, ta);

            map_add(&g_catalog.assets,   path.filename, ta.asset_id);
            map_add(&g_catalog.textures, ta.asset_id,   texture_id);
        }
    } else {
        TextureAsset *t = find_texture(id);
        ASSERT(t != nullptr);

        TextureData n = load_texture_bmp(path, g_frame);

        if (n.pixels != nullptr) {
            u32 mip_levels = (u32)floor(log2(max(n.width, n.height))) + 1;

            GfxTexture texture = gfx_create_texture(
                n.width,
                n.height,
                mip_levels,
                n.format,
                VkComponentMapping{},
                n.pixels);
            gfx_copy_texture(&t->gfx_texture, &texture);
            gfx_destroy_texture(texture);
        }
    }
}

void set_entity_data(Entity *entity, EntityData data)
{
    entity->position = data.position;
    entity->scale    = data.scale;
    entity->rotation = data.rotation;
    entity->mesh_id  = data.mesh_id;

    i32 binding = 0;
    for (i32 i = 0; i < data.textures.count; i++) {
        TextureAsset *texture = find_texture(data.textures[i]);
        if (texture != nullptr) {
            gfx_set_texture(
                Pipeline_mesh,
                entity->descriptor_set, binding++,
                texture->gfx_texture);
        }
    }
}

CATALOG_PROCESS_FUNC(catalog_process_entity)
{
    AssetID id = find_asset_id(path.filename.bytes);
    if (id == ASSET_INVALID_ID) {
        EntityData data = parse_entity_data(path);

        if (data.valid == false) {
            LOG_ERROR("failed parsing entity data");
            return;
        }

        Entity e = {};
        e.id = (i32)g_entities.count;
        e.descriptor_set = gfx_create_descriptor(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            g_vulkan->pipelines[Pipeline_mesh].set_layouts[1]);

        set_entity_data(&e, data);
        array_add(&g_entities, e);

        AssetID asset_id = g_catalog.next_asset_id++;

        map_add(&g_catalog.assets, path.filename, asset_id);
        map_add(&g_catalog.entities, asset_id, e.id);
    } else {
        EntityID *eid = map_find(&g_catalog.entities, id);
        if (eid && *eid != ASSET_INVALID_ID) {
            EntityData data = parse_entity_data(path);
            if (data.valid == false) {
                LOG_ERROR("failed parsing entity data");
                return;
            }

            set_entity_data(&g_entities[*eid], data);
        }
    }
}

CATALOG_PROCESS_FUNC(catalog_process_obj)
{
    AssetID id = find_asset_id(path.filename.bytes);
    if (id == ASSET_INVALID_ID) {
        add_mesh_obj(path);
        return;
    }

    ASSERT(false && "hot reloading changed meshes not supported");
}

CATALOG_PROCESS_FUNC(catalog_process_fbx)
{
    String prefix = create_string(
        g_frame,
        { StringView(path.filename.bytes, path.filename.size - path.extension.size),
        "_" });
    FilePath output = resolve_file_path(GamePath_models, "", g_frame);
    AlcResult result = alc_convert_fbx(path.absolute.bytes, output.absolute.bytes, prefix.bytes);
    ASSERT(result == ALC_RESULT_SUCCESS);
}

CATALOG_PROCESS_FUNC(catalog_process_msh)
{
    AssetID id = find_asset_id(path.filename.bytes);
    LOG("loading mesh: %s", path.filename.bytes);

    usize size;
    char *file = read_file(path, &size, g_frame);
    if (file == nullptr) {
        LOG("unable to read file: %s", path.absolute.bytes);
        return;
    }

    LOG("-- file size: %llu bytes", size);

    AlcMeshHeader *amesh = (AlcMeshHeader*)file;
    if (amesh->identifier != ALC_MESH_IDENTIFIER) {
        LOG("-- invalid Alchemy Mesh (%s) identifier: %d",
            path.filename.bytes,
            amesh->identifier);
        return;
    }

    if (amesh->num_vertices == 0) {
        LOG("-- Mesh (%s) contains no vertices", path.filename.bytes);
        return;
    }

    // TODO(jesper): support meshes without uvs?
    if ((amesh->flags & ALC_MESH_FLAG_NORMAL_BIT) == 0) {
        LOG("-- Mesh (%s) contains no normals", path.filename.bytes);
        return;
    }

    // TODO(jesper): support meshes without normals?
    if ((amesh->flags & ALC_MESH_FLAG_UV_BIT) == 0) {
        LOG("-- Mesh (%s) contains no uvs", path.filename.bytes);
        return;
    }

    LOG(" -- num vertices: %d", amesh->num_vertices);
    LOG(" -- num indices: %d", amesh->num_vertices);
    LOG(" -- normals: %s", amesh->flags & ALC_MESH_FLAG_NORMAL_BIT ? "yes" : "no");
    LOG(" -- uvs: %s", amesh->flags & ALC_MESH_FLAG_UV_BIT ? "yes" : "no");

    Mesh mesh = {};
    init_array(&mesh.points, g_heap, amesh->num_vertices);

    if (amesh->flags & ALC_MESH_FLAG_NORMAL_BIT) {
        init_array(&mesh.normals, g_heap, amesh->num_vertices);
    }

    if (amesh->flags & ALC_MESH_FLAG_UV_BIT) {
        init_array(&mesh.uvs, g_heap, amesh->num_vertices);
        init_array(&mesh.tangents, g_heap, amesh->num_vertices);
        init_array(&mesh.bitangents, g_heap, amesh->num_vertices);
    }

    if (amesh->version < 2) {
        f32 *vertices = (f32*)(file + sizeof *amesh);
        for (u32 i = 0; i < amesh->num_vertices; i++) {
            Vector3 point;
            point.x = *vertices++;
            point.y = *vertices++;
            point.z = *vertices++;

            array_add(&mesh.points, point);

            if (amesh->flags & ALC_MESH_FLAG_NORMAL_BIT) {
                Vector3 normal;
                normal.x = *vertices++;
                normal.y = *vertices++;
                normal.z = *vertices++;

                array_add(&mesh.normals, normal);
            }

            if (amesh->flags & ALC_MESH_FLAG_UV_BIT) {
                Vector2 uv;
                uv.u = *vertices++;
                uv.v = *vertices++;

                array_add(&mesh.uvs, uv);
            }
        }

        if (amesh->flags & ALC_MESH_FLAG_UV_BIT) {
            for (i32 i = 0; i < mesh.points.count; i += 3) {
                Vector3 p0 = mesh.points[i];
                Vector3 p1 = mesh.points[i+1];
                Vector3 p2 = mesh.points[i+2];

                Vector2 uv0 = mesh.uvs[i];
                Vector2 uv1 = mesh.uvs[i+1];
                Vector2 uv2 = mesh.uvs[i+2];

                Vector3 t, b;
                calc_tangent_and_bitangent(
                    &t, &b,
                    p1 - p0, p2 - p0,
                    uv1 - uv0, uv2 - uv0);

                array_add(&mesh.tangents, t);
                array_add(&mesh.tangents, t);
                array_add(&mesh.tangents, t);

                array_add(&mesh.bitangents, b);
                array_add(&mesh.bitangents, b);
                array_add(&mesh.bitangents, b);
            }
        }
    } else {
        isize offset = sizeof *amesh;

        usize points_size  = amesh->num_vertices * sizeof(f32) * 3;
        usize normals_size = amesh->num_vertices * sizeof(f32) * 3;
        usize uvs_size     = amesh->num_vertices * sizeof(f32) * 2;

        f32 *points  = (f32*)(file + offset);
        memcpy(mesh.points.data, points, points_size);
        mesh.points.count = amesh->num_vertices;
        offset += points_size;
        
        // NOTE(jesper): swap to counter-clockwise-winding
        for (i32 i = 0; i < mesh.points.count; i+=3) {
            Vector3 tmp = mesh.points[i];
            mesh.points[i] = mesh.points[i+2];
            mesh.points[i+2] = tmp;
        }
        
        if (amesh->flags & ALC_MESH_FLAG_NORMAL_BIT) {
            f32 *normals  = (f32*)(file + offset);
            memcpy(mesh.normals.data, normals, normals_size);
            mesh.normals.count = amesh->num_vertices;
            offset += normals_size;
            
            // NOTE(jesper): swap to counter-clockwise-winding
            for (i32 i = 0; i < mesh.normals.count; i+=3) {
                Vector3 tmp = mesh.normals[i];
                mesh.normals[i] = mesh.normals[i+2];
                mesh.normals[i+2] = tmp;
            }
        }

        if (amesh->flags & ALC_MESH_FLAG_UV_BIT) {
            f32 *uvs  = (f32*)(file + offset);
            memcpy(mesh.uvs.data, uvs, uvs_size);
            mesh.uvs.count = amesh->num_vertices;
            offset += uvs_size;
            
            // NOTE(jesper): swap to counter-clockwise-winding
            for (i32 i = 0; i < mesh.uvs.count; i+=3) {
                Vector2 tmp = mesh.uvs[i];
                mesh.uvs[i] = mesh.uvs[i+2];
                mesh.uvs[i+2] = tmp;
            }

            for (i32 i = 0; i < mesh.points.count; i += 3) {
                Vector3 p0 = mesh.points[i];
                Vector3 p1 = mesh.points[i+1];
                Vector3 p2 = mesh.points[i+2];

                Vector2 uv0 = mesh.uvs[i];
                Vector2 uv1 = mesh.uvs[i+1];
                Vector2 uv2 = mesh.uvs[i+2];

                Vector3 t, b;
                calc_tangent_and_bitangent(
                    &t, &b,
                    p1 - p0, p2 - p0,
                    uv1 - uv0, uv2 - uv0);

                array_add(&mesh.tangents, t);
                array_add(&mesh.tangents, t);
                array_add(&mesh.tangents, t);

                array_add(&mesh.bitangents, b);
                array_add(&mesh.bitangents, b);
                array_add(&mesh.bitangents, b);
            }
        }
    }

    if (id == ASSET_INVALID_ID) {
        add_mesh(mesh, path.filename);
    } else {
        MeshID *mesh_id = map_find(&g_catalog.meshes, id);
        ASSERT(mesh_id != nullptr);
        update_mesh(*mesh_id, mesh);
    }
}

CATALOG_PROCESS_FUNC(catalog_process_glsl)
{
    AssetID id = find_asset_id(path.filename.bytes);
    LOG("loading shader: %s", path.filename.bytes);

    usize size;
    char *file = read_file(path, &size, g_frame);
    if (file == nullptr) {
        LOG("unable to read file: %s", path.absolute.bytes);
        return;
    }

    LOG("-- file size: %llu bytes", size);

    PipelineID pipeline;
    if (path.filename == "mesh.glsl") {
        pipeline = Pipeline_mesh;
    } else if (path.filename == "font.glsl") {
        pipeline = Pipeline_font;
    } else if (path.filename == "basic2d.glsl") {
        pipeline = Pipeline_basic2d;
    } else if (path.filename == "gui_basic.glsl") {
        pipeline = Pipeline_gui_basic;
    } else if (path.filename == "terrain.glsl") {
        pipeline = Pipeline_terrain;
    } else if (path.filename == "line.glsl") {
        pipeline = Pipeline_line;
    } else if (path.filename == "wireframe.glsl") {
        pipeline = Pipeline_wireframe;
    } else if (path.filename == "wireframe_lines.glsl") {
        pipeline = Pipeline_wireframe_lines;
    } else {
        LOG_ERROR("unknown shader pipeline: %s", path.filename.bytes);
        return;
    }

    create_pipeline(pipeline);
    if (id == ASSET_INVALID_ID) {
        AssetID asset_id = g_catalog.next_asset_id++;
        map_add(&g_catalog.assets, path.filename, asset_id);
    }
}

void init_catalog_system()
{
    g_catalog = {};
    init_array(&g_catalog.folders,       g_heap);

    init_map(&g_catalog.assets,          g_heap);
    init_map(&g_catalog.textures,        g_heap);
    init_map(&g_catalog.meshes,          g_heap);
    init_map(&g_catalog.entities,        g_heap);

    init_map(&g_catalog.processes,       g_heap);
    init_array(&g_catalog.process_queue, g_heap);

    init_mutex(&g_catalog.mutex);

    array_add(&g_catalog.folders, resolve_folder_path(GamePath_data, "shaders", g_persistent));
    array_add(&g_catalog.folders, resolve_folder_path(GamePath_data, "textures", g_persistent));
    array_add(&g_catalog.folders, resolve_folder_path(GamePath_data, "models", g_persistent));
    array_add(&g_catalog.folders, resolve_folder_path(GamePath_data, "entities", g_persistent));

    map_add(&g_catalog.processes, "bmp", catalog_process_bmp);
    map_add(&g_catalog.processes, "ent", catalog_process_entity);
    map_add(&g_catalog.processes, "obj", catalog_process_obj);
    map_add(&g_catalog.processes, "fbx", catalog_process_fbx);
    map_add(&g_catalog.processes, "msh", catalog_process_msh);
    map_add(&g_catalog.processes, "glsl", catalog_process_glsl);

    init_array(&g_textures, g_heap);
    init_array(&g_meshes,   g_heap);

    for (i32 i = 0; i < g_catalog.folders.count; i++) {
        Array<FilePath> files = list_files(g_catalog.folders[i], g_heap);

        for (auto &p : files) {
            catalog_process_t **func = map_find(
                &g_catalog.processes,
                p.extension);

            if (func == nullptr) {
                LOG_ERROR("could not find process function for extension: %.*s",
                          p.extension.size,
                          p.extension.bytes);
                continue;
            }

            (*func)(p);
        }
    }

    create_catalog_thread(g_catalog.folders, &catalog_thread_proc);
}
