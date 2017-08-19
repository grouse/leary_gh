/**
 * file:    hash.cpp
 * created: 2017-08-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#define MURMUR_SEED (0xdeadbeef)
constexpr u32 hash32(void *key, i32 length)
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

constexpr u32 hash32(const char *str)
{
    return hash32((u8*)str, string_length(str));
}

