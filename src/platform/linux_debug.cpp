/**
 * file:    linux_debug.cpp
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2015-2018 - all rights reserved
**/

#if LEARY_ENABLE_LOGGING
#define DEBUG_BUFFER_SIZE (2048)

extern Allocator *g_debug_frame;

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
    char *message = (char*)alloc(g_debug_frame, DEBUG_BUFFER_SIZE);
    char *buffer  = (char*)alloc(g_debug_frame, DEBUG_BUFFER_SIZE);

    va_start(args, fmt);
    i32 length = vsnprintf(message, DEBUG_BUFFER_SIZE, fmt, args);
    va_end(args);
    ASSERT(length < DEBUG_BUFFER_SIZE);

    length = snprintf(buffer, DEBUG_BUFFER_SIZE, "%s:%d: %s: [%s] %s\n",
                      file, line, channel_str, function, message);
    ASSERT(length < DEBUG_BUFFER_SIZE);
    write(1, buffer, length);
}

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          const char *fmt, ...)
{
    const char *channel_str = log_channel_string(Log_info);

    va_list args;
    char *message = (char*)alloc(g_debug_frame, DEBUG_BUFFER_SIZE);
    char *buffer  = (char*)alloc(g_debug_frame, DEBUG_BUFFER_SIZE);

    va_start(args, fmt);
    i32 length = vsnprintf(message, DEBUG_BUFFER_SIZE, fmt, args);
    va_end(args);
    ASSERT(length < DEBUG_BUFFER_SIZE);

    length = snprintf(buffer, DEBUG_BUFFER_SIZE, "%s:%d: %s: [%s] %s\n",
                      file, line, channel_str, function, message);
    ASSERT(length < DEBUG_BUFFER_SIZE);
    write(1, buffer, length);
}

#endif // LEARY_ENABLE_LOGGING
