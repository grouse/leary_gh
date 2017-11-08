/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "core.h"

template<typename T>
Array<T> array_create(Allocator *allocator)
{
    Array<T> a  = {};
    a.allocator = allocator;

    return a;
}

template<typename T>
Array<T> array_create(Allocator *allocator, isize capacity)
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
void array_destroy(Array<T> *a)
{
    a->allocator->dealloc(a->data);
    a->capacity = 0;
    a->count    = 0;
}

template<typename T>
void array_resize(Array<T> *a, isize count)
{
    assert(a->count    == 0);
    assert(a->capacity == 0);
    assert(a->data == nullptr);

    a->count    = count;
    a->capacity = count;
    a->data     = a->allocator->template alloc_array<T>(count);
}

template<typename T>
isize array_add(Array<T> *a, T e)
{
    assert(a->allocator != nullptr);

    if (a->count >= a->capacity) {
        isize capacity = a->capacity == 0 ? 1 : a->capacity * 2;
        a->data        = a->allocator->realloc_array(a->data, capacity);
        a->capacity    = capacity;
    }

    a->data[a->count] = e;
    return a->count++;
}

template<typename T>
isize array_remove(Array<T> *a, isize i)
{
    if ((a->count - 1) == i) {
        return --a->count;
    }

    a->data[i] = a->data[--a->count];
    return a->count;
}

template<typename T>
isize array_remove_ordered(Array<T> *a, isize i)
{
    if ((a->count - 1) == i) {
        return --a->count;
    }

    for (; i < a->count-1; i++) {
        a->data[i] = a->data[i+1];
    }

    return --a->count;
}



template<typename T>
StaticArray<T> array_create_static(void *data, isize capacity)
{
    StaticArray<T> a;
    a.data      = (T*)data;
    a.capacity  = capacity;

    return a;
}

template<typename T>
StaticArray<T> array_create_static(void* ptr, isize offset, isize capacity)
{
    StaticArray<T> a;
    a.data      = (T*)((u8*)ptr + offset);
    a.capacity  = capacity;

    return a;
}

template<typename T>
void array_destroy(StaticArray<T> *a)
{
    a->capacity = 0;
    a->count    = 0;
}

template<typename T>
isize array_add(StaticArray<T> *a, T e)
{
    assert(a->count <= a->capacity);

    a->data[a->count] = e;
    return a->count++;
}

template<typename T>
isize array_remove(StaticArray<T> *a, isize i)
{
    if ((a->count - 1) == i) {
        return --a->count;
    }

    a->data[i] = a->data[--a->count];
    return a->count;
}

