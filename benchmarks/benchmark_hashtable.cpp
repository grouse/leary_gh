/**
 * file:    benchmark_hashtable.cpp
 * created: 2017-11-15
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <unordered_map>

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
