/**
 * file:    profiling.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_PROFILING_H
#define LEARY_PROFILING_H

#include "build_config.h"
#include "core/types.h"

struct ProfileTimer {
    const char *name;
    u64 cycles;
    u32 calls;
    bool open;
};

struct ProfileTimers {
    i32        count;
    i32        capacity;
    const char **name;
    u64        *cycles;
    u32        *calls;
    bool       *open;
};

extern ProfileTimers g_profile_timers;
extern ProfileTimers g_profile_timers_prev;

u64 rdtsc()
{
#if defined(_WIN32)
    int registers[4];
    __cpuid(registers, 0);
    return __rdtsc();
#else
    return __rdtsc();
#endif
}


#if LEARY_ENABLE_PROFILING

i32 profile_start_timer(const char *name);
void profile_end_timer(i32 index, i64 cycles);

struct ProfileBlock {
    i32 id;
    u64 start_cycles;

    ProfileBlock(const char *name) {
        this->id = profile_start_timer(name);
        this->start_cycles = rdtsc();
    }

    ~ProfileBlock() {
        u64 end_cycles = rdtsc();
        profile_end_timer(this->id, end_cycles - start_cycles);
    }
};

#define NUM_PROFILE_TIMERS (256)

#define PROFILE_START(name)\
    u64 start_##name = rdtsc();\
    i32 profile_timer_id_##name = profile_start_timer(#name)

#define PROFILE_END(name)\
    u64 end_##name = rdtsc();\
    u64 difference_##name = end_##name - start_##name;\
    profile_end_timer(profile_timer_id_##name, difference_##name)

#define PROFILE_BLOCK(name) ProfileBlock profile_block_##__LINE__(#name)
#define PROFILE_FUNCTION() ProfileBlock profile_block_##__FUNCTION__(__FUNCTION__)

#else

#define PROFILE_START(...)    do {} while(0)
#define PROFILE_END(...)      do {} while(0)
#define PROFILE_BLOCK(...)    do {} while(0)
#define PROFILE_FUNCTION(...) do {} while(0)

#endif // LEARY_ENABLE_PROFILING

#endif /* LEARY_PROFILING_H */

