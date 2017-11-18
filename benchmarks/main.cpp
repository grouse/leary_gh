/**
 * file:    main.cpp
 * created: 2017-08-22
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include <time.h>
#include <vector>

#define LEARY_ENABLE_LOGGING 0
#define LEARY_ENABLE_PROFILING 0

#include "platform/platform.h"

#if defined(_WIN32)
    #include "platform/win32_debug.cpp"
    #include "platform/win32_file.cpp"
#elif defined(__linux__)
    #include "platform/linux_debug.cpp"
    #include "platform/linux_file.cpp"
#else
    #error "unsupported platform"
#endif

#include "core/profiling.cpp"
#include "core/lexer.cpp"
#include "core/string.cpp"
#include "core/random.cpp"
#include "core/hash.cpp"
#include "core/hash_table.cpp"
#include "core/allocator.cpp"
#include "core/array.cpp"
#include "core/maths.cpp"

#if defined(__clang__)
#define DONT_OPTIMIZE(value) asm volatile("" : : "g"(value) : "memory")
#else
#define DONT_OPTIMIZE(value) asm volatile("" : : "i,r,m"(value) : "memory")
#endif

#define MEMORY_BARRIER() asm volatile("" : : : "memory")

#define DIVIDER "-----------------------------------------------------------------------------------------------------------------------------------------------------"

u64 get_time()
{
#if defined(__x86_64__) || defined(__amd64__)
  u64 low, high;
  __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
  return (high << 32) | low;
#endif
}

#define MCOMBINE2(a, b) a ## b
#define MCOMBINE(a, b) MCOMBINE2(a, b)

#define BENCHMARK_FUNC(name) void MCOMBINE(benchmark_, name)(Benchmark *state)
#define BENCHMARK(name) \
    static Benchmark MCOMBINE(benchmark_var_, name) = create_benchmark(#name, &benchmark_##name)

struct Benchmark;
typedef void benchmark_t(Benchmark *);

struct Benchmark {
    const char  *name;
    benchmark_t *func;

    u64 start_time     = 0;
    u64 end_time       = 0;
    u64 total_duration = 0;

    i32 iterations     = 0;
    i32 max_iterations = 2048;

    f64 avg = 0.0f;
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

    u64 duration = state->end_time - state->start_time;
    state->total_duration += duration;
}

#include "benchmark_array.cpp"
#include "benchmark_hashtable.cpp"
#include "benchmark_maths.cpp"

int main()
{
    for (auto &benchmark : g_benchmarks) {
        benchmark.func(&benchmark);
    }

    i32 pad = 5;
    i32 col0 = strlen("Benchmark");
    i32 col1 = strlen("Iterations");
    i32 col2 = strlen("Cycles");

    for (auto &benchmark : g_benchmarks) {
        if ((i32)strlen(benchmark.name) > col0) {
            col0 = strlen(benchmark.name);
        }

        i32 iter = std::snprintf(nullptr, 0, "%d", benchmark.iterations);
        if (iter > col1) {
            col1 = iter;
        }

        benchmark.avg = benchmark.total_duration / (f64)benchmark.iterations;
        i32 avg = std::snprintf(nullptr, 0, "%f", benchmark.avg);
        if (avg > col2) {
            col2 = avg;
        }
    }

    col0 += pad;
    col1 += pad;
    col2 += pad;

    printf("%.*s\n", col0 + col1 + col2, DIVIDER);
    printf("%-*s %-*s %-*s\n",
           col0, "Benchmark",
           col1, "Iterations",
           col2, "Cycles");
    printf("%.*s\n", col0 + col1 + col2, DIVIDER);

    for (auto &benchmark : g_benchmarks) {
        printf("%-*s %-*d %-*f\n",
               col0, benchmark.name,
               col1, benchmark.iterations,
               col2, benchmark.avg);
    }

    return 0;
}
