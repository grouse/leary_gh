/**
 * file:    benchmark_hashtable.cpp
 * created: 2017-11-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#include <unordered_map>

#define HASHMAP_FIND_LOAD (128)

BENCHMARK_FUNC(rh_hashmap_add)
{
    auto r  = create_random(0xDEADBEEF);

    RHHashMap<u32, u32> map;
    init_map(&map, &g_allocator);
    defer { destroy_map(&map); };

    while (keep_running(state)) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);

        start_timing(state);
        map_add(&map, k, v);
        stop_timing(state);
    }
}
BENCHMARK(rh_hashmap_add);

BENCHMARK_FUNC(rh_hashmap_find)
{
    auto r  = create_random(0xDEADBEEF);

    RHHashMap<u32, u32> map;
    init_map(&map, &g_allocator);
    defer { destroy_map(&map); };

    for (i32 i = 0; i < HASHMAP_FIND_LOAD; i++) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);
        map_add(&map, k, v);
    }

    u32 fk = next_u32(&r);
    u32 fv = next_u32(&r);
    map_add(&map, fk, fv);

    for (i32 i = 0; i < HASHMAP_FIND_LOAD; i++) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);
        map_add(&map, k, v);
    }

    MEMORY_BARRIER();

    while (keep_running(state)) {
        start_timing(state);
        DONT_OPTIMIZE(map_find(&map, fk));
        stop_timing(state);
    }
}
BENCHMARK(rh_hashmap_find);


BENCHMARK_FUNC(hashtable_add)
{
    auto r  = create_random(0xDEADBEEF);
    auto ht = create_hashtable<u32, u32>(&g_allocator);
    defer { destroy_hashtable(&ht); };

    while (keep_running(state)) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);

        start_timing(state);
        table_add(&ht, k, v);
        stop_timing(state);
    }
}
BENCHMARK(hashtable_add);

BENCHMARK_FUNC(hashtable_find)
{
    auto r  = create_random(0xDEADBEEF);
    auto ht = create_hashtable<u32, u32>(&g_allocator);
    defer { destroy_hashtable(&ht); };

    for (i32 i = 0; i < HASHMAP_FIND_LOAD; i++) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);
        table_add(&ht, k, v);
    }

    u32 fk = next_u32(&r);
    u32 fv = next_u32(&r);
    table_add(&ht, fk, fv);

    for (i32 i = 0; i < HASHMAP_FIND_LOAD; i++) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);
        table_add(&ht, k, v);
    }

    MEMORY_BARRIER();

    while (keep_running(state)) {
        start_timing(state);
        DONT_OPTIMIZE(table_find(&ht, fk));
        stop_timing(state);
    }
}
BENCHMARK(hashtable_find);

//// std::unorderd_map
BENCHMARK_FUNC(std_unordered_map_add)
{
    auto r  = create_random(0xDEADBEEF);
    std::unordered_map<u32, u32> ht;

    while (keep_running(state)) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);

        start_timing(state);
        ht[k] = v;
        stop_timing(state);
    }
}
BENCHMARK(std_unordered_map_add);

BENCHMARK_FUNC(std_unordered_map_find)
{
    auto r  = create_random(0xDEADBEEF);
    std::unordered_map<u32, u32> ht;

    for (i32 i = 0; i < HASHMAP_FIND_LOAD; i++) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);
        ht[k] = v;
    }

    u32 fk = next_u32(&r);
    u32 fv = next_u32(&r);
    ht[fk] = fv;

    for (i32 i = 0; i < HASHMAP_FIND_LOAD; i++) {
        u32 k = next_u32(&r);
        u32 v = next_u32(&r);
        ht[k] = v;
    }

    MEMORY_BARRIER();

    while (keep_running(state)) {
        start_timing(state);
        DONT_OPTIMIZE(ht[fk]);
        stop_timing(state);
    }
}
BENCHMARK(std_unordered_map_find);
