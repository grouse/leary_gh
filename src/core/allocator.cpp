/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

struct FrameAllocator {
	void *data;
	void *ptr;
	void *end;
	isize size;
};

struct PersistentAllocator {
	void *data;
	void *ptr;
	void *end;
	isize size;
};

struct DefaultAllocator {};

FrameAllocator make_frame_allocator(void *ptr, isize size)
{
	FrameAllocator a;
	a.data = ptr;
	a.end  = (u8*)a.data + size;
	a.ptr  = a.data;
	a.size = size;

	return a;
}

FrameAllocator make_frame_allocator(isize size)
{
	FrameAllocator a;
	a.data = (u8*)malloc(size);
	a.end  = (u8*)a.data + size;
	a.ptr  = a.data;
	a.size = size;

	return a;
}

PersistentAllocator make_persistent_allocator(void *ptr, isize size)
{
	PersistentAllocator a;
	a.data = ptr;
	a.end  = (u8*)a.data + size;
	a.ptr  = a.data;
	a.size = size;

	return a;
}

PersistentAllocator make_persistent_allocator(isize size)
{
	PersistentAllocator a;
	a.data = (u8*)malloc(size);
	a.end  = (u8*)a.data + size;
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
	a->ptr = (u8*)a->ptr + size;
	DEBUG_ASSERT(a->ptr < a->end);
	return r;
}

template<typename T>
T* allocate(FrameAllocator *a, i32 count)
{
	isize size = sizeof(T) * count;

	T* r = (T*)a->ptr;
	a->ptr = (u8*)a->ptr + size;
	DEBUG_ASSERT(a->ptr < a->end);
	return r;
}

template<typename T>
T* allocate(PersistentAllocator *a)
{
	isize size = sizeof(T);

	T* r = (T*)a->ptr;
	a->ptr = (u8*)a->ptr + size;
	DEBUG_ASSERT(a->ptr < a->end);
	return r;
}

template<typename T>
T* allocate(PersistentAllocator *a, i32 count)
{
	isize size = sizeof(T) * count;

	T* r = (T*)a->ptr;
	a->ptr = (u8*)a->ptr + size;
	DEBUG_ASSERT(a->ptr < a->end);
	return r;
}


template<typename T>
void dealloc(FrameAllocator *, T *)
{
	DEBUG_LOG("dealloc called on frame allocator, memory will be leaked");
}

template<typename T>
void dealloc(PersistentAllocator *, T *)
{
	DEBUG_LOG("dealloc called on persistent allocator, memory will be leaked");
}

