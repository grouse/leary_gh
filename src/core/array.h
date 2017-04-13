/**
 * file:    array.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef ARRAY_H
#define ARRAY_H

#ifndef ARRAY_USE_TEMPLATES
#define ARRAY_USE_TEMPLATES 1
#endif

#if ARRAY_USE_TEMPLATES

#define ARRAY(type) Array<type>
#define ARRAY_CREATE(type, ...) array_create<type>(__VA_ARGS__)

#define SARRAY(type) StaticArray<type>
#define SARRAY_CREATE(type, ...) array_create_static<type>(__VA_ARGS__)

#define ARRAY_TEMPLATE template<typename T>
#define SARRAY_TEMPLATE template<typename T>

ARRAY_TEMPLATE
struct Array {
	T* data;
	isize count;
	isize capacity;

	Allocator *allocator;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

SARRAY_TEMPLATE
struct StaticArray {
	T* data;
	isize count;
	isize capacity;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

#else

#define ARRAY(type) Array_##type
#define ARRAY_CREATE(type, ...) array_create_##type(__VA_ARGS__)

#define SARRAY(type) StaticArray_##type
#define SARRAY_CREATE(type, ...) array_create_static_##type(__VA_ARGS__)

#include "generated/array.h"

#endif /* ARRAY_USE_TEMPLATES */

#endif /* ARRAY_H */

