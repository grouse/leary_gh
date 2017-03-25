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

LinearAllocator make_linear_allocator(void *start, isize size)
{
	LinearAllocator a;
	a.start   = start;
	a.current = a.start;
	a.last    = nullptr;
	a.size    = size;
	return a;
}

StackAllocator make_stack_allocator(void *start, isize size)
{
	StackAllocator a;
	a.start   = start;
	a.current = a.start;
	a.size    = size;
	return a;
}

FreeListAllocator make_free_list_allocator(void *start, isize size)
{
	FreeListAllocator a;
	a.free = (FreeBlock*)start;
	a.free->size = size;
	a.free->next = nullptr;

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

	auto header = (AllocationHeader*)((uptr)aligned - header_size);
	header->size      = size;
	header->unaligned = unaligned;

	return aligned;
}

void *alloc(FreeListAllocator *a, isize size)
{
	u8 header_size = sizeof(AllocationHeader);
	isize required = size + header_size + 16;

	FreeBlock *prev = nullptr;
	FreeBlock *free = a->free;
	while(free != nullptr && free->size < required) {
		// TODO(jesper): find best fitting free block
		prev = free;
		free = free->next;
	}

	DEBUG_ASSERT(free && free->size >= required);
	if (free == nullptr || free->size < required) {
		return nullptr;
	}

	void *unaligned = (void*)free;
	void *aligned   = align_address(unaligned, 16, header_size);

	isize free_size = free->size;
	isize remaining = free_size - size;

	// TODO(jesper): double check suitable value of minimum remaining
	if (remaining < (header_size + 16 + 8)) {
		size = free_size;

		if (prev != nullptr) {
			prev->next = free->next;
		} else {
			a->free = a->free->next;
		}
	} else {
		FreeBlock *nfree = (FreeBlock*)((uptr)aligned + size);
		nfree->size = remaining;
		nfree->next = free->next;

		if (prev != nullptr) {
			prev->next = nfree;
		} else {
			a->free = nfree;
		}
	}

	auto header = (AllocationHeader*)((uptr)aligned - header_size);
	header->size      = size;
	header->unaligned = unaligned;

	return aligned;
}

void dealloc(LinearAllocator *a, void *ptr)
{
	if (ptr == nullptr) {
		return;
	}

	auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
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

void dealloc(FreeListAllocator *a, void *ptr)
{
	if (ptr == nullptr) {
		return;
	}

	auto  header     = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
	isize size       = header->size;
	void  *unaligned = header->unaligned;

	FreeBlock *nfree = (FreeBlock*)unaligned;
	nfree->size = size;
	nfree->next = a->free;
	a->free     = nfree;

	// TODO(jesper): automatic defragment by finding neighbour free blocks and
	// merging them
}

void *realloc(LinearAllocator *a, void *ptr, isize size)
{
	if (ptr == nullptr) {
		return alloc(a, size);
	}

	auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
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

void *realloc(FreeListAllocator *a, void *ptr, isize size)
{
	if (ptr == nullptr) {
		return alloc(a, size);
	}

	auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));

	// TODO(jesper): try to find neighbour FreeBlock and expand
	void *nptr = alloc(a, size);
	memcpy(nptr, ptr, header->size);
	dealloc(a, ptr);

	return nptr;
}

void reset(LinearAllocator *a)
{
	a->current = a->start;
	a->last    = nullptr;
}


template <typename T, typename A>
T *alloc(A *a)
{
	return (T*)alloc(a, sizeof(T));
}

template <typename T, typename A>
T *ialloc(A *a)
{
	T* mem = (T*)alloc(a, sizeof(T));
	T e = {};
	memcpy(mem, &e, sizeof(T));
	return mem;
}

template <typename T, typename A>
T *alloc_array(A *a, i32 count)
{
	return (T*)alloc(a, sizeof(T) * count);
}

template<typename T, typename A>
T *realloc_array(A *a, T *ptr, isize capacity)
{
	isize size = capacity * sizeof(T);
	return (T*)realloc(a, ptr, size);
}


