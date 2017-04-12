/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "array.h"

template<typename T>
ARRAY(T) array_create(Allocator *allocator)
{
	ARRAY(T) a  = {};
	a.allocator = allocator;

	return a;
}

template<typename T>
ARRAY(T) array_create(Allocator *allocator, isize capacity)
{
	ARRAY(T) a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T>
void array_destroy(ARRAY(T) *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

template<typename T>
isize array_add(ARRAY(T) *a, T e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
		a->data        = realloc_array(a->allocator, a->data, capacity);
		a->capacity    = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

template<typename T>
isize array_remove(ARRAY(T) *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}


template<typename T>
SARRAY(T) array_create_static(void *data, isize capacity)
{
	SARRAY(T) a;
	a.data      = (T*)data;
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T>
SARRAY(T) array_create_static(void* ptr, isize offset, isize capacity)
{
	SARRAY(T) a;
	a.data      = (T*)((u8*)ptr + offset);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T>
void array_destroy(SARRAY(T) *a)
{
	a->capacity = 0;
	a->count    = 0;
}

template<typename T>
isize array_add(SARRAY(T) *a, T e)
{
	DEBUG_ASSERT(a->count <= a->capacity);

	a->data[a->count] = e;
	return a->count++;
}

template<typename T>
isize array_remove(SARRAY(T) *a, isize i)
{
	if ((a->count - 1) == i) {
		return --a->count;
	}

	a->data[a->count-1] = a->data[i];
	return --a->count;
}

