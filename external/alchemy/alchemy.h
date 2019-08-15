#ifndef ALCHEMY_H
#define ALCHEMY_H

#define ALC_MESH_IDENTIFIER 0xdeadbeef

#if defined(ALCHEMY_BUILD_LIB)
#define ALC_API extern "C" __declspec(dllexport)
#else
#define ALC_API extern "C" __declspec(dllexport)
#endif

enum AlcMeshFlags {
    ALC_MESH_FLAG_POINT_BIT  = 1 << 0,
    ALC_MESH_FLAG_UV_BIT     = 1 << 1,
    ALC_MESH_FLAG_NORMAL_BIT = 1 << 2,
};

enum AlcResult {
    ALC_RESULT_SUCCESS,
    ALC_RESULT_ERROR,
    ALC_RESULT_ERROR_FBX_VERSION_UNSUPPORTED,
};

struct AlcMeshHeader {
    u32 identifier;
    u32 version;
    u32 flags;
    u32 num_vertices;
};

ALC_API
AlcResult alc_convert_fbx(const char *file, const char *output_path, const char *prefix);

#endif /* ALCHEMY_H */
