/**
 * file:    core.h
 * created: 2017-11-08
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#ifndef LEARY_CORE_H
#define LEARY_CORE_H

#include "types.h"
#include "allocator.h"
#include "string.h"
#include "array.h"

#define TABLE_SIZE 128

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

#endif /* LEARY_CORE_H */

