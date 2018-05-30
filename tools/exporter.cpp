#include <stdio.h>

#define LEARY_ENABLE_LOGGING 0
#define LEARY_ENABLE_PROFILING 0

#include "platform/platform.h"

#if defined(_WIN32)
    #include "platform/win32_debug.cpp"
    #include "platform/win32_file.cpp"
    #include "platform/win32_thread.cpp"
#else
    #error "unsupported platform"
#endif

#include "leary.h"
#include "core/types.h"
#include "leary_macros.h"
#include "core/maths.cpp"
#include "core/file.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"
#include "core/string.cpp"

#undef DL_EXPORT
#define DL_EXPORT extern "C" __declspec(dllexport)

struct BlenderVertex {
    Vector3 point;
    Vector3 normal;
};

struct BlenderModel {
    const char *name;
    Vector3    scale;

    i32           vertex_count;
    BlenderVertex *vertices;

    i32      index_count;
    Vector3i *indices;

    i32     uv_count;
    Vector2 *uvs;
};

FilePath g_export_path;
Array<BlenderModel> g_models;
Allocator *g_export_alloc;
Settings g_settings;


DL_EXPORT
void init(const char* path)
{
    isize size      = 1024*1024*64;
    g_export_alloc  = new Allocator();
    *g_export_alloc = linear_allocator(malloc(size), size);

    g_export_path = create_file_path(g_export_alloc, path);
    init_array(&g_models, g_export_alloc);
}

DL_EXPORT
void add_model(BlenderModel model)
{
    array_add(&g_models, model);
}

DL_EXPORT
void finish()
{
    FILE *f = fopen(g_export_path.absolute.bytes, "w");

    for (BlenderModel mdl : g_models) {
        fprintf(f, "object %s;\n", mdl.name);
        fprintf(f, "scale %f, %f, %f;\n", mdl.scale.x, mdl.scale.y, mdl.scale.z);

        for (i32 i = 0; i < mdl.vertex_count; i++) {
            BlenderVertex v = mdl.vertices[i];
            fprintf(f, "p %f, %f, %f;\n", v.point.x, v.point.y, v.point.z);
        }

        for (i32 i = 0; i < mdl.vertex_count; i++) {
            BlenderVertex v = mdl.vertices[i];
            fprintf(f, "n %f, %f, %f;\n", v.normal.x, v.normal.y, v.normal.z);
        }

        for (i32 i = 0; i < mdl.index_count; i++) {
            Vector3i v = mdl.indices[i];
            fprintf(f, "vi %d, %d, %d;\n", v.x, v.y, v.z);
        }

        for (i32 i = 0; i < mdl.uv_count; i++) {
            Vector2 uv = mdl.uvs[i];
            fprintf(f, "uv %f, %f;\n", uv.x, uv.y);
        }
    }

    fclose(f);
}
