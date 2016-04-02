/**
 * @file:   debug.h
 * @author: Jesper Stefansson (grouse)
 * @email:  jesper.stefansson@gmail.com
 *
 * Copyright (c) 2015-2016 Jesper Stefansson
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef LEARY_LEARY_DEBUG_H
#define LEARY_LEARY_DEBUG_H

#include <stdint.h>

enum eLogType : uint8_t
{
    Error    = (1 << 0),
    Warning  = (1 << 1),
    Info     = (1 << 2),
    Assert   = (1 << 3),
    Any      = (1 << 4) - 1
};

namespace debug
{
    void printf(const char*      func,
                const uint32_t&  line,
                const char*      file,
                const char*      fmt, ...);

    void printf(eLogType chan,
                const char*      func,
                const uint32_t&  line,
                const char*      file,
                const char*      fmt, ...);
}




#endif // LEARY_LEARY_DEBUG_H
