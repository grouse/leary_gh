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
	void *start;
	void *current;
	void *last;
	isize size;
};

struct StackAllocator {
	void *start;
	void *sp;
	isize size;
};

struct FreeBlock {
	isize size;
	FreeBlock *next;
};

struct FreeListAllocator {
	void *start;
	isize size;

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

	union {
		LinearAllocator   linear;
		StackAllocator    stack;
		FreeListAllocator free_list;
	};
};

#endif /* ALLOCATOR_H */

