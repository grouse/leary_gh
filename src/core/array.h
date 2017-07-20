/**
 * file:    array.h
 * created: 2017-03-20
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef ARRAY_H
#define ARRAY_H

template<typename T>
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

template<typename T>
struct StaticArray {
	T* data;
	isize count;
	isize capacity;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

#endif /* ARRAY_H */

