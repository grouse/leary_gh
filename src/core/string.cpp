/**te
 * file:    string.cpp
 * created: 2017-08-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "string.h"

Path create_path(const char *str)
{
    Path p = {};

    char *ptr = (char*)str;
    p.absolute = { (isize)strlen(str), (char*)ptr };

    while (*ptr) {
        if ((*ptr == '/' || *ptr == '\\') && *(ptr + 1)) {
            p.filename.bytes = ptr+1;
        }
        ptr++;
    }

    if (p.filename.bytes != nullptr) {
        p.filename.length = strlen(p.filename.bytes);

        isize ext = 0;
        for (i32 i = 0; i < p.filename.length; i++) {
            if (p.filename[i] == '.') {
                ext = i;
            }
        }

        if (ext != 0) {
            p.extension = { p.filename.length - ext + 1, p.filename.bytes + ext };
        }
    }

    return p;
}

i32 string_length(const char *str)
{
    i32 length = 0;
    while (*str++) {
        length++;
    }
    return length;
}
