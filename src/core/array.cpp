/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

template<typename T, typename A>
struct Array {
	T* data;
	isize count;
	isize capacity;

	A* allocator;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

template<typename T, typename A>
struct StaticArray {
	T* data;
	isize count;
	isize capacity;

	A* allocator;

	T& operator[] (isize i)
	{
		return data[i];
	}
};

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
	a.data      = allocate<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T, typename A>
StaticArray<T, A> make_static_array(A *allocator, isize capacity)
{
	StaticArray<T, A> a;
	a.allocator = allocator;
	a.data      = allocate<T>(allocator, capacity);
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T, typename A>
StaticArray<T, A> make_static_array(void* ptr, isize capacity)
{
	StaticArray<T, A> a;
	a.allocator = nullptr;
	a.data      = ptr;
	a.count     = 0;
	a.capacity  = capacity;

	return a;
}

template<typename T>
StaticArray<T, DefaultAllocator> make_static_array(void* ptr,
                                                   isize offset,
                                                   isize capacity)
{
	StaticArray<T, DefaultAllocator> a;
	a.allocator = nullptr;
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

template<typename T, typename A>
void free_array(StaticArray<T, A> *a)
{
	if (a->allocator) {
		dealloc(a->allocator, a->data);
	}

	a->capacity = 0;
	a->count    = 0;
}

template<typename T, typename A>
isize array_add(Array<T, A> *a, T &e)
{
	if (a->count >= a->capacity) {
		isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;

		T* data = allocate<T>(a->allocator, capacity);
		memcpy(data, a->data, sizeof(T) * a->capacity);

		dealloc(a->allocator, a->data);
		a->data     = data;
		a->capacity = capacity;
	}

	a->data[a->count++] = e;
	return a->count;
}

template<typename T, typename A>
isize array_add(StaticArray<T, A> *a, T &e)
{
	DEBUG_ASSERT(a->count <= a->capacity);

	a->data[a->count++] = e;
	return a->count;
}
