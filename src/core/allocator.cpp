/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

struct FrameAllocator {
	u8 *data;
	u8 *ptr;
	isize size;
};

FrameAllocator make_frame_allocator(isize size)
{
	FrameAllocator a;
	a.data = (u8*)malloc(size);
	a.ptr  = a.data;
	a.size = size;

	return a;
}

void reset(FrameAllocator *a)
{
	a->ptr = a->data;
}

template<typename T>
T* allocate(FrameAllocator *a)
{
	isize size = sizeof(T);

	T* r = (T*)a->ptr;
	a->ptr = a->ptr + size;
	DEBUG_ASSERT(a->ptr < (a->data + a->size));
	return r;
}

template<typename T>
T* allocate(FrameAllocator *a, i32 count)
{
	isize size = sizeof(T) * count;

	T* r = (T*)a->ptr;
	a->ptr = a->ptr + size;
	DEBUG_ASSERT(a->ptr < (a->data + a->size));
	return r;
}

