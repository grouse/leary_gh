/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

// TODO(jesper): realloc
// TODO(jesper): zalloc - allocate and zero memset
// TODO(jesper): ialloc - allocate and default initialise struct
// TODO(jesper): allocation header infront of allocated ptr for meta data

struct LinearAllocator {
	void *start;
	void *current;
	void *last;
	isize size;
};

struct DefaultAllocator {};

LinearAllocator make_linear_allocator(void *start, isize size)
{
	LinearAllocator a;
	a.start   = start;
	a.current = a.start;
	a.last    = nullptr;
	a.size    = size;
	return a;
}

template <typename T>
T* alloc(LinearAllocator *a)
{
	// TODO(jesper): alignment
	isize size = sizeof(T);

	T *ptr     = (T*)a->current;
	a->current = (u8*)a->current + size;
	a->last    = ptr;
	DEBUG_ASSERT(a->current < (u8*)a->start + a->size);

	return ptr;
}

template <typename T>
T* alloc(LinearAllocator *a, i32 count)
{
	// TODO(jesper): alignment
	isize size = sizeof(T) * count;

	T *ptr     = (T*)a->current;
	a->current = (u8*)a->current + size;
	a->last    = ptr;
	DEBUG_ASSERT(a->current < (u8*)a->start + a->size);

	return ptr;
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

void reset(LinearAllocator *a)
{
	a->current = a->start;
	a->last    = nullptr;
}

