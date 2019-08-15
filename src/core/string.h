/**
 * file:    string.h
 * created: 2017-11-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

struct String {
    char  *bytes         = nullptr;
    Allocator *allocator = nullptr;
    // NOTE(jesper): size and capacity includes a terminating '\0'
    i32 size             = 0;
    i32 capacity         = 0;

    char& operator[] (isize i)
    {
        ASSERT(i < size);
        ASSERT(i >= 0);

        return bytes[i];
    }
};

struct StringView {
    const char *bytes = nullptr;
    // NOTE(jesper): size includes a terminating '\0'
    i32 size          = 0;

    StringView() {}

    StringView(const char *str)
    {
        size  = (i32)strlen(str) + 1;
        bytes = str;
    }

    StringView(const char *str, i32 size)
    {
        bytes = str;
        this->size = size;
    }

    StringView(String other)
    {
        size  = other.size;
        bytes = other.bytes;
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

String create_string(Allocator *a, const char* str);
String create_string(Allocator *a, StringView str);
String create_string(Allocator *a, String str);
String create_string(Allocator *a, std::initializer_list<StringView> args);

// NOTE(jesper): allocates the new string in temporary storage
String string_from_utf16(const u16 *in_str, i32 length);
