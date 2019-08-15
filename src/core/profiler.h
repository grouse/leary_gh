/**
 * file:    profiler.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2018 - all rights reserved
 */

enum ProfileEventType {
    ProfileEvent_start,
    ProfileEvent_end
};

struct ProfileEvent {
    ProfileEventType type;
    const char       *name;
    const char       *file;
    i32              line;
    u64              timestamp;
};

struct ProfileTimer {
    const char *name;
    const char *file;
    i32         line;
    u64 duration;
    u64 calls;
    i32 parent;
};

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

void profiler_start(const char *name, const char *file, i32 line);
void profiler_end(const char *name, const char *file, i32 line);

struct ProfileScope {
    const char *name;
    const char *file;
    i32         line;

    ProfileScope(const char *name, const char *file, i32 line)
    {
        this->name = name;
        this->file = file;
        this->line = line;
        profiler_start(name, file, line);
    }

    ~ProfileScope()
    {
        profiler_end(name, file, line);
    }
};


#if LEARY_ENABLE_PROFILER || 1

#define PROFILE_START(name) profiler_start(#name, __FILE__, __LINE__)
#define PROFILE_END(name)   profiler_end(#name, __FILE__, __LINE__)

#define PROFILE_SCOPE(name)\
    ProfileScope MCOMBINE(profile_scope, __LINE__)(#name, __FILE__, __LINE__)
#define PROFILE_FUNCTION()\
    ProfileScope MCOMBINE(profile_scope, __LINE__)(__FUNCTION__, __FILE__, __LINE__)

#else

#define PROFILE_START(...)    do {} while(0)
#define PROFILE_END(...)      do {} while(0)
#define PROFILE_SCOPE(...)    do {} while(0)
#define PROFILE_FUNCTION(...) do {} while(0)

#endif // LEARY_ENABLE_PROFILER