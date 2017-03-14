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
	isize size;
};

struct DefaultAllocator {};

LinearAllocator make_linear_allocator(void *start, isize size)
{
	LinearAllocator a;
	a.start   = start;
	a.current = a.start;
	a.size    = size;
	return a;
}

template <typename T>
T* alloc(LinearAllocator *a)
{
	// TODO(jesper): alignment
	isize size = sizeof(T);

	T *ptr = (T*)a->current;
	a->current = (u8*)a->current + size;
	DEBUG_ASSERT(a->current < (u8*)a->start + a->size);

	return ptr;
}

template <typename T>
T* alloc(LinearAllocator *a, i32 count)
{
	// TODO(jesper): alignment
	isize size = sizeof(T) * count;

	T *ptr = (T*)a->current;
	a->current = (u8*)a->current + size;
	DEBUG_ASSERT(a->current < (u8*)a->start + a->size);

	return ptr;
}

void dealloc(LinearAllocator *, void *)
{
	// TODO(jesper): deallocate if the ptr is the last one allocated
	DEBUG_LOG("calling dealloc on linear allocator, leaking memory");
}

void reset(LinearAllocator *a)
{
	a->current = a->start;
}

