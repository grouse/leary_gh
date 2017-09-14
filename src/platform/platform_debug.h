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
    #include <x86intrin.h>
#elif defined(_WIN32)
    #include <intrin.h>
#else
    #error "unsupported error"
#endif

#include <string.h>

#if defined(__linux__)
    #define DEBUG_BREAK()   asm("int $3")
    #define DEBUG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

    #define rdtsc() __rdtsc()
#elif defined(_WIN32)
    #define DEBUG_BREAK()       __debugbreak()
    #define DEBUG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

    #define rdtsc() __rdtsc()
#else
    #error "unsupported error"
#endif

#define DEBUG_LOG(...)        platform_debug_print(DEBUG_FILENAME, __LINE__, __FUNCTION__, __VA_ARGS__)
#define DEBUG_UNIMPLEMENTED() DEBUG_LOG(Log_unimplemented, "fixme! stub");

#define DEBUG_BUFFER_SIZE (2048)

enum LogChannel {
    Log_error,
    Log_warning,
    Log_info,
    Log_assert,
    Log_unimplemented
};

static void platform_debug_print(const char *file,
                                 u32 line,
                                 const char *function,
                                 LogChannel channel,
                                 const char *fmt, ...);

static void platform_debug_print(const char *file,
                                 u32 line,
                                 const char *function,
                                 const char *fmt, ...);

#endif /* PLATFORM_DEBUG_H */

