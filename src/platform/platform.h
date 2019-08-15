/**
 * file:    platform_main.h
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2015-2018 - all rights reserved
 */

#if defined(__linux__)
extern "C" {
    #define EXIT_SUCCESS 0
    void exit(i32);
    char* getenv(const char*);
    char* realpath(const char *path, char *resolved_path);
    void* malloc(size_t);
    void free(void*);

}
#elif defined(_WIN32)
#else
    #error "unsupported platform"
#endif

// -- platform generic defines
#ifndef INTROSPECT
#define INTROSPECT
#endif

#define PLATFORM_PRE_RELOAD_FUNC(fname) void  fname(PlatformState *platform)
#define PLATFORM_RELOAD_FUNC(fname)     void  fname(PlatformState *platform)
#define PLATFORM_UPDATE_FUNC(fname)     void  fname(PlatformState *platform, f32 dt)

// -- platform specific defines
#if defined(__linux__)
    #define DLL_EXPORT extern "C"

    #define FILE_SEP "/"
    #define FILE_EOL "\n"

    #define PACKED(decl) decl __attribute__((__packed__))

    #define PLATFORM_INIT_FUNC(fname)       void fname(PlatformState *platform)
#elif defined(_WIN32)
    #define DLL_EXPORT extern "C" __declspec(dllexport)

    #define FILE_SEP "\\"
    #define FILE_EOL "\r\n"

    #define PACKED(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))

    #define PLATFORM_INIT_FUNC(fname)       void fname(PlatformState *platform, HINSTANCE instance)
#else
    #error "unsupported platform"
#endif



// -- platform specific types
#if defined(__linux__)

struct NativePlatformState {
    Window     window;
    Display    *display;
    XkbDescPtr xkb;
    i32        xinput2;
    Cursor     hidden_cursor;

    Atom WM_DELETE_WINDOW;

    struct {
        i32 x;
        i32 y;
    } mouse;
};

#elif defined(_WIN32)

struct NativePlatformState {
    HINSTANCE hinstance;
    HWND      hwnd;
};
#else
    #error "unsupported platform"
#endif

// -- platform generic types
struct PlatformState {
    NativePlatformState native;
    
    struct {
        Allocator heap;
        Allocator debug_frame;
        Allocator frame;
        Allocator persistent;
        Allocator stack;
        Allocator system;
    } allocators;
    
    bool raw_mouse = false;
    
    struct {
        void            *game;
        
        Allocator *heap;
        Allocator *frame;
        Allocator *debug_frame;
        Allocator *persistent;
        Allocator *stack;
        Allocator *system_alloc;
    } reload_state;
};
