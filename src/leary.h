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

#include "platform/platform.h"
#include "platform/platform_input.h"

#include "core/types.h"
#include "core/allocator.h"

#define ARRAY_SIZE(a) sizeof((a)) / sizeof((a)[0])

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

struct GameMemory {
	void *game;
	LinearAllocator   frame;
	LinearAllocator   persistent;
	StackAllocator    stack;
	FreeListAllocator free_list;
};

#endif /* LEARY_H */

