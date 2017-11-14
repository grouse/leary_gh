/**
 * file:    benchmark_array.cpp
 * created: 2017-08-22
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

BENCHMARK_FUNC(array_remove_front)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator  = &g_allocator;

    array_resize(&arr, state->max_iterations);
    defer { array_destroy(&arr); };

    while (keep_running(state)) {

        start_timing(state);
        array_remove(&arr, 0);
        stop_timing(state);
    }
}
BENCHMARK(array_remove_front);

BENCHMARK_FUNC(array_remove_ordered)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator = &g_allocator;

    array_resize(&arr, state->max_iterations);
    defer { array_destroy(&arr); };

    while (keep_running(state)) {
        start_timing(state);
        array_remove_ordered(&arr, 0);
        stop_timing(state);
    }

}
BENCHMARK(array_remove_ordered);

BENCHMARK_FUNC(array_remove_back)
{
    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator = &g_allocator;

    array_resize(&arr, state->max_iterations);
    defer { array_destroy(&arr); };

    while (keep_running(state)) {
        start_timing(state);
        array_remove(&arr, arr.count-1);
        stop_timing(state);
    }

}
BENCHMARK(array_remove_back);

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

#if 0
void benchmark_removal_back()
{
    SystemAllocator allocator = {};

    struct s4b  { i32 a[1]; };
    struct s8b  { i32 a[2]; };
    struct s16b { i32 a[4]; };
    struct s64b { i32 a[16]; };

    i64 c4b, c8b, c16b, c64b;
    {
        Array<s4b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 2047; i >= 0; i--) {
            array_remove(&arr, i);
        }
        timespec t1 = get_time();
        c4b = get_time_difference(t0, t1);
    }

    {
        Array<s8b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 2047; i >= 0; i--) {
            array_remove(&arr, i);
        }
        timespec t1 = get_time();
        c8b = get_time_difference(t0, t1);
    }

    {
        Array<s16b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 2047; i >= 0; i--) {
            array_remove(&arr, i);
        }
        timespec t1 = get_time();
        c16b = get_time_difference(t0, t1);
    }

    {
        Array<s64b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 2047; i >= 0; i--) {
            array_remove(&arr, i);
        }
        timespec t1 = get_time();
        c64b = get_time_difference(t0, t1);
    }

    i64 v4b, v8b, v16b, v64b;
    {
        std::vector<s4b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.pop_back();
        }
        timespec t1 = get_time();
        v4b = get_time_difference(t0, t1);
    }

    {
        std::vector<s8b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.pop_back();
        }
        timespec t1 = get_time();
        v8b = get_time_difference(t0, t1);
    }

    {
        std::vector<s16b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.pop_back();
        }
        timespec t1 = get_time();
        v16b = get_time_difference(t0, t1);
    }

    {
        std::vector<s64b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.pop_back();
        }
        timespec t1 = get_time();
        v64b = get_time_difference(t0, t1);
    }

    f64 cavg = (c4b + c8b + c16b + c64b) / (2048.0 * 4.0);
    f64 vavg = (v4b + v8b + v16b + v64b) / (2048.0 * 4.0);

    printf("2048 rem. back\t4b\t8b\t16b\t64b\tavg/removal\n");
    printf("custom array\t%ldns\t%ldns\t%ldns\t%ldns\t%fns\n", c4b, c8b, c16b, c64b, cavg);
    printf("std::vector\t%ldns\t%ldns\t%ldns\t%ldns\t%fns\n\n", v4b, v8b, v16b, v64b, vavg);
}
void benchmark_removal_front()
{
    SystemAllocator allocator = {};

    struct s4b  { i32 a[1]; };
    struct s8b  { i32 a[2]; };
    struct s16b { i32 a[4]; };
    struct s64b { i32 a[16]; };

    i64 c4b, c8b, c16b, c64b;
    {
        Array<s4b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_remove(&arr, 0);
        }
        timespec t1 = get_time();
        c4b = get_time_difference(t0, t1);
    }

    {
        Array<s8b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_remove(&arr, 0);
        }
        timespec t1 = get_time();
        c8b = get_time_difference(t0, t1);
    }

    {
        Array<s16b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_remove(&arr, 0);
        }
        timespec t1 = get_time();
        c16b = get_time_difference(t0, t1);
    }

    {
        Array<s64b> arr = {};
        arr.allocator = &allocator;
        array_resize(&arr, 2048);
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_remove(&arr, 0);
        }
        timespec t1 = get_time();
        c64b = get_time_difference(t0, t1);
    }

    i64 v4b, v8b, v16b, v64b;
    {
        std::vector<s4b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.erase(v.begin());
        }
        timespec t1 = get_time();
        v4b = get_time_difference(t0, t1);
    }

    {
        std::vector<s8b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.erase(v.begin());
        }
        timespec t1 = get_time();
        v8b = get_time_difference(t0, t1);
    }

    {
        std::vector<s16b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.erase(v.begin());
        }
        timespec t1 = get_time();
        v16b = get_time_difference(t0, t1);
    }

    {
        std::vector<s64b> v;
        v.resize(2048);

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.erase(v.begin());
        }
        timespec t1 = get_time();
        v64b = get_time_difference(t0, t1);
    }

    f64 cavg = (c4b + c8b + c16b + c64b) / (2048.0 * 4.0);
    f64 vavg = (v4b + v8b + v16b + v64b) / (2048.0 * 4.0);

    printf("2048 rem. front\t4b\t8b\t16b\t64b\tavg/removal\n");
    printf("custom array\t%ldns\t%ldns\t%ldns\t%ldns\t%fns\n", c4b, c8b, c16b, c64b, cavg);
    printf("std::vector\t%ldns\t%ldns\t%ldns\t%ldns\t%fns\n\n", v4b, v8b, v16b, v64b, vavg);
}

void benchmark_insertion()
{
    SystemAllocator allocator = {};

    struct s4b  { i32 a[1]; };
    struct s8b  { i32 a[2]; };
    struct s16b { i32 a[4]; };
    struct s64b { i32 a[16]; };

    i64 c4b, c8b, c16b, c64b;
    {
        Array<s4b> arr = {};
        arr.allocator = &allocator;
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_add(&arr, {});
        }
        timespec t1 = get_time();
        c4b = get_time_difference(t0, t1);
    }

    {
        Array<s8b> arr = {};
        arr.allocator = &allocator;
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_add(&arr, {});
        }
        timespec t1 = get_time();
        c8b = get_time_difference(t0, t1);
    }

    {
        Array<s16b> arr = {};
        arr.allocator = &allocator;
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_add(&arr, {});
        }
        timespec t1 = get_time();
        c16b = get_time_difference(t0, t1);
    }

    {
        Array<s64b> arr = {};
        arr.allocator = &allocator;
        defer { allocator.dealloc(arr.data); };

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            array_add(&arr, {});
        }
        timespec t1 = get_time();
        c64b = get_time_difference(t0, t1);
    }

    i64 v4b, v8b, v16b, v64b;
    {
        std::vector<s4b> v;

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.push_back({});
        }
        timespec t1 = get_time();
        v4b = get_time_difference(t0, t1);
    }

    {
        std::vector<s8b> v;

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.push_back({});
        }
        timespec t1 = get_time();
        v8b = get_time_difference(t0, t1);
    }

    {
        std::vector<s16b> v;

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.push_back({});
        }
        timespec t1 = get_time();
        v16b = get_time_difference(t0, t1);
    }

    {
        std::vector<s64b> v;

        timespec t0 = get_time();
        for (i32 i = 0; i < 2048; i++) {
            v.push_back({});
        }
        timespec t1 = get_time();
        v64b = get_time_difference(t0, t1);
    }

    f64 cavg = (c4b + c8b + c16b + c64b) / (2048.0 * 4.0);
    f64 vavg = (v4b + v8b + v16b + v64b) / (2048.0 * 4.0);

    printf("2048 ins. back\t4b\t8b\t16b\t64b\tavg/insert\n");
    printf("custom array\t%ldns\t%ldns\t%ldns\t%ldns\t%fns\n", c4b, c8b, c16b, c64b, cavg);
    printf("std::vector\t%ldns\t%ldns\t%ldns\t%ldns\t%fns\n\n", v4b, v8b, v16b, v64b, vavg);
}

void benchmark_array()
{
    benchmark_insertion();
    benchmark_removal_front();
    benchmark_removal_back();
}

#endif
