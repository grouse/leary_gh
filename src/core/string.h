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

    String& operator=(String &&other)
    {
        if (this == &other) {
            return *this;
        }

        size      = other.size;
        capacity  = other.capacity;
        allocator = other.allocator;
        bytes     = other.bytes;

        other.bytes    = nullptr;
        other.size     = 0;
        other.capacity = 0;

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
        size  = (i32)strlen(str);
        bytes = str;
    }

    StringView(String other)
    {
        size  = other.size;
        bytes = other.bytes;
    }

    StringView(i32 size, const char *str)
    {
        this->size = size;
        bytes      = str;
    }

    ~StringView()
    {
        size  = 0;
        bytes = nullptr;
    }

    const char& operator[] (i32 i)
    {
        ASSERT(i < size);
        ASSERT(i >= 0);

        return bytes[i];
    }
};

bool operator==(String lhs, String rhs);
bool operator==(StringView lhs, StringView rhs);

template<i32 N>
bool operator==(String lhs, const char (&str)[N])
{
    if (lhs.bytes == str && lhs.size == N) {
        return true;
    }

    return strncmp(lhs.bytes, str, N) == 0;
}

template<i32 N>
bool operator==(StringView lhs, const char (&str)[N])
{
    if (lhs.bytes == str && lhs.size == N) {
        return true;
    }

    return strncmp(lhs.bytes, str, N) == 0;
}

i32 utf8_size(char);
i32 utf8_size(WCHAR *str);
i32 utf8_size(char *str);
i32 utf8_size(const char *str);
i32 utf8_size(String str);
i32 utf8_size(StringView str);

void string_concat(String *str, char c);
void string_concat(String *str, String other);
void string_concat(String *str, StringView other);
void string_concat(String *str, const char *other);
void string_concat(String *str, char *other);
void string_concat(String *str, WCHAR *other);


template<typename T, typename... Args>
i32 utf8_size(T first, Args... rest)
{
    return utf8_size(first) + utf8_size(rest...);
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

