/**
 * file:    leary_macros.h
 * created: 2017-09-14
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#define MCOMBINE2(a, b) a ## b
#define MCOMBINE(a, b) MCOMBINE2(a, b)

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

#define PARSE_ERROR(path, lexer, msg) \
    LOG_ERROR("parse error %.*s:%d: "\
              msg,\
              path.absolute.size,\
              path.absolute.bytes,\
              (lexer).line_number)

#define PARSE_ERROR_F(path, lexer, msg, ...) \
    LOG_ERROR("parse error %.*s:%d: "\
              msg,\
              path.absolute.size,\
              path.absolute.bytes,\
              (lexer).line_number,\
              __VA_ARGS__)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
                LOG_ASSERT("assert failed: " # condition); \
                DEBUG_BREAK(); \
        } \
    } while(0)
