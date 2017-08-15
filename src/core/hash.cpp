/**
 * file:    hash.cpp
 * created: 2017-08-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

u64 hash64(const char *str)
{
    u64 h = 0;

    // TODO(jesper): better hash!
    while (*str) {
        h += *str++ % 26;
    }

    return h;
}

