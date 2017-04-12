/**
 * file:    leary.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_H
#define LEARY_H

#ifndef INTROSPECT
#define INTROSPECT
#endif

#include <math.h>
#include <utility>

#include "platform/platform.h"
#include "platform/platform_input.h"

#include "core/types.h"
#include "core/allocator.h"

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
};

#define defer__(line) defer_ ## line
#define defer_(line) defer__( line )

struct DeferDummy { };
template<typename F>
Defer<F> operator+ (DeferDummy, F&& f)
{
	return defer_create<F>(std::forward<F>(f));
}

#define defer auto defer_( __LINE__ ) = DeferDummy( ) + [&]( )

struct GameMemory {
	void *game;
	Allocator frame;
	Allocator persistent;
	Allocator stack;
	Allocator free_list;
};

#endif /* LEARY_H */

