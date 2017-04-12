/**
 * file:    allocator.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

struct SystemAllocator {};

struct LinearAllocator {
	void *current;
	void *last;
};

struct StackAllocator {
	void *sp;
};

struct FreeBlock {
	isize size;
	FreeBlock *next;
};

struct FreeListAllocator {
	FreeBlock *free;
};

enum AllocatorType {
	Allocator_Linear,
	Allocator_Stack,
	Allocator_FreeList,
	Allocator_System
};

struct Allocator {
	AllocatorType type;

	void *start;
	isize size;

	union {
		LinearAllocator   linear;
		StackAllocator    stack;
		FreeListAllocator free_list;
	};
};

#endif /* ALLOCATOR_H */

