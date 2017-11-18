/**
 * file:    platform_debug.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PLATFORM_DEBUG_H
#define PLATFORM_DEBUG_H

#if defined(__linux__)
#elif defined(_WIN32)
    #include <intrin.h>
#else
    #error "unsupported error"
#endif

#if defined(__linux__)
    #define DEBUG_BREAK()   asm("int $3")
    #define DEBUG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif defined(_WIN32)
    #define DEBUG_BREAK()       __debugbreak()
    #define DEBUG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #error "unsupported error"
#endif

enum LogChannel {
    Log_error,
    Log_warning,
    Log_info,
    Log_assert,
    Log_unimplemented
};

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          LogChannel channel,
                          const char *fmt, ...);

void platform_debug_print(const char *file,
                          u32 line,
                          const char *function,
                          const char *fmt, ...);

#endif /* PLATFORM_DEBUG_H */

