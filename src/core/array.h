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
	T* data        = nullptr;
	isize count    = 0;
	isize capacity = 0;

	Allocator *allocator = nullptr;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

template<typename T>
struct StaticArray {
	T* data        = nullptr;
	isize count    = 0;
	isize capacity = 0;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

#endif /* ARRAY_H */

