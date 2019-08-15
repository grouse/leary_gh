#include "build_config.h"

// TODO(jesper): replace with my own stuff
#include <stdint.h>
#include "core/types.h"

#if defined(__linux__)
#elif defined(_WIN32)
    #include <Windows.h>
#else
    #error "unsupported platform"
#endif

#define _USE_MATH_DEFINES
#include <math.h>

#include <initializer_list>
#include <stdio.h>
#include <stdlib.h>

#include "leary_macros.h"
#include "platform/platform_debug.h"
#include "platform/thread.h"
#include "core/allocator.h"
#include "platform/platform.h"

#if defined(__linux__)
    #include "platform/linux_main.cpp"
#elif defined(_WIN32)
    #include "platform/win32_main.cpp"
#else
    #error "unsupported platform"
#endif