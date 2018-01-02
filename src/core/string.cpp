/**
 * file:    string.cpp
 * created: 2017-08-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "string.h"

bool operator==(String lhs, String rhs)
{
    if (lhs.bytes == rhs.bytes && lhs.size == rhs.size) {
        return true;
    }

    if (lhs.size != rhs.size) {
        return false;
    }

    return strncmp(lhs.bytes, rhs.bytes, lhs.size) == 0;
}

bool operator==(StringView lhs, StringView rhs)
{
    if (lhs.bytes == rhs.bytes && lhs.size == rhs.size) {
        return true;
    }

    if (lhs.size != rhs.size) {
        return false;
    }

    return strncmp(lhs.bytes, rhs.bytes, lhs.size) == 0;
}

char16_t* utf16_string(String str, Allocator *a)
{
    (void)a;
    return nullptr;
}

i32 utf8_size(char)
{
    return 1;
}

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

i32 utf8_size(char *str)
{
    i32 size = 0;
    while (*str++) {
        size++;
    }
    return size;
}

i32 utf8_size(const char *str)
{
    i32 size = 0;
    while (*str++) {
        size++;
    }
    return size;
}

i32 utf8_size(String str)
{
    return str.size;
}

i32 utf8_size(StringView str)
{
    return str.size;
}

void string_concat(String *str, char c)
{
    ASSERT((str->size + 1) <= str->capacity);
    char *ptr = str->bytes + str->size;
    *ptr++ = c;
    *ptr = '\0';
    str->size += 1;
}

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

