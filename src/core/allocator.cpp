/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "allocator.h"

// TODO(jesper): realloc
// TODO(jesper): zalloc - allocate and zero memset
// TODO(jesper): ialloc - allocate and default initialise struct
// TODO(jesper): allocation header infront of allocated ptr for meta data

extern "C"
MAKE_LINEAR_ALLOCATOR_FUNC(make_linear_allocator)
{
	LinearAllocator a;
	a.start   = start;
	a.current = a.start;
	a.last    = nullptr;
	a.size    = size;
	return a;
}

extern "C"
MAKE_STACK_ALLOCATOR_FUNC(make_stack_allocator)
{
	StackAllocator a;
	a.start   = start;
	a.current = a.start;
	a.size    = size;
	return a;
}

void *align_address(void *address, uptr alignment)
{
	return (void*)(((uptr)address + alignment) & ~(alignment - 1));
}

void *alloc(LinearAllocator *a, isize size)
{
	void *ptr  = align_address(a->current, 16);
	a->current = (void*)((uptr)ptr + size);
	a->last    = ptr;
	DEBUG_ASSERT((uptr)a->current < ((uptr)a->start + a->size));

	return ptr;
}

void *alloc(StackAllocator *a, isize size)
{
	void *ptr  = align_address(a->current, 16);
	a->current = (void*)((uptr)ptr + size);
	DEBUG_ASSERT((uptr)a->current < ((uptr)a->start + a->size));
	return ptr;
}

template <typename T>
T* alloc(LinearAllocator *a)
{
	return (T*)alloc(a, sizeof(T));
}

template <typename T>
T* ialloc(LinearAllocator *a)
{
	T* mem = (T*)alloc(a, sizeof(T));
	T e = {};
	memcpy(mem, &e, sizeof(T));
	return mem;
}

template <typename T>
T* alloc_array(LinearAllocator *a, i32 count)
{
	return (T*)alloc(a, sizeof(T) * count);
}

template <typename T>
T* alloc(StackAllocator *a)
{
	return (T*)alloc(a, sizeof(T));
}

template <typename T>
T* alloc_array(StackAllocator *a, i32 count)
{
	return (T*)alloc(a, sizeof(T) * count);
}

void dealloc(LinearAllocator *a, void *ptr)
{
	if (a->last != nullptr && a->last == ptr) {
		a->current = ptr;
		a->last    = nullptr;
	} else {
		DEBUG_LOG("calling dealloc on linear allocator, leaking memory");
	}
}

void dealloc(StackAllocator *a, void *ptr)
{
	a->current = ptr;
}

void reset(LinearAllocator *a)
{
	a->current = a->start;
	a->last    = nullptr;
}

