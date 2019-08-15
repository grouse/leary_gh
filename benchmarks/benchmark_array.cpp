/**
 * file:    benchmark_array.cpp
 * created: 2017-08-22
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

BENCHMARK_FUNC(array_remove_front)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator  = &g_allocator;

    array_resize(&arr, state->max_iterations);
    defer { destroy_array(&arr); };

    while (keep_running(state)) {
        start_timing(state);
        array_remove(&arr, 0);
        stop_timing(state);
    }
}
BENCHMARK(array_remove_front);

BENCHMARK_FUNC(array_remove_front_ordered)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator = &g_allocator;

    array_resize(&arr, state->max_iterations);
    defer { destroy_array(&arr); };

    while (keep_running(state)) {
        start_timing(state);
        array_remove_ordered(&arr, 0);
        stop_timing(state);
    }

}
BENCHMARK(array_remove_front_ordered);

BENCHMARK_FUNC(array_remove_back)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator = &g_allocator;

    array_resize(&arr, state->max_iterations);
    defer { destroy_array(&arr); };

    while (keep_running(state)) {
        start_timing(state);
        array_remove(&arr, arr.count-1);
        stop_timing(state);
    }

}
BENCHMARK(array_remove_back);

BENCHMARK_FUNC(array_add_back)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator = &g_allocator;
    defer { destroy_array(&arr); };

    while (keep_running(state)) {
        start_timing(state);
        array_add(&arr, {});
        stop_timing(state);
    }
}
BENCHMARK(array_add_back);



///// std::vector
BENCHMARK_FUNC(std_vector_remove_front)
{
    struct s4b { i32 a[1]; };
    std::vector<s4b> v;

    while (keep_running(state)) {
        v.resize(state->max_iterations);

        start_timing(state);
        v.erase(v.begin());
        stop_timing(state);
    }
}
BENCHMARK(std_vector_remove_front);

BENCHMARK_FUNC(std_vector_remove_back)
{
    struct s4b { i32 a[1]; };
    std::vector<s4b> v;

    while (keep_running(state)) {
        v.resize(state->max_iterations);

        start_timing(state);
        v.erase(v.end() - 1);
        stop_timing(state);
    }
}
BENCHMARK(std_vector_remove_back);

BENCHMARK_FUNC(std_vector_add_back)
{
    struct s4b { i32 a[1]; };
    std::vector<s4b> v;

    while (keep_running(state)) {
        start_timing(state);
        v.push_back({});
        stop_timing(state);
    }
}
BENCHMARK(std_vector_add_back);
