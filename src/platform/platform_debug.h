/**
 * file:    platform_debug.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#if defined(__linux__)
    #define DEBUG_BREAK()   asm("int $3")
    #define DEBUG_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#elif defined(_WIN32)
    #define DEBUG_BREAK()       __debugbreak()
    #define DEBUG_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
    #error "unsupported error"
#endif

#if defined(_WIN32)
char* win32_system_error_message(DWORD error);
#endif
