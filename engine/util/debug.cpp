/**
 * @file:   debug.cpp
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
**/

#include "prefix.h"
#include "debug.h"

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_BUFFER_SIZE (256)

namespace debug
{
    void print_internal(eLogType         type,
                         const char*     func,
                         const uint32_t& line,
                         const char*     file,
                         const char*     msg)
    {
#if LEARY_LOG_ENABLE
        char c;
        switch (type) {
		case eLogType::Info:
            c = (LEARY_LOG_FILTER & eLogType::Info)    ? 'I' : '?';
            break;
        case eLogType::Error:
            c = (LEARY_LOG_FILTER & eLogType::Error)   ? 'E' : '?';
            break;
        case eLogType::Warning:
            c = (LEARY_LOG_FILTER & eLogType::Warning) ? 'W' : '?';
            break;
        case eLogType::Assert:
            c = (LEARY_LOG_FILTER & eLogType::Assert)  ? 'A' : '?';
            break;
        default:
            c = '?';
            break;
        }

        // Don't log if unknown, happens when filtering is enabled
        if (c == '?') return;
        ::printf("[%c] %s in %s:%d - %s\n", c, func, file, line, msg);
#endif
    }


    void printf(const char*     func,
                const uint32_t& line,
                const char*     file,
                const char*     fmt, ...)
    {
#if LEARY_LOG_ENABLE
        va_list args;
        va_start(args, fmt);

        char buffer[DEBUG_BUFFER_SIZE];
        vsprintf(buffer, fmt, args);

        va_end(args);

        print_internal(eLogType::Info, func, line, file, buffer);
#endif
    }

    void printf(eLogType         type,
                const char*      func,
                const uint32_t&  line,
                const char*      file,
                const char*      fmt, ...)
    {
#if LEARY_LOG_ENABLE
        va_list args;
        va_start(args, fmt);

        char buffer[DEBUG_BUFFER_SIZE];
        vsprintf(buffer, fmt, args);

        va_end(args);

        print_internal(type, func, line, file, buffer);
#endif
    }
}