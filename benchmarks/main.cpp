/**
 * file:    main.cpp
 * created: 2017-08-22
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <time.h>

#include "leary.h"
#include "platform/platform.h"

#include "core/lexer.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"

LinearAllocator *g_debug_frame;

#if defined(_WIN32)
    #include "platform/win32_debug.cpp"
    #include "platform/win32_file.cpp"
#elif defined(__linux__)
    #include "platform/linux_debug.cpp"
    #include "platform/linux_file.cpp"
#else
    #error "unsupported platform"
#endif


timespec get_time()
{
    timespec ts;
    i32 result = clock_gettime(CLOCK_MONOTONIC, &ts);
    (void)result;
    return ts;
}

i64 get_time_difference(timespec start, timespec end)
{
    i64 difference = (end.tv_sec - start.tv_sec) * 1000000000 +
                     (end.tv_nsec - start.tv_nsec);
    return difference;
}

#include "benchmark_array.cpp"

#define BENCHMARK_FUNC(name) void benchmark_##name(Benchmark *state)
#define BENCHMARK(name) \
    static Benchmark benchmark_var_##name = create_benchmark(#name, &benchmark_##name)

struct Benchmark;
typedef void benchmark_t(Benchmark *);

struct Benchmark {
    const char  *name;
    benchmark_t *func;

    timespec start_time;
    timespec end_time;
    i64 total_duration = 0;
    i32 iterations     = 0;
    i32 max_iterations = 2048;
};

static SystemAllocator  g_allocator  = {};
static Array<Benchmark> g_benchmarks = create_array<Benchmark>(&g_allocator);

Benchmark create_benchmark(const char *name, benchmark_t *func)
{
    Benchmark bm = {};
    bm.name = name;
    bm.func = func;

    array_add(&g_benchmarks, bm);
    return bm;
}

bool keep_running(Benchmark *state)
{
    return state->iterations++ < state->max_iterations;
}

void start_timing(Benchmark *state)
{
    state->start_time = get_time();
}

void stop_timing(Benchmark *state)
{
    state->end_time = get_time();

    i64 duration = get_time_difference(state->start_time, state->end_time);
    state->total_duration += duration;
}

BENCHMARK_FUNC(array_remove_front)
{
    SystemAllocator allocator = {};

    struct s4b { i32 a[1]; };

    Array<s4b> arr = {};
    arr.allocator  = &allocator;

    while (keep_running(state)) {
        array_resize(&arr, state->max_iterations);
        defer { array_destroy(&arr); };

        start_timing(state);
        array_remove(&arr, 0);
        stop_timing(state);
    }
}
BENCHMARK(array_remove_front);

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

void report_benchmarks()
{
    printf("benchmark      iterations      time\n");

    for (auto &benchmark : g_benchmarks) {
        f64 avg = benchmark.total_duration / (f64)benchmark.iterations;
        printf("%s: %d        %fns\n",
               benchmark.name, benchmark.iterations, avg);
    }
}

int main()
{
    isize debug_frame_size = 64  * 1024 * 1024;
    void *debug_frame_mem = malloc(debug_frame_size);
    g_debug_frame = new LinearAllocator(debug_frame_mem, debug_frame_size);

    for (auto &benchmark : g_benchmarks) {
        benchmark.func(&benchmark);
    }
    report_benchmarks();

    return 0;
}
