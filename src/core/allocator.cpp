/**
 * file:    allocator.cpp
 * created: 2017-03-12
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "allocator.h"

// TODO(jesper): zalloc - allocate and zero memset
// TODO(jesper): ialloc - allocate and default initialise struct

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

u8 align_address_adjustment(void *address, u8 alignment, u8 header_size)
{
	uptr aligned = (uptr)align_address(address, alignment, header_size);
	return (u8)(aligned - (uptr)address);
}

/*******************************************************************************
 * LinearAllocator
 ******************************************************************************/
LinearAllocator allocator_create_linear(void *start, isize size)
{
	LinearAllocator a;
	a.start   = start;
	a.current = a.start;
	a.size    = size;
	return a;
}

void *alloc(LinearAllocator *a, isize size)
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

void reset(LinearAllocator *a)
{
	a->current = a->start;
}

/*******************************************************************************
 * StackAllocator
 ******************************************************************************/
StackAllocator allocator_create_stack(void *start, isize size)
{
	StackAllocator a;
	a.start = start;
	a.sp    = a.start;
	a.size  = size;
	return a;
}

void *alloc(StackAllocator *a, isize size)
{
	u8 header_size = sizeof(AllocationHeader);

	void *unaligned = a->sp;
	void *aligned   = align_address(unaligned, 16, header_size);

	a->sp = (void*)((uptr)aligned + size);
	DEBUG_ASSERT((uptr)a->sp < ((uptr)a->start + a->size));

	auto header = (AllocationHeader*)((uptr)aligned - header_size);
	header->size      = size;
	header->unaligned = unaligned;

	return aligned;
}

void dealloc(StackAllocator *a, void *ptr)
{
	auto  header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
	a->sp = header->unaligned;
}

void *realloc(StackAllocator *a, void *ptr, isize size)
{
	if (ptr == nullptr) {
		return alloc(a, size);
	}

	// NOTE(jesper): reallocing can be bad as we'll almost certainly leak the
	// memory, but for the general use case this should be fine
	auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
	if ((uptr)ptr + header->size == (uptr)a->sp) {
		isize extra = size - header->size;
		DEBUG_ASSERT(extra > 0); // NOTE(jesper): untested

		a->sp = (void*)((uptr)a->sp + extra);
		header->size = size;
		return ptr;
	} else {
		DEBUG_LOG("can't expand stack allocation, leaking memory");
		void *nptr = alloc(a, size);
		memcpy(nptr, ptr, header->size);
		return nptr;
	}
}

void reset(StackAllocator *a, void *ptr)
{
	a->sp = ptr;
}

/*******************************************************************************
 * FreeListAllocator
 ******************************************************************************/
FreeListAllocator allocator_create_freelist(void *start, isize size)
{
	FreeListAllocator a;
	a.start = start;
	a.size  = size;

	a.free = (FreeBlock*)start;
	a.free->size = size;
	a.free->next = nullptr;

	return a;
}

