/**
 * file:    profiler.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_PROFILER_H
#define LEARY_PROFILER_H

#include "build_config.h"
#include "core/types.h"

u64 cpu_ticks()
{
#if defined(_WIN32)
    int registers[4];
    __cpuid(registers, 0);
    return __rdtsc();
#else
    return __rdtsc();
#endif
}

void init_profiler();
void profiler_begin_frame();
void profiler_end_frame();

void profiler_start(const char *name);
void profiler_end(const char *name);

struct ProfileScope {
    const char *name;

    ProfileScope(const char *name)
    {
        this->name = name;
        profiler_start(name);
    }

    ~ProfileScope()
    {
        profiler_end(name);
    }
};


#if LEARY_ENABLE_PROFILER || 1

#define PROFILE_START(name) profiler_start(#name)
#define PROFILE_END(name)   profiler_end(#name)

#define PROFILE_SCOPE(name) ProfileScope MCOMBINE(profile_scope, __LINE__)(#name)
#define PROFILE_FUNCTION()  ProfileScope MCOMBINE(profile_scope, __LINE__)(__FUNCTION__)

#else

#define PROFILE_START(...)    do {} while(0)
#define PROFILE_END(...)      do {} while(0)
#define PROFILE_SCOPE(...)    do {} while(0)
#define PROFILE_FUNCTION(...) do {} while(0)

#endif // LEARY_ENABLE_PROFILER

#endif /* LEARY_PROFILER_H */

