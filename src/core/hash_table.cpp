/**
 * file:    hash_table.cpp
 * created: 2017-08-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#include "hash_table.h"
#include "string.h"

template <typename K, typename V>
void init_table(HashTable<K, V> *table, Allocator *a)
{
    *table = {};
    table->allocator = a;

    for (i32 i = 0; i < TABLE_SIZE; i++) {
        table->table[i].allocator = a;
    }
}

template <typename K, typename V>
HashTable<K, V> create_hashtable(Allocator *a)
{
    HashTable<K, V> table = {};
    for (i32 i = 0; i < TABLE_SIZE; i++) {
        table.table[i].allocator = a;
    }
    return table;
}

template <typename K, typename V>
void destroy_hashtable(HashTable<K, V> *table)
{
    for (i32 i = 0; i < TABLE_SIZE; i++) {
        destroy_array(&table->table[i]);
    }
}

template <typename V>
void destroy_hashtable(HashTable<StringView, V> *table)
{
    for (i32 i = 0; i < TABLE_SIZE; i++) {
        for (i32 j = 0; j < table->table[i].count; j++) {
            table->allocator->dealloc(table->table[i][j].key.bytes);
        }
        destroy_array(&table->table[i]);
    }
}



template <typename K, typename V>
V* table_add(HashTable<K, V> *table, K key, V value)
{
    u64 hash  = hash32(&key);
    u64 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (table->table[index][i].key == key) {
            // TODO(jesper): to_string key
            LOG("key already exists in hash table");
            ASSERT(false);
            return nullptr;
        }
    }

    Pair<K, V> pair = { key, value };
    i32 i = array_add(&table->table[index], pair);
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
            ASSERT(false);
            return nullptr;
        }
    }

    Pair<const char*, V> pair = { key, value };
    i32 i = array_add(&table->table[index], pair);
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
            ASSERT(false);
            return nullptr;
        }
    }

    Pair<char*, V> pair = { key, value };
    i32 i = array_add(&table->table[index], pair);
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
            ASSERT(false);
            return nullptr;
        }
    }

    Pair<char*, V> pair = { (char*)key, value };
    i32 i = array_add(&table->table[index], pair);
    return &table->table[index][i].value;
}

template<typename V>
V* table_add(HashTable<StringView, V> *table, StringView key, V value)
{
    u64 hash  = hash32(&key);
    u64 index = hash % TABLE_SIZE;

    for (i32 i = 0; i < table->table[index].count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (table->table[index][i].key == key) {
            // TODO(jesper): to_string key
            LOG("key already exists in hash table");
            ASSERT(false);
            return nullptr;
        }
    }

    String str_key = create_string(table->allocator, key);
    Pair<StringView, V> pair = { str_key, value };
    i32 i = array_add(&table->table[index], pair);
    return &table->table[index][i].value;
}

// NOTE(jesper): IMPORTANT: the pointers return from this function should not be
// kept around, they will become invalid as the table grows, because it's being
// backed by a dynamic array instead of a linked list.
template <typename K, typename V>
V* table_find(HashTable<K, V> *table, K key)
{
    u64 hash  = hash32(&key);
    u64 index = hash % TABLE_SIZE;

    auto ltable = *table;

    for (i32 i = 0, count = ltable.table[index].count; i < count; i++) {
        // TODO(jesper): add a comparator template argument? only allow keys
        // with defined operator==? hmmm
        if (ltable.table[index][i].key == key) {
            return &ltable.table[index][i].value;
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


template<typename K, typename V>
void init_map(
    RHHashMap<K, V> *map,
    Allocator *a,
    i32 initial_size = RH_INITIAL_SIZE)
{
    using Entry = typename RHHashMap<K, V>::Entry;

    map->allocator = a;
    map->capacity = initial_size;
    map->mask     = map->capacity - 1;
    map->entries  = ialloc_array<Entry>(a, map->capacity);
    map->resize_threshold = (map->capacity * RH_LOAD_FACTOR) / 100;

}

template<typename K, typename V>
void destroy_map(RHHashMap<K, V> *map)
{
    for (i32 i = 0; i < map->count; i++) {
        map->entries[i].value.~V();
    }

    map->allocator->dealloc(map->entries);
    *map = {};
}

template<typename V>
void destroy_map(RHHashMap<StringView, V> *map)
{
    for (i32 i = 0; i < map->count; i++) {
        map->entries[i].value.~V();
        map->allocator->dealloc(map->entries[i].key);
    }

    map->allocator->dealloc(map->entries);
    *map = {};
}

template<typename K, typename V>
void map_add(RHHashMap<K, V> *map, K key, V value)
{
    using Entry = typename RHHashMap<K, V>::Entry;

    if (++map->count >= map->resize_threshold) {
        auto a = map->allocator;

        i32 capacity = map->capacity * 2;
        Entry *entries = ialloc_array<Entry>(a, capacity);
        std::memcpy(entries, map->entries, map->capacity);
        dealloc(a, map->entries);

        map->capacity = capacity;
        map->mask     = map->capacity - 1;
        map->entries  = entries;
        map->resize_threshold = (map->capacity * RH_LOAD_FACTOR) / 100;
    }

    u32 hash = hash32(&key);
    u32 index = hash & map->mask;

    Entry e = {
        std::move(key),
        std::move(value),
        0
    };

    for (i32 i = (i32)index; ; i = (i + 1) & map->mask) {
        if (map->entries[i].distance == -1) {
            map->entries[i] = std::move(e);
            return;
        }

        if (map->entries[i].distance < e.distance) {
            std::swap(e, map->entries[i]);
        }

        e.distance++;
    }
}

template<typename V>
void map_add(RHHashMap<StringView, V> *map, StringView key, V value)
{
    using Entry = typename RHHashMap<StringView, V>::Entry;

    if (++map->count >= map->resize_threshold) {
        auto a = map->allocator;

        i32 capacity = map->capacity * 2;
        Entry *entries = ialloc_array<Entry>(a, capacity);
        std::memcpy(entries, map->entries, map->capacity);
        dealloc(a, map->entries);

        map->capacity = capacity;
        map->mask     = map->capacity - 1;
        map->entries  = entries;
        map->resize_threshold = (map->capacity * RH_LOAD_FACTOR) / 100;
    }

    u32 hash  = hash32(&key);
    u32 index = hash & map->mask;

    Entry e = {};
    e.key      = create_string(map->allocator, key);
    e.value    = std::move(value);
    e.distance = 0;

    for (i32 i = (i32)index; ; i = (i + 1) & map->mask) {
        if (map->entries[i].distance == -1) {
            map->entries[i] = std::move(e);
            return;
        }

        if (map->entries[i].distance < e.distance) {
            std::swap(e, map->entries[i]);
        }

        e.distance++;
    }
}

template<typename K, typename V>
V* map_find(RHHashMap<K, V> *map, K key)
{
    u32 hash = hash32(&key);
    u32 index = hash & map->mask;
    i32 distance = 0;

    for (;;) {
        if (map->entries[index].distance == -1 ||
            distance > map->entries[index].distance)
        {
            return nullptr;
        }

        if (map->entries[index].key == key) {
            return &map->entries[index].value;
        }

        index = (index+1) & map->mask;
        distance++;
    }
}
