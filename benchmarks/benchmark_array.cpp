/**
 * file:    benchmark_array.cpp
 * created: 2017-08-22
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <vector>

void benchmark_removal_back()
{
    printf("\n\nbenchmark of 2048 consecutive deletions from the back of array\n");
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

    printf("            : 4b\t8b\t16b\t64b\tavg/removal\n");
    printf("custom array: %ldns\t%ldns\t%ldns\t%ldns\t%fns\n", c4b, c8b, c16b, c64b, cavg);
    printf("std::vector : %ldns\t%ldns\t%ldns\t%ldns\t%fns\n", v4b, v8b, v16b, v64b, vavg);
}
void benchmark_removal_front()
{
    printf("\n\nbenchmark of 2048 consecutive deletions from the front of array\n");
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

    printf("              : 4b\t8b\t16b\t64b\tavg/removal\n");
    printf("custom array  : %ldns\t%ldns\t%ldns\t%ldns\t%fns\n", c4b, c8b, c16b, c64b, cavg);
    printf("std::vector   : %ldns\t%ldns\t%ldns\t%ldns\t%fns\n", v4b, v8b, v16b, v64b, vavg);
}

void benchmark_insertion()
{
    printf("\n\nbenchmark of 2048 consecutive insertions into back of array\n");
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

    printf("              : 4b\t8b\t16b\t64b\tavg/insert\n");
    printf("custom array  : %ldns\t%ldns\t%ldns\t%ldns\t%fns\n", c4b, c8b, c16b, c64b, cavg);
    printf("std::vector   : %ldns\t%ldns\t%ldns\t%ldns\t%fns\n", v4b, v8b, v16b, v64b, vavg);
}

void benchmark_array()
{
    benchmark_insertion();
    benchmark_removal_front();
    benchmark_removal_back();
}


