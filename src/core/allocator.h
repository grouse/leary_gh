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
	void *current;
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

#endif /* ALLOCATOR_H */

