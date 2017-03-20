/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "array.h"

template<typename T, typename A>
Array<T, A> make_array(A *allocator)
{
	Array<T, A> a  = {};
	a.allocator = allocator;

	return a;
}


template<typename T, typename A>
Array<T, A> make_array(A *allocator, isize capacity)
{
	Array<T, A> a;
	a.allocator = allocator;
	a.data      = alloc_array<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T>
StaticArray<T> make_static_array(void *data, isize capacity)
{
	StaticArray<T> a;
	a.data      = (T*)data;
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T>
StaticArray<T> make_static_array(void* ptr,
                                 isize offset,
                                 isize capacity)
{
	StaticArray<T> a;
	a.data      = (T*)((u8*)ptr + offset);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T, typename A>
void free_array(Array<T, A> *a)
{
	dealloc(a->allocator, a->data);
	a->capacity = 0;
	a->count    = 0;
}

template<typename T>
void free_array(StaticArray<T> *a)
{
	a->capacity = 0;
	a->count    = 0;
}

template<typename T, typename A>
isize array_add(Array<T, A> *a, T &e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;

		T* data = alloc_array<T>(a->allocator, capacity);

		if (a->data != nullptr) {
			memcpy(data, a->data, sizeof(T) * a->capacity);
			dealloc(a->allocator, a->data);
		}

		a->data     = data;
		a->capacity = capacity;
	}

	a->data[a->count] = e;
	return a->count++;
}

template<typename T>
isize array_add(StaticArray<T> *a, T &e)
{
	DEBUG_ASSERT(a->count <= a->capacity);

	a->data[a->count] = e;
	return a->count++;
}
