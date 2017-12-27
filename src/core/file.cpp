/**
 * file:    file.cpp
 * created: 2017-12-24
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "file.h"

Path create_path(const char *str, Allocator *a)
{
    Path p = {};
    p.absolute = create_string(a, str);

    char *ptr = (char*)str;
    while (*ptr) {
        if ((*ptr == '/' || *ptr == '\\') && *(ptr + 1)) {
            p.filename.bytes = ptr+1;
        }
        ptr++;
    }

    if (p.filename.bytes != nullptr) {
        p.filename.size = utf8_size(p.filename.bytes);

        i32 ext = 0;
        for (i32 i = 0; i < p.filename.size; i++) {
            if (p.filename[i] == '.') {
                ext = i;
            }
        }

        if (ext != 0) {
            p.extension = { p.filename.size - ext + 1, p.filename.bytes + ext };
        }
    }

    return p;
}
