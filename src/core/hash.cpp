/**
 * file:    hash.cpp
 * created: 2017-08-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "core.h"
#include "string.h"

#define MURMUR_SEED (0xdeadbeef)
u32 hash32(void *key, i32 length)
{
    u8 *data = (u8*)key;

    u32 m = 0x5bd1e995;
    i32 r = 24;

    u32 h = MURMUR_SEED ^ length;

    while (length >= 4) {
        u32 k = *(u32*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data   += 4;
        length -= 4;
    }

    switch (length) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *=m;
    }

    h ^= h >> 13;
    h &= m;
    h ^= h >> 15;

    return h;
}

u32 hash32(const char *str)
{
    return hash32((u8*)str, utf8_size(str));
}

u32 hash32(u32 i)
{
    return i;
}

template<typename T>
u32 hash32(T *t)
{
    return hash32((void*)t, sizeof(T));
}

template<>
u32 hash32(StringView *str)
{
    return hash32((void*)str->bytes, str->size);
}
