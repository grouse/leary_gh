/**
 * file:    main.cpp
 * created: 2017-08-21
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "core/types.h"
#include "platform/platform.h"

#if defined(_WIN32)
	#include "platform/win32_debug.cpp"
	#include "platform/win32_file.cpp"
#elif defined(__linux__)
	#include "platform/linux_debug.cpp"
	#include "platform/linux_file.cpp"
#else
	#error "unsupported platform"
#endif

#include "core/tokenizer.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"

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

#include "test_allocator.cpp"

int main()
{
    bool result = true;
    result = result && allocators_test();
    return 0;
}

