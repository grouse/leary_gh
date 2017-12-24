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
    isize length = 0;
    isize capacity = 0;
    char  *bytes = nullptr;
    Allocator *allocator = nullptr;

    String() {}

    String(String &other)
    {
        *this = other;
    }

    String(String &&other)
    {
        length    = other.length;
        capacity  = other.capacity;
        allocator = other.allocator;
        bytes     = other.bytes;

        other.bytes    = nullptr;
        other.length   = 0;
        other.capacity = 0;
    }

    String& operator=(const String &other)
    {
        length    = other.length;
        capacity  = other.length;
        allocator = other.allocator;

        if (allocator != nullptr) {
            bytes     = (char*)allocator->alloc(length);
            std::memcpy(bytes, other.bytes, length);
        }
        return *this;
    }

    ~String()
    {
        ASSERT(allocator != nullptr || bytes == nullptr);

        if (allocator != nullptr) {
            allocator->dealloc(bytes);
        }

        bytes = nullptr;
        capacity = 0;
        length = 0;
    }

    char& operator[] (isize i)
    {
        ASSERT(i < length);
        ASSERT(i >= 0);

        return bytes[i];
    }
};

struct StringView {
    const char *bytes = nullptr;
    isize length = 0;

    StringView() {}

    template<i32 N>
    StringView(const char (&str)[N])
    {
        length = N;
        bytes  = str;
    }

    StringView(const char *str)
    {
        bytes  = str;
        length = strlen(str);
    }

    StringView(isize length, const char *str)
    {
        bytes  = str;
        this->length = length;
    }

    const char& operator[] (i32 i)
    {
        ASSERT(i < length);
        ASSERT(i >= 0);

        return bytes[i];
    }
};

template<typename T>
void string_concat(String *str, T t)
{
    (void)str, (void)t;
    ASSERT(false);
}

template<>
void string_concat(String *str, String other)
{
    ASSERT((str->length + other.length) < str->capacity);
    char *ptr = str->bytes + str->length;
    for (i32 i = 0; i < other.length; i++) {
        *ptr++ = other[i];
    }

    str->length += other.length;
}

template<>
void string_concat(String *str, StringView other)
{
    ASSERT((str->length + other.length) < str->capacity);
    char *ptr = str->bytes + str->length;
    for (i32 i = 0; i < other.length; i++) {
        *ptr++ = other[i];
    }

    str->length += other.length;
}

template<>
void string_concat(String *str, const char16_t *other)
{
    wchar_t *ptr = other;
    while (*ptr) {
        u16 utf16 = *ptr;

        u16 utf16_hi_surrogate_start = 0xD800;
        u16 utf16_lo_surrogate_start = 0xDC00;
        u16 utf16_surrogate_end = 0xDFFF;

        u16 high_surrogate = 0;
        if (utf16 >= utf16_hi_surrogate_start &&
            utf16 < utf16_lo_surrogate_start)
        {
            high_surrogate = utf16;
            utf16 = *(++ptr);
        }

        u32 utf32 = utf16;
        if (utf16 >= utf16_lo_surrogate_start &&
            utf16 <= utf16_surrogate_end)
        {
            utf32  = (high_surrogate - utf16_hi_surrogate_start) << 10;
            utf32 |= utf16;
            utf32 += 0x1000000;
        }

        ptr++;
    }
}

template<>
void string_concat(String *str, const char *other)
{
    isize length = strlen(other);

    ASSERT((str->length + length) < str->capacity);
    char *ptr = str->bytes + str->length;
    for (i32 i = 0; i < length; i++) {
        *ptr++ = other[i];
    }

    str->length += length;
}


template<typename T, typename... Args>
void string_concat(String *str, T first, Args... rest)
{
    string_concat(str, first);
    string_concat(str, rest...);
}


template<typename T>
i32 string_length(T t)
{
    (void)t;
    return 0;
}

template<>
i32 string_length(const wchar_t *str)
{
    i32 length = 0;
    while (*str++) {
        length++;
    }
    return length;
}

template<>
i32 string_length(const char *str)
{
    i32 length = 0;
    while (*str++) {
        length++;
    }
    return length;
}

template<>
i32 string_length(String str)
{
    return (i32)str.length;
}

template<>
i32 string_length(StringView str)
{
    return (i32)str.length;
}

template<typename T, typename... Args>
i32 string_length(T first, Args... rest)
{
    return string_length(first) + string_length(rest...);
}

template<typename T>
String create_string(Allocator *a, T first)
{
    String str = {};
    str.allocator = a;

    i32 length = string_length(first) + 1;

    str.capacity = length;
    str.bytes  = (char*)a->alloc(length);
    string_concat(&str, first);

    return str;
}


template<typename T, typename... Args>
String create_string(Allocator *a, T first, Args... rest)
{
    String str = {};
    str.allocator = a;

    i32 length = string_length(first) + string_length(rest...) + 1;

    str.capacity = length;
    str.bytes  = (char*)a->alloc(length);
    string_concat(&str, first, rest...);

    return str;
}

#endif /* LEARY_STRING_H */

