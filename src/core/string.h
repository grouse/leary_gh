/**
 * file:    string.h
 * created: 2017-11-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_STRING_H
#define LEARY_STRING_H

#include "leary_macros.h"

struct String {
    char  *bytes         = nullptr;
    Allocator *allocator = nullptr;
    i32 size             = 0;
    i32 capacity         = 0;

    String() {}

    String(String &other)
    {
        *this = other;
    }

    String(String &&other)
    {
        size      = other.size;
        capacity  = other.capacity;
        allocator = other.allocator;
        bytes     = other.bytes;

        other.bytes    = nullptr;
        other.size     = 0;
        other.capacity = 0;
    }

    String& operator=(const String &other)
    {
        size      = other.size;
        capacity  = other.size + 1;
        allocator = other.allocator;

        if (allocator != nullptr) {
            bytes     = (char*)allocator->alloc(capacity);
            std::memcpy(bytes, other.bytes, capacity);
        }
        return *this;
    }

    ~String()
    {
        ASSERT(allocator != nullptr || bytes == nullptr);

        if (allocator != nullptr) {
            allocator->dealloc(bytes);
        }

        bytes    = nullptr;
        capacity = 0;
        size     = 0;
    }

    char& operator[] (isize i)
    {
        ASSERT(i < size);
        ASSERT(i >= 0);

        return bytes[i];
    }
};

struct StringView {
    const char *bytes = nullptr;
    i32 size          = 0;

    StringView() {}

    template<i32 N>
    StringView(const char (&str)[N])
    {
        size  = N;
        bytes = str;
    }

    StringView(const char *str)
    {
        bytes = str;
        size  = (i32)strlen(str);
    }

    StringView(String other)
    {
        bytes = other.bytes;
        size  = other.size;
    }

    StringView(i32 size, const char *str)
    {
        bytes      = str;
        this->size = size;
    }

    const char& operator[] (i32 i)
    {
        ASSERT(i < size);
        ASSERT(i >= 0);

        return bytes[i];
    }
};

template<typename T>
i32 utf8_size(T)
{
    static_assert(false && "implement utf8_size for this type!");
    return 0;
}

template<>
i32 utf8_size(char)
{
    return 1;
}

template<>
i32 utf8_size(WCHAR *str)
{
    i32 size = 0;

    while (*str) {
        u16 utf16 = (u16)*str++;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *str++;
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        if (utf32 <= 0x007F) {
            // U+0000..U+007F
            // 00000000000000xxxxxxx = 0xxxxxxx
            size += 1;
        } else if (utf32 <= 0x07FF) {
            // U+0080..U+07FF
            // 0000000000yyyyyxxxxxx = 110yyyyy 10xxxxxx
            size += 2;
        } else if (utf32 <= 0x0FFFF) {
            // U+0800..U+D7FF, U+E000..U+FFFF
            // 00000zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
            size += 3;
        } else {
            // U+10000..U+10FFFF
            // uuuzzzzzzyyyyyyxxxxxx = 11110uuu 10zzzzzz 10yyyyyy 10xxxxxx
            size += 4;
        }
    }

    return size;
}

template<>
i32 utf8_size(char *str)
{
    i32 size = 0;
    while (*str++) {
        size++;
    }
    return size;
}

template<>
i32 utf8_size(const char *str)
{
    i32 size = 0;
    while (*str++) {
        size++;
    }
    return size;
}

template<>
i32 utf8_size(String str)
{
    return str.size;
}

template<>
i32 utf8_size(StringView str)
{
    return str.size;
}

template<typename T, typename... Args>
i32 utf8_size(T first, Args... rest)
{
    return utf8_size(first) + utf8_size(rest...);
}



template<typename T>
void string_concat(String *, T)
{
    static_assert(false && "implement string_concat for this type!");
}

template<>
void string_concat(String *str, char c)
{
    ASSERT((str->size + 1) <= str->capacity);
    char *ptr = str->bytes + str->size;
    *ptr++ = c;
    *ptr = '\0';
    str->size += 1;
}

template<>
void string_concat(String *str, String other)
{
    isize size = utf8_size(other);
    ASSERT((str->size + size) <= str->capacity);

    char *ptr = str->bytes + str->size;
    for (i32 i = 0; i < size; i++) {
        *ptr++ = other[i];
    }
    *ptr = '\0';

    str->size += other.size;
}

template<>
void string_concat(String *str, StringView other)
{
    i32 size = utf8_size(other);
    ASSERT((str->size + size) <= str->capacity);

    char *out = str->bytes + str->size;
    for (i32 i = 0; i < size; i++) {
        *out++ = other[i];
    }
    *out = '\0';

    str->size += size;
}

template<>
void string_concat(String *str, const char *other)
{
    i32 size = utf8_size(other);
    ASSERT((str->size + size) <= str->capacity);

    char *out = str->bytes + str->size;
    for (i32 i = 0; i < size; i++) {
        *out++ = other[i];
    }
    *out = '\0';

    str->size += size;
}

template<>
void string_concat(String *str, char *other)
{
    i32 size = utf8_size(other);
    ASSERT((str->size + size) <= str->capacity);

    char *out = str->bytes + str->size;
    for (i32 i = 0; i < size; i++) {
        *out++ = other[i];
    }
    *out= '\0';

    str->size += size;
}

template<>
void string_concat(String *str, WCHAR *other)
{
    i32 size = utf8_size(other);
    ASSERT((str->size + size) <= str->capacity);

    const WCHAR *ptr = other;
    char *out = str->bytes + str->size;

    while (*ptr) {
        u16 utf16 = *ptr++;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *ptr++;
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        if (utf32 <= 0x007F) {
            // U+0000..U+007F
            // 00000000000000xxxxxxx = 0xxxxxxx
            *out++ = (char)utf32;
        } else if (utf32 <= 0x07FF) {
            // U+0080..U+07FF
            // 0000000000yyyyyxxxxxx = 110yyyyy 10xxxxxx
            *out++ = (char)((utf32 >> 6)   | (3 << 6));
            *out++ = (char)((utf32) & 0x3F | (1 << 8));
        } else if (utf32 <= 0x0FFFF) {
            // U+0800..U+D7FF, U+E000..U+FFFF
            // 00000zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
            *out++ = (char)((utf32 >> 12)       | (7 << 5));
            *out++ = (char)((utf32 >> 6) & 0x3F | (1 << 8));
            *out++ = (char)((utf32)      & 0x3F | (1 << 8));
        } else {
            // U+10000..U+10FFFF
            // uuuzzzzzzyyyyyyxxxxxx = 11110uuu 10zzzzzz 10yyyyyy 10xxxxxx
            *out++ = (char)((utf32 >> 18)        | (15 << 4));
            *out++ = (char)((utf32 >> 12) & 0x3F | (1  << 8));
            *out++ = (char)((utf32 >> 6)  & 0x3F | (1  << 8));
            *out++ = (char)((utf32)       & 0x3F | (1  << 8));
        }
    }
    *out = '\0';

    str->size += size;
}

template<typename T, typename... Args>
void string_concat(String *str, T first, Args... rest)
{
    string_concat(str, first);
    string_concat(str, rest...);
}

template<typename T>
String create_string(Allocator *a, T first)
{
    String str = {};
    str.allocator = a;

    i32 size = utf8_size(first) + 1;

    str.capacity = size;
    str.bytes    = (char*)a->alloc(size);
    string_concat(&str, first);

    return str;
}

template<typename T, typename... Args>
String create_string(Allocator *a, T first, Args... rest)
{
    String str = {};
    str.allocator = a;

    i32 size = utf8_size(first) + utf8_size(rest...) + 1;

    str.capacity = size;
    str.bytes    = (char*)a->alloc(size);
    string_concat(&str, first, rest...);

    return str;
}

#endif /* LEARY_STRING_H */

