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


// NOTE: IMPORTANT: this will likely _never_ have a constructor or + operator or
// copy constructors or anything on those lines. This struct is _only_ intended
// to be a _slightly_ more powerful extension to c-strings with which I can have
// several Struct objects point to the same memory but with different lengths,
// to reduce memory allocation overhead
struct String {
    // TODO(jesper): for now these are both null-terminated and have a set
    // length in the struct. this is for clib compatability for now, but will be
    // going away when I've built my own strcat w/ friends.
    isize length;
    char  *bytes;

    char& operator[] (isize i)
    {
        assert(i < length);
        return bytes[i];
    }
};

struct Path {
    String absolute;
    String filename;  // NOTE(jesper): includes extension
    String extension; // NOTE(jesper): excluding .
};


Path create_path(const char *str);
i32 string_length(const char *str);

template<typename T>
i32 array_add(Array<T> *a, T e);

#endif /* LEARY_CORE_H */

