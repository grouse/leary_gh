/**
 * file:    main.cpp
 * created: 2017-08-22
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <time.h>
#include "core/types.h"
#include "platform/platform.h"

#include "core/tokenizer.cpp"
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


timespec get_time()
{
    timespec ts;
    i32 result = clock_gettime(CLOCK_MONOTONIC, &ts);
    (void)result;
    return ts;
}

i64 get_time_difference(timespec start, timespec end)
{
    i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
                     (end.tv_nsec - start.tv_nsec);
    return difference;
}

#include "benchmark_array.cpp"

int main()
{
    isize debug_frame_size = 64  * 1024 * 1024;
    void *debug_frame_mem = malloc(debug_frame_size);
    g_debug_frame = new LinearAllocator(debug_frame_mem, debug_frame_size);

    benchmark_array();
    return 0;
}
