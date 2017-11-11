/**
 * file:    hash_table.cpp
 * created: 2017-08-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "core.h"

template <typename K, typename V>
void init_table(HashTable<K, V> *table, Allocator *a)
{
    *table = {};
    for (i32 i = 0; i < TABLE_SIZE; i++) {
        table->table[i].allocator = a;
    }
}

template <typename K, typename V>
V* table_add(HashTable<K, V> *table, K key, V value)
{
    u64 hash  = hash32((void*)&key, sizeof(K));
    u64 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (table->table[index][i].key == key) {
            // TODO(jesper): to_string key
            LOG("key already exists in hash table");
            assert(false);
            return nullptr;
        }
    }

    Pair<K, V> pair = { key, value };
    isize i = array_add(&table->table[index], pair);
    return &table->table[index][i].value;
}

template <typename V>
V* table_add(HashTable<const char*, V> *table, const char *key, V value)
{
    u32 hash  = hash32(key);
    u32 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (strcmp(table->table[index][i].key, key) == 0) {
            // TODO(jesper): to_string key
            LOG("key already exists in hash table");
            assert(false);
            return nullptr;
        }
    }

    Pair<const char*, V> pair = { key, value };
    isize i = array_add(&table->table[index], pair);
    return &table->table[index][i].value;
}

template <typename V>
V* table_add(HashTable<char*, V> *table, char *key, V value)
{
    u32 hash  = hash32(key);
    u32 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (strcmp(table->table[index][i].key, key) == 0) {
            // TODO(jesper): to_string key
            LOG("key already exists in hash table");
            assert(false);
            return nullptr;
        }
    }

    Pair<char*, V> pair = { key, value };
    isize i = array_add(&table->table[index], pair);
    return &table->table[index][i].value;
}

template <typename V>
V* table_add(HashTable<char*, V> *table, const char *key, V value)
{
    u32 hash  = hash32(key);
    u32 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (strcmp(table->table[index][i].key, key) == 0) {
            // TODO(jesper): to_string key
            LOG("key already exists in hash table");
            assert(false);
            return nullptr;
        }
    }

    Pair<char*, V> pair = { (char*)key, value };
    isize i = array_add(&table->table[index], pair);
    return &table->table[index][i].value;
}

// NOTE(jesper): IMPORTANT: the pointers return from this function should not be
// kept around, they will become invalid as the table grows, because it's being
// backed by a dynamic array instead of a linked list.
template <typename K, typename V>
V* table_find(HashTable<K, V> *table, K key)
{
    u64 hash  = hash32((void*)&key, sizeof(K));
    u64 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (table->table[index][i].key == key) {
            return &table->table[index][i].value;
        }
    }

    return nullptr;
}

template <typename V>
V* table_find(HashTable<const char*, V> *table, const char *key)
{
    u32 hash  = hash32(key);
    u32 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (strcmp(table->table[index][i].key, key) == 0) {
            return &table->table[index][i].value;
        }
    }

    return nullptr;
}

template <typename V>
V* table_find(HashTable<char*, V> *table, char *key)
{
    u32 hash  = hash32(key);
    u32 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (strcmp(table->table[index][i].key, key) == 0) {
            return &table->table[index][i].value;
        }
    }

    return nullptr;
}

template <typename V>
V* table_find(HashTable<char*, V> *table, const char *key)
{
    u32 hash  = hash32(key);
    u32 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (strcmp(table->table[index][i].key, key) == 0) {
            return &table->table[index][i].value;
        }
    }

    return nullptr;
}
