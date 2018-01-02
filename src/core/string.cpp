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
