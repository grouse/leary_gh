/**
 * @file:   linux_debug.cpp
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

#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#include "platform_debug.h"

#if LEARY_ENABLE_LOGGING
#define DEBUG_BUFFER_SIZE (2048)

extern LinearAllocator *g_debug_frame;

const char* log_channel_string(LogChannel channel)
{
    switch (channel) {
    case Log_info:          return "info";
    case Log_error:         return "error";
    case Log_warning:       return "warning";
    case Log_assert:        return "assert";
    case Log_unimplemented: return "unimplemented";
    default:                return "";
    }
}

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          LogChannel channel,
                          const char *fmt, ...)
{
    const char *channel_str = log_channel_string(channel);

    va_list args;
    char *message = (char*)g_debug_frame->alloc(DEBUG_BUFFER_SIZE);
    char *buffer  = (char*)g_debug_frame->alloc(DEBUG_BUFFER_SIZE);

    va_start(args, fmt);
    i32 length = vsnprintf(message, DEBUG_BUFFER_SIZE, fmt, args);
    va_end(args);
    assert(length < DEBUG_BUFFER_SIZE);

    length = snprintf(buffer, DEBUG_BUFFER_SIZE, "%s:%d: %s: [%s] %s\n",
                      file, line, channel_str, function, message);
    assert(length < DEBUG_BUFFER_SIZE);
    write(1, buffer, length);
}

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          const char *fmt, ...)
{
    const char *channel_str = log_channel_string(Log_info);

    va_list args;
    char *message = (char*)g_debug_frame->alloc(DEBUG_BUFFER_SIZE);
    char *buffer  = (char*)g_debug_frame->alloc(DEBUG_BUFFER_SIZE);

    va_start(args, fmt);
    i32 length = vsnprintf(message, DEBUG_BUFFER_SIZE, fmt, args);
    va_end(args);
    assert(length < DEBUG_BUFFER_SIZE);

    length = snprintf(buffer, DEBUG_BUFFER_SIZE, "%s:%d: %s: [%s] %s\n",
                      file, line, channel_str, function, message);
    assert(length < DEBUG_BUFFER_SIZE);
    write(1, buffer, length);
}

#endif // LEARY_ENABLE_LOGGING
