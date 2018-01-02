/**
 * file:    file.cpp
 * created: 2017-12-24
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "file.h"

bool operator==(Path lhs, Path rhs)
{
    return lhs.absolute == rhs.absolute;
}
