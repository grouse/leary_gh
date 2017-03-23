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

struct AllocationHeader {
	isize size;
	void *unaligned;
};

void *align_address(void *address, uptr alignment)
{
	return (void*)(((uptr)address + alignment) & ~(alignment - 1));
}

void *align_address(void *address, u8 alignment, u8 header_size)
{
	u8 adjustment = alignment - ((uptr)address & ((uptr)alignment - 1));
	if (adjustment == alignment) {
		adjustment = 0;
	}

	u8 required = header_size;
	if (adjustment < required) {
		required -= adjustment;

		adjustment += alignment * (required / alignment);
		if (required % alignment > 0) {
			adjustment += alignment;
		}
	}

	return (void*)((uptr)address + adjustment);
}

void *alloc(LinearAllocator *a, isize size)
{
	u8 header_size = sizeof(AllocationHeader);

	void *unaligned = a->current;
	void *aligned   = align_address(unaligned, 16, header_size);

	a->current = (void*)((uptr)aligned + size);
	a->last    = aligned;
	DEBUG_ASSERT((uptr)a->current < ((uptr)a->start + a->size));

	AllocationHeader *header = (AllocationHeader*)((uptr)aligned - header_size);
	header->size      = size;
	header->unaligned = unaligned;

	return aligned;
}

void *alloc(StackAllocator *a, isize size)
{
	u8 header_size = sizeof(AllocationHeader);

	void *unaligned = a->current;
	void *aligned   = align_address(unaligned, 16, header_size);

	a->current = (void*)((uptr)aligned + size);
	DEBUG_ASSERT((uptr)a->current < ((uptr)a->start + a->size));

	AllocationHeader *header = (AllocationHeader*)((uptr)aligned - header_size);
	header->size      = size;
	header->unaligned = unaligned;

	return aligned;
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

void *realloc(LinearAllocator *a, void *ptr, isize size)
{
	if (ptr == nullptr) {
		return alloc(a, size);
	}

	AllocationHeader *header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
	if ((uptr)ptr + header->size == (uptr)a->current) {
		isize extra = size - header->size;
		DEBUG_ASSERT(extra > 0); // NOTE(jesper): untested

		a->current   = (void*)((uptr)a->current + extra);
		header->size = size;
		return ptr;
	} else {
		DEBUG_LOG("can't expand linear allocation, leaking memory");
		void *nptr = alloc(a, size);
		memcpy(nptr, ptr, header->size);
		return nptr;
	}
}

template<typename T>
T *realloc_array(LinearAllocator *a, T *ptr, isize capacity)
{
	isize size = capacity * sizeof(T);
	return (T*)realloc(a, ptr, size);
}

void dealloc(LinearAllocator *a, void *ptr)
{
	if (ptr == nullptr) {
		return;
	}

	AllocationHeader *header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
	if ((uptr)ptr + header->size == (uptr)a->current) {
		a->current = header->unaligned;
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

