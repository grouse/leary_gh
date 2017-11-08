/**
 * file:    profiling.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PROFILING_H
#define PROFILING_H

#ifndef PROFILE_TIMERS_ENABLE
#define PROFILE_TIMERS_ENABLE 1
#endif

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

#if PROFILE_TIMERS_ENABLE

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

#define PROFILE_START(...)
#define PROFILE_END(...)

#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION(...)

#endif

#endif /* PROFILING_H */

