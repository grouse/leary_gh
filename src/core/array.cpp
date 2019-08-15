/**
 * file:    array.cpp
 * created: 2017-03-10
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

template<typename T>
Array<T> create_array(Allocator *allocator)
{
    Array<T> a  = {};
    a.allocator = allocator;

    return a;
}

template<typename T>
Array<T> create_array(Allocator *a, i32 capacity)
{
    Array<T> arr;
    arr.allocator = a;
    arr.data      = (T*)alloc(a, sizeof(T) * capacity);
    arr.count     = 0;
    arr.capacity  = capacity;

    return arr;
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
    arr->data      = (T*)alloc(a, sizeof(T) * capacity);
    arr->count     = 0;
    arr->capacity  = capacity;
}

template<typename T>
void reset_array(Array<T> *arr)
{
    ASSERT(arr->allocator != nullptr);
    dealloc(arr->allocator, arr->data);

    arr->data     = nullptr;
    arr->count    = 0;
    arr->capacity = 0;
}

template<typename T>
void reset_array_count(Array<T> *arr)
{
    arr->count    = 0;
}

template<typename T>
void destroy_array(Array<T> *a)
{
    dealloc(a->allocator, a->data);
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
        a->data      = realloc_array(a->allocator, a->data, capacity);
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
void array_clear(Array<T> *a)
{
    for (i32 i = 0; i < a->count; i++) {
        a->data[i].~T();
    }
    a->count = 0;
}

template<typename T, typename F>
void array_insertion_sort(Array<T> *a, F cmp)
{
    for (i32 i = 0; i < a->count; i++) {
        for (i32 j = i;
             j > 0 && cmp(&a->data[j-1], &a->data[j]);
             j--)
        {
            std::swap(a->data[j], a->data[j-1]);
        }
    }
}
