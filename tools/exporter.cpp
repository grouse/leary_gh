#include <stdio.h>

#define DL_EXPORT extern "C" __declspec(dllexport)

DL_EXPORT
void export_blender_mdl(const char* path, int d)
{
    FILE *f = fopen(path, "w");
    fprintf(f, "%d\n", d);
    fclose(f);
}
