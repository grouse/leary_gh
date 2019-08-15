/**
 * file:    file.cpp
 * created: 2017-12-24
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

bool operator==(FilePath lhs, FilePath rhs)
{
    return lhs.absolute == rhs.absolute;
}

bool operator==(FolderPath lhs, FolderPath rhs)
{
    return lhs.absolute == rhs.absolute;
}

void resolve_filename_ext(
    StringView absolute,
    StringView *filename,
    StringView *extension)
{
    i32 slash = 0;
    i32 ext   = 0;
    for (i32 i = 0; i < absolute.size; i++) {
        if (absolute[i] == '/' || absolute[i] == '\\') {
            slash = i;
        } else if (absolute[i] == '.') {
            ext = i;
        }
    }

    *filename = {
        absolute.bytes + slash + 1,
        absolute.size  - slash - 1
    };

    if (ext != 0) {
        *extension = {
            absolute.bytes + ext + 1,
            absolute.size  - ext - 1
        };
    }
}
