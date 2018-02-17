/**
 * file:    leary_macros.h
 * created: 2017-09-14
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_MACROS_H
#define LEARY_MACROS_H

#include <utility>

#include "platform/platform_debug.h"

#define ARRAY_SIZE(a) (isize)(sizeof((a)) / sizeof((a)[0]))

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

template <typename F>
struct Defer {
    Defer(F f) : f(f) {}
    ~Defer() { f(); }
    F f;
};

template <typename F>
Defer<F> defer_create( F f ) {
    return Defer<F>( f );
}

#define defer__(line) defer_ ## line
#define defer_(line) defer__( line )

struct DeferDummy { };
template<typename F>
Defer<F> operator+ (DeferDummy, F&& f)
{
    return defer_create<F>(std::forward<F>(f));
}

#define defer auto defer_( __LINE__ ) = DeferDummy( ) + [&]( )

#if LEARY_ENABLE_LOGGING
#define LOG(...) platform_debug_print(DEBUG_FILENAME, __LINE__, __FUNCTION__, __VA_ARGS__)
#define LOG_UNIMPLEMENTED() LOG(Log_unimplemented, "fixme! stub");
#else
#define LOG(...) do {} while(0)
#define LOG_UNIMPLEMENTED() do {} while(0)
#endif

#define PARSE_ERROR(path, lexer, msg) \
    LOG(Log_error,\
        "parse error %.*s:%d: " msg,\
        path.absolute.size, path.absolute.bytes, (lexer).line_number)

#define PARSE_ERROR_F(path, lexer, msg, ...) \
    LOG(Log_error,\
        "parse error %.*s:%d: " msg,\
        path.absolute.size, path.absolute.bytes, lexer.line_number, __VA_ARGS__)


#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
                LOG("assert failed: " # condition); \
                DEBUG_BREAK(); \
        } \
    } while(0)

#endif /* LEARY_MACROS_H */

