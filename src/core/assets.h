/**
 * file:    assets.h
 * created: 2017-08-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#define ASSET_INVALID_ID (-1)

#define CATALOG_PROCESS_FUNC(fname) void fname(FilePath path)
typedef CATALOG_PROCESS_FUNC(catalog_process_t);

#define DEFINE_ID_TYPE(name, type)\
    struct name {\
        type id;\
        name() {}\
        name(const type other)\
        {\
            id = other;\
        }\
        name(const name &other)\
        {\
            id = other.id;\
        }\
        name(const name &&other)\
        {\
            id = other.id;\
        }\
        name& operator=(const name &other)\
        {\
            id = other.id;\
            return *this;\
        }\
        name& operator=(const name &&other)\
        {\
            id = other.id;\
            return *this;\
        }\
        name& operator++()\
        {\
            id += 1;\
            return *this;\
        }\
        name operator++(int)\
        {\
            name tmp(*this);\
            id += 1;\
            return tmp;\
        }\
        operator int()\
        {\
            return id;\
        }\
    }

DEFINE_ID_TYPE(AssetID,   i32);
DEFINE_ID_TYPE(TextureID, i32);
DEFINE_ID_TYPE(EntityID,  i32);
DEFINE_ID_TYPE(MeshID,    i32);

struct Mesh {
    AssetID asset_id = ASSET_INVALID_ID;

    Array<Vector3> points;
    Array<Vector3> normals;
    Array<Vector3> tangents;
    Array<Vector3> bitangents;
    Array<Vector2> uvs;
    Array<u32> indices;

    struct {
        VulkanBuffer points;
        VulkanBuffer normals;
        VulkanBuffer tangents;
        VulkanBuffer bitangents;
        VulkanBuffer uvs;
    } vbo;

    VulkanBuffer ibo;
    u32 element_count;
};

struct TextureData {
    i32      width;
    i32      height;
    VkFormat format;
    isize    size;
    void     *pixels;
};

struct SoundSample {
    i16 left  = 0;
    i16 right = 0;
};


struct SoundData {
    i32 sample_rate;
    i32 num_channels;
    i32 bits_per_sample;
    i32 num_samples;
    i16 *samples;
};

struct TextureAsset {
    AssetID asset_id = ASSET_INVALID_ID;
    GfxTexture gfx_texture;
};

struct Texture {
    AssetID asset_id = ASSET_INVALID_ID;
    u32   width;
    u32   height;

    // TODO(jesper): we might not want these in this structure? I think the only
    // place that really needs it untemporarily is the heightmap, the other
    // places we'll just be uploading straight to the GPU and no longer require
    // the data
    isize size;
    void  *data;

    VkFormat       format;
    VkImage        image;
    VkImageView    image_view;
    VkDeviceMemory memory;
};

struct EntityData {
    bool       valid    = false;
    Vector3    position = {};
    Vector3    scale    = {};
    Quaternion rotation = {};
    MeshID     mesh_id = ASSET_INVALID_ID;
    String     mesh     = {};
    Array<String> textures;
};

struct Catalog {
    Array<FolderPath> folders;

    AssetID next_asset_id = 0;
    RHHashMap<StringView, catalog_process_t*> processes;

    RHHashMap<StringView, AssetID> assets;
    RHHashMap<AssetID, TextureID>  textures;
    RHHashMap<AssetID, EntityID>   entities;
    RHHashMap<AssetID, MeshID>     meshes;

    Mutex mutex;
    Array<FilePath> process_queue;
};

Mesh* find_mesh(MeshID mesh_id);
Mesh* find_mesh(StringView name);

#define CATALOG_CALLBACK(fname)  void fname(FilePath path)
typedef CATALOG_CALLBACK(catalog_callback_t);

void create_catalog_thread(Array<FolderPath> folders, catalog_callback_t *callback);
