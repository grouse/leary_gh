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
 * StackAllocator
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
	DEBUG_ASSERT(type != Allocator_System);

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
		u8 header_size = sizeof(AllocationHeader);

		void *unaligned = a->linear.current;
		void *aligned   = align_address(unaligned, 16, header_size);

		a->linear.current = (void*)((uptr)aligned + size);
		DEBUG_ASSERT((uptr)a->linear.current < ((uptr)a->linear.start + a->linear.size));

		AllocationHeader *header = (AllocationHeader*)((uptr)aligned - header_size);
		header->size      = size;
		header->unaligned = unaligned;

		return aligned;
	} break;
	case Allocator_Stack: {
		u8 header_size = sizeof(AllocationHeader);

		void *unaligned = a->stack.sp;
		void *aligned   = align_address(unaligned, 16, header_size);

		a->stack.sp = (void*)((uptr)aligned + size);
		DEBUG_ASSERT((uptr)a->stack.sp < ((uptr)a->stack.start + a->stack.size));

		auto header = (AllocationHeader*)((uptr)aligned - header_size);
		header->size      = size;
		header->unaligned = unaligned;

		return aligned;
	} break;
	case Allocator_FreeList: {
		u8 header_size = sizeof(AllocationHeader);
		isize required = size + header_size + 16;

		FreeBlock *prev = nullptr;
		FreeBlock *free = a->free_list.free;
		while(free != nullptr) {
			u8 adjustment = align_address_adjustment(free, 16, header_size);
			if (free->size > (size + adjustment)) {
				break;
			}

			// TODO(jesper): find best fitting free block
			prev = free;
			free = free->next;
		}

		DEBUG_ASSERT(((uptr)free + size) < ((uptr)a->free_list.start + a->free_list.size));
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
				a->free_list.free = a->free_list.free->next;
			}
		} else {
			FreeBlock *nfree = (FreeBlock*)((uptr)aligned + size);
			nfree->size = remaining;
			nfree->next = free->next;

			if (prev != nullptr) {
				prev->next = nfree;
			} else {
				a->free_list.free = nfree;
			}
		}

		auto header = (AllocationHeader*)((uptr)aligned - header_size);
		header->size      = size;
		header->unaligned = unaligned;

		return aligned;
	} break;
	case Allocator_System: {
		return malloc(size);
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
		if (ptr == nullptr) {
			return;
		}

		auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
		if ((uptr)ptr + header->size == (uptr)a->linear.current) {
			a->linear.current = header->unaligned;
		} else {
			DEBUG_LOG("calling dealloc on linear allocator, leaking memory");
		}
	} break;
	case Allocator_Stack: {
		auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
		a->stack.sp = header->unaligned;
	} break;
	case Allocator_FreeList: {
		auto  header     = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
		isize size       = header->size;

		uptr start = (uptr)header->unaligned;
		uptr end   = start + size;

		FreeBlock *nfree = (FreeBlock*)start;
		nfree->size      = size;

		bool expanded = false;
		bool insert   = false;
		FreeBlock *prev = nullptr;
		FreeBlock *free = a->free_list.free;
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
	} break;
	case Allocator_System: {
		free(ptr);
	} break;
	default:
		DEBUG_LOG(Log_error, "unknown allocator type: %d", a->type);
		DEBUG_ASSERT(false);
		break;
	}
}

void *realloc(Allocator *a, void *ptr, isize size)
{
	if (ptr == nullptr) {
		return alloc(a, size);
	}

	switch (a->type) {
	case Allocator_Linear: {
		auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
		if ((uptr)ptr + header->size == (uptr)a->linear.current) {
			isize extra = size - header->size;
			DEBUG_ASSERT(extra > 0); // NOTE(jesper): untested

			a->linear.current   = (void*)((uptr)a->linear.current + extra);
			header->size = size;
			return ptr;
		} else {
			DEBUG_LOG("can't expand linear allocation, leaking memory");
			void *nptr = alloc(a, size);
			memcpy(nptr, ptr, header->size);
			return nptr;
		}
	} break;
	case Allocator_Stack: {
		// NOTE(jesper): reallocing can be bad as we'll almost certainly leak the
		// memory, but for the general use case this should be fine
		auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));
		if ((uptr)ptr + header->size == (uptr)a->stack.sp) {
			isize extra = size - header->size;
			DEBUG_ASSERT(extra > 0); // NOTE(jesper): untested

			a->stack.sp = (void*)((uptr)a->stack.sp + extra);
			header->size = size;
			return ptr;
		} else {
			DEBUG_LOG("can't expand stack allocation, leaking memory");
			void *nptr = alloc(a, size);
			memcpy(nptr, ptr, header->size);
			return nptr;
		}
	} break;
	case Allocator_FreeList: {
		auto header = (AllocationHeader*)((uptr)ptr - sizeof(AllocationHeader));

		// TODO(jesper): try to find neighbour FreeBlock and expand
		void *nptr = alloc(a, size);
		memcpy(nptr, ptr, header->size);
		dealloc(a, ptr);

		return nptr;
	} break;
	case Allocator_System: {
		return realloc(ptr, size);
	} break;
	default:
		DEBUG_LOG(Log_error, "unknown allocator type: %d", a->type);
		DEBUG_ASSERT(false);
		break;
	}

	return nullptr;
}

void alloc_reset(Allocator *a)
{
	DEBUG_ASSERT(a->type == Allocator_Linear);
	a->linear.current = a->linear.start;
}

void alloc_reset(Allocator *a, void *ptr)
{
	DEBUG_ASSERT(a->type == Allocator_Stack);
	a->stack.sp = ptr;
}


/*******************************************************************************
 * Templated allocator helpers
 ******************************************************************************/
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
