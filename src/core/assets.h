/**
 * file:    assets.h
 * created: 2017-08-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_ASSETS_H
#define LEARY_ASSETS_H

#include "core.h"

#define ASSET_INVALID_ID (-1)

#define CATALOG_PROCESS_FUNC(fname) void fname(FilePath path)
typedef CATALOG_PROCESS_FUNC(catalog_process_t);

typedef i32 AssetID;
typedef i32 TextureID;
typedef i32 EntityID;
typedef i32 MeshID;

struct Mesh {
    AssetID asset_id = ASSET_INVALID_ID;

    Array<f32> vertices;
    Array<u32> indices;
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
    String     mesh     = {};
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


#endif /* LEARY_ASSETS_H */

