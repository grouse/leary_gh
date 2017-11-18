/**
 * file:    main.cpp
 * created: 2017-08-21
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "platform/platform.h"
#include "leary.h"

#include "core/types.h"

#include "core/lexer.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"

LinearAllocator *g_debug_frame;

#if defined(_WIN32)
    #include "platform/win32_debug.cpp"
    #include "platform/win32_file.cpp"
#elif defined(__linux__)
    #include "platform/linux_debug.cpp"
    #include "platform/linux_file.cpp"
#else
    #error "unsupported platform"
#endif

#define TEST_START(name) printf("-- running test: %s\n", name)

#define CHECK(r, c) \
    do { \
        bool tmp = (c); \
        r = r && tmp; \
        if (!tmp) { \
            printf("ERROR: " #c "\n"); \
        } else { \
            printf("OK   : " #c "\n"); \
        } \
    } while(0)

#include "test_array.cpp"
#include "test_allocator.cpp"

int main()
{
    isize debug_frame_size = 64  * 1024 * 1024;
    void *debug_frame_mem = malloc(debug_frame_size);
    g_debug_frame = new LinearAllocator(debug_frame_mem, debug_frame_size);

    bool result = true;
    result = result && test_allocators();
    result = result && test_array();
    return 0;
}