void *alloc(FreeListAllocator *a, isize size)
{
	u8 header_size = sizeof(AllocationHeader);
	isize required = size + header_size + 16;

	FreeBlock *prev = nullptr;
	FreeBlock *free = a->free;
	while(free != nullptr) {
		u8 adjustment = align_address_adjustment(free, 16, header_size);
		if (free->size > (size + adjustment)) {
			break;
		}

		// TODO(jesper): find best fitting free block
		prev = free;
		free = free->next;
	}

	DEBUG_ASSERT(((uptr)free + size) < ((uptr)a->start + a->size));
	DEBUG_ASSERT(free && free->size >= required);
	if (free == nullptr || free->size < required) {
		return nullptr;
	}

	void *unaligned = (void*)free;
	void *aligned   = align_address(unaligned, 16, header_size);

	isize free_size = free->size;
	isize remaining = free_size - size;

	// TODO(jesper): double check suitable value of minimum remaining
	if (remaining < (header_size + 48)) {
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

void dealloc(FreeListAllocator *a, void *ptr)
{
	auto  header     = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
	isize size       = header->size;

	uptr start = (uptr)header->unaligned;
	uptr end   = start + size;

	FreeBlock *nfree = (FreeBlock*)start;
	nfree->size      = size;

	bool expanded = false;
	bool insert   = false;
	FreeBlock *prev = nullptr;
	FreeBlock *free = a->free;
	FreeBlock *next = nullptr;

	while (free != nullptr) {
		if (end == (uptr)free) {
			nfree->size = nfree->size + free->size;
			nfree->next = free->next;

			if (prev != nullptr) {
				prev->next = nfree;
			}
			expanded = true;
			break;
		} else if (((uptr)free + free->size) == start) {
			free->size = free->size + nfree->size;
			expanded = true;
			break;
		} else if ((uptr)free > start) {
			prev = free;
			next = free->next;
			insert = true;
			break;
		}

		prev = free;
		free = free->next;
	}

	DEBUG_ASSERT(insert || expanded);
	if (!expanded) {
		prev->next  = nfree;
		nfree->next = next;
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

/*******************************************************************************
 * SystemAllocator
 ******************************************************************************/
void *alloc(SystemAllocator *, isize size)
{
	return malloc(size);
}

void dealloc(SystemAllocator *, void *ptr)
{
	free(ptr);
}

void *realloc(SystemAllocator *, void *ptr, isize size)
{
	return realloc(ptr, size);
}


/*******************************************************************************
 * Templated allocator helpers
 ******************************************************************************/
Allocator allocator_create(AllocatorType type)
{
	DEBUG_ASSERT(type == Allocator_System);

	Allocator a;
	a.type = type;

	return a;
}
Allocator allocator_create(AllocatorType type, void *data, isize size)
{
	Allocator a;
	a.type = type;
	switch (type) {
	case Allocator_Linear: {
		a.linear.start   = data;
		a.linear.current = data;
		a.linear.last    = nullptr;
		a.linear.size    = size;
	} break;
	case Allocator_Stack: {
		a.stack.start = data;
		a.stack.sp    = data;
		a.linear.size = size;
	} break;
	case Allocator_FreeList: {
		a.free_list.start      = data;
		a.free_list.size       = size;
		a.free_list.free       = (FreeBlock*)data;
		a.free_list.free->size = size;
		a.free_list.free->next = nullptr;
	} break;
	default:
		DEBUG_LOG(Log_error, "unknown allocator type: %d", type);
		DEBUG_ASSERT(false);
		break;
	}

	return a;
}

void *alloc(Allocator *a, isize size)
{
	switch (a->type) {
	case Allocator_Linear: {
		return alloc(&a->linear, size);
	} break;
	case Allocator_Stack: {
		return alloc(&a->stack, size);
	} break;
	case Allocator_FreeList: {
		return alloc(&a->free_list, size);
	} break;
	default:
		DEBUG_LOG(Log_error, "unknown allocator type: %d", a->type);
		DEBUG_ASSERT(false);
		break;
	}

	return nullptr;
}

void dealloc(Allocator *a, void *ptr)
{
	switch (a->type) {
	case Allocator_Linear: {
		dealloc(&a->linear, ptr);
	} break;
	case Allocator_Stack: {
		dealloc(&a->stack, ptr);
	} break;
	case Allocator_FreeList: {
		dealloc(&a->free_list, ptr);
	} break;
	default:
		DEBUG_LOG(Log_error, "unknown allocator type: %d", a->type);
		DEBUG_ASSERT(false);
		break;
	}
}

void *realloc(Allocator *a, void *ptr, isize size)
{
	switch (a->type) {
	case Allocator_Linear: {
		return realloc(&a->linear, ptr, size);
	} break;
	case Allocator_Stack: {
		return realloc(&a->stack, ptr, size);
	} break;
	case Allocator_FreeList: {
		return realloc(&a->free_list, ptr, size);
	} break;
	default:
		DEBUG_LOG(Log_error, "unknown allocator type: %d", a->type);
		DEBUG_ASSERT(false);
		break;
	}

	return nullptr;
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
T *alloc_array(A *a, isize count)
{
	return (T*)alloc(a, sizeof(T) * count);
}

template <typename T, typename A>
T *zalloc_array(A *a, isize count)
{
	T *ptr = (T*)alloc(a, sizeof(T) * count);
	memset(ptr, 0, sizeof(T) * count);
	return ptr;
}

template <typename T, typename A>
T *ialloc_array(A *a, isize count)
{
	T *ptr = (T*)alloc(a, sizeof(T) * count);
	T val = {};
	for (i32 i = 0; i < count; i++) {
		ptr[i] = val;
	}
	return ptr;
}

template<typename T, typename A>
T *realloc_array(A *a, T *ptr, isize capacity)
{
	isize size = capacity * sizeof(T);
	return (T*)realloc(a, ptr, size);
}
