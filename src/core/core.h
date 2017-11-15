/**
 * file:    core.h
 * created: 2017-11-08
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_CORE_H
#define LEARY_CORE_H

#include <assert.h>

#include "types.h"
#include "allocator.h"
#include "string.h"


#define TABLE_SIZE 128

template<typename T>
struct Array {
    T* data      = nullptr;
    i32 count    = 0;
    i32 capacity = 0;

    Allocator *allocator = nullptr;

    T& operator[] (i32 i)
    {
        assert(i < count);
        return data[i];
    }

    T* begin()
    {
        return &data[0];
    }

    T* end()
    {
        return &data[count];
    }
};

template<typename T>
struct StaticArray {
    T* data      = nullptr;
    i32 count    = 0;
    i32 capacity = 0;

    T& operator[] (i32 i)
    {
        assert(i < count);
        return data[i];
    }

    T* begin()
    {
        return &data[0];
    }

    T* end()
    {
        return &data[count];
    }
};

template <typename K, typename V>
struct Pair {
    K key;
    V value;
};

template <typename K, typename V>
struct HashTable {

    // TODO(jesper): replace this with custom array implementation, I don't want
    // to use the generalised array implementation here.
    Array<Pair<K, V>> table[TABLE_SIZE];
};


template<typename T>
i32 array_add(Array<T> *a, T e);

#endif /* LEARY_CORE_H */

