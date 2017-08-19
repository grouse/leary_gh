/**
 * file:    string.cpp
 * created: 2017-08-17
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "string.h"

constexpr i32 string_length(const char *str)
{
    i32 length = 0;
    while (*str++) {
        length++;
    }
    return length;
}
