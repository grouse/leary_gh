/**
 * file:    string.cpp
 * created: 2017-08-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

bool operator==(String lhs, String rhs)
{
    if (lhs.bytes == rhs.bytes && lhs.size == rhs.size) {
        return true;
    }

    if (lhs.size != rhs.size) {
        return false;
    }

    return strncmp(lhs.bytes, rhs.bytes, lhs.size - 1) == 0;
}

bool operator==(StringView lhs, StringView rhs)
{
    if (lhs.bytes == rhs.bytes && lhs.size == rhs.size) {
        return true;
    }

    if (lhs.size != rhs.size) {
        return false;
    }

    return strncmp(lhs.bytes, rhs.bytes, lhs.size - 1) == 0;
}

String string_from_utf16(const u16 *in_str, i32 length)
{
    String str = {};
    str.bytes     = (char*)alloc(g_frame, length + 1);
    str.capacity  = length + 1;
    str.allocator = g_frame;

    const u16 *ptr = in_str;
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

            if (str.size + 1 > str.capacity) {
                str.bytes = (char*)realloc(str.allocator, str.bytes, str.size + 1);
            }
            str.bytes[str.size++] = (char)utf32;
        } else if (utf32 <= 0x07FF) {
            // U+0080..U+07FF
            // 0000000000yyyyyxxxxxx = 110yyyyy 10xxxxxx

            if (str.size + 2 > str.capacity) {
                str.bytes = (char*)realloc(str.allocator, str.bytes, str.size + 2);
            }
            str.bytes[str.size++] = (char)((utf32 >> 6) | (3 << 6));
            str.bytes[str.size++] = (char)((utf32 & 0x3F) | (1 << 8));
        } else if (utf32 <= 0x0FFFF) {
            // U+0800..U+D7FF, U+E000..U+FFFF
            // 00000zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx

            if (str.size + 3 > str.capacity) {
                str.bytes = (char*)realloc(str.allocator, str.bytes, str.size + 3);
            }
            str.bytes[str.size++] = (char)((utf32 >> 12) | (7 << 5));
            str.bytes[str.size++] = (char)(((utf32 >> 6) & 0x3F) | (1 << 8));
            str.bytes[str.size++] = (char)((utf32 & 0x3F) | (1 << 8));
        } else {
            // U+10000..U+10FFFF
            // uuuzzzzzzyyyyyyxxxxxx = 11110uuu 10zzzzzz 10yyyyyy 10xxxxxx

            if (str.size + 4 > str.capacity) {
                str.bytes = (char*)realloc(str.allocator, str.bytes, str.size + 4);
            }
            str.bytes[str.size++] = (char)((utf32 >> 18) | (15 << 4));
            str.bytes[str.size++] = (char)(((utf32 >> 12) & 0x3F) | (1 << 8));
            str.bytes[str.size++] = (char)(((utf32 >> 6) & 0x3F) | (1 << 8));
            str.bytes[str.size++] = (char)(((utf32) & 0x3F) | (1 << 8));
        }
    }

    ASSERT(str.size < str.capacity);
    str.bytes[str.size++] = '\0';

    return str;
}

String create_string(Allocator *a, const char* str)
{
    String result;
    isize required_size = strlen(str) + 1;
    result.bytes    = (char*)alloc(a, required_size);
    result.size     = required_size;
    result.capacity = required_size;
    memcpy(result.bytes, str, required_size);
    return result;
}

String create_string(Allocator *a, StringView str)
{
    String result;
    result.bytes    = (char*)alloc(a, str.size);
    result.size     = str.size;
    result.capacity = str.size;
    memcpy(result.bytes, str.bytes, str.size);
    return result;
}

String create_string(Allocator *a, String str)
{
    String result;
    result.bytes    = (char*)alloc(a, str.size);
    result.size     = str.size;
    result.capacity = str.size;
    memcpy(result.bytes, str.bytes, str.size);

    return result;
}

String create_string(Allocator *a, std::initializer_list<StringView> args)
{
    String result;

    const StringView *start = args.begin();
    const StringView *end = start + args.size();

    isize required_size = 0;
    for (const StringView *it = start; it != end; ++it) {
        required_size += it->size - 1;
    }
    required_size += 1;

    result.bytes    = (char*)alloc(a, required_size);
    result.size     = 0;
    result.capacity = required_size;

    for (const StringView *it = start; it != end; ++it) {
        memcpy(result.bytes + result.size, it->bytes, it->size - 1);
        result.size += it->size - 1;
    }
    result.bytes[result.size++] = '\0';
    return result;
}
