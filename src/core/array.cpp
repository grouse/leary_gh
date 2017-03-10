/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

template<typename T>
struct Array {
	T* data;
	isize count;
	isize capacity;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

template<typename T>
Array<T> make_array()
{
	Array<T> a = {};
	return a;
}

template<typename T>
void free_array(Array<T> *a)
{
	free(a->data);
	a->capacity = 0;
	a->count    = 0;
}


template<typename T>
isize array_add(Array<T> *a, T &e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;

		T* data = (T*)malloc(sizeof(T) * capacity);
		memcpy(data, a->data, sizeof(T) * a->capacity);

		free(a->data);
		a->data     = data;
		a->capacity = capacity;
	}

	a->data[a->count++] = e;
	return a->count;
}
