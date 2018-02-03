/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#include "array.h"

template<typename T>
Array<T> create_array(Allocator *allocator)
{
    Array<T> a  = {};
    a.allocator = allocator;

    return a;
}

template<typename T>
Array<T> create_array(Allocator *allocator, i32 capacity)
{
    Array<T> a;
    a.allocator = allocator;
    a.data      = allocator->alloc_array<T>(capacity);
    a.count     = 0;
    a.capacity  = capacity;

    return a;
}

template<typename T>
void init_array(Array<T> *arr, Allocator *a)
{
    arr->allocator = a;
    arr->data      = nullptr;
    arr->count     = 0;
    arr->capacity  = 0;
}

template<typename T>
void init_array(Array<T> *arr, Allocator *a, i32 capacity)
{
    arr->allocator = a;
    arr->data      = a->alloc_array<T>(capacity);
    arr->count     = 0;
    arr->capacity  = capacity;
}

template<typename T>
void destroy_array(Array<T> *a)
{
    a->allocator->dealloc(a->data);
    a->data     = nullptr;
    a->capacity = 0;
    a->count    = 0;
}

template<typename T>
void array_resize(Array<T> *a, i32 count)
{
    ASSERT(a->count    == 0);
    ASSERT(a->capacity == 0);
    ASSERT(a->data == nullptr);

    a->count    = count;
    a->capacity = count;
    a->data     = a->allocator->template alloc_array<T>(count);
}

template<typename T>
i32 array_add(Array<T> *a, T e)
{
    ASSERT(a->allocator != nullptr);

    if (a->count >= a->capacity) {
        i32 capacity = a->capacity == 0 ? 1 : a->capacity * 2;
        a->data      = a->allocator->realloc_array(a->data, capacity);
        a->capacity  = capacity;
    }

    a->data[a->count] = e;
    return a->count++;
}

template<typename T>
i32 array_remove(Array<T> *a, i32 i)
{
    if ((a->count - 1) == i) {
        return --a->count;
    }

    a->data[i] = a->data[--a->count];
    return a->count;
}

template<typename T>
i32 array_remove_ordered(Array<T> *a, i32 i)
{
    if ((a->count - 1) == i) {
        return --a->count;
    }

    std::memmove(&a->data[i], &a->data[i+1], (a->count-i-1) * sizeof(T));
    return --a->count;
}



template<typename T>
StaticArray<T> create_static_array(void *data, i32 capacity)
{
    StaticArray<T> a;
    a.data      = (T*)data;
    a.capacity  = capacity;

    return a;
}

template<typename T>
StaticArray<T> create_static_array(void* ptr, i32 offset, i32 capacity)
{
    StaticArray<T> a;
    a.data      = (T*)((u8*)ptr + offset);
    a.capacity  = capacity;

    return a;
}

template<typename T>
void destroy_array(StaticArray<T> *a)
{
    a->data     = nullptr;
    a->capacity = 0;
    a->count    = 0;
}

template<typename T>
i32 array_add(StaticArray<T> *a, T e)
{
    ASSERT(a->count <= a->capacity);

    a->data[a->count] = e;
    return a->count++;
}

template<typename T>
i32 array_remove(StaticArray<T> *a, i32 i)
{
    if ((a->count - 1) == i) {
        return --a->count;
    }

    a->data[i] = a->data[--a->count];
    return a->count;
}


template<typename T>
void array_clear(Array<T> *a)
{
    for (i32 i = 0; i < a->count; i++) {
        a->data[i].~T();
    }
    a->count = 0;
}
