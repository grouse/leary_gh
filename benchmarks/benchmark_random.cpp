
BENCHMARK_FUNC(random)
{
    Random r = create_random(0xdeadbeef);

    while (keep_running(state)) {
        DONT_OPTIMIZE(r.state);
        DONT_OPTIMIZE(r.seed);
        DONT_OPTIMIZE(r);

        start_timing(state);
        DONT_OPTIMIZE(next_i32(&r));
        MEMORY_BARRIER();
        stop_timing(state);

    }
}
BENCHMARK(random);
