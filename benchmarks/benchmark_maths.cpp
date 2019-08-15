/**
 * file:    benchmark_maths.cpp
 * created: 2017-11-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017-2018 - all rights reserved
 */

#include <cmath>
#include "core/maths.h"

BENCHMARK_FUNC(cos)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(lry::cos(f));
        stop_timing(state);
    }
}
BENCHMARK(cos);

BENCHMARK_FUNC(sin_taylor)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(lry::sin_taylor(f));
        stop_timing(state);
    }
}
BENCHMARK(sin_taylor);

BENCHMARK_FUNC(sin_cephes)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(lry::sin_cephes(f));
        stop_timing(state);
    }
}
BENCHMARK(sin_cephes);

BENCHMARK_FUNC(tan)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(lry::tan(f));
        stop_timing(state);
    }
}
BENCHMARK(tan);

BENCHMARK_FUNC(sqrt)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(lry::sqrt(f));
        stop_timing(state);
    }
}
BENCHMARK(sqrt);


/// std::math
BENCHMARK_FUNC(std_cos)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(std::cos(f));
        stop_timing(state);
    }
}
BENCHMARK(std_cos);

BENCHMARK_FUNC(std_sin)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(std::sin(f));
        stop_timing(state);
    }
}
BENCHMARK(std_sin);

BENCHMARK_FUNC(std_tan)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(std::tan(f));
        stop_timing(state);
    }
}
BENCHMARK(std_tan);

BENCHMARK_FUNC(std_sqrt)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        f32 f = next_f32(&r);
        MEMORY_BARRIER();

        start_timing(state);
        DONT_OPTIMIZE(std::sqrt(f));
        stop_timing(state);
    }
}
BENCHMARK(std_sqrt);
