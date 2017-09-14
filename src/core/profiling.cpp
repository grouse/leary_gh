/**
 * file:    profiling.cpp
 * created: 2017-02-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "profiling.h"

#if PROFILE_TIMERS_ENABLE

ProfileTimers g_profile_timers;
ProfileTimers g_profile_timers_prev;

void init_profiling()
{
    g_profile_timers.count    = 0;
    g_profile_timers.capacity = NUM_PROFILE_TIMERS;
    g_profile_timers.name     = g_persistent->alloc_array<const char*>(NUM_PROFILE_TIMERS);
    g_profile_timers.cycles   = g_persistent->alloc_array<u64>(NUM_PROFILE_TIMERS);
    g_profile_timers.calls    = g_persistent->alloc_array<u32>(NUM_PROFILE_TIMERS);
    g_profile_timers.open     = g_persistent->alloc_array<bool>(NUM_PROFILE_TIMERS);

    g_profile_timers_prev.count    = 0;
    g_profile_timers_prev.capacity = NUM_PROFILE_TIMERS;
    g_profile_timers_prev.name     = g_persistent->alloc_array<const char*>(NUM_PROFILE_TIMERS);
    g_profile_timers_prev.cycles   = g_persistent->alloc_array<u64>(NUM_PROFILE_TIMERS);
    g_profile_timers_prev.calls    = g_persistent->alloc_array<u32>(NUM_PROFILE_TIMERS);
    g_profile_timers_prev.open     = g_persistent->alloc_array<bool>(NUM_PROFILE_TIMERS);
}


i32 profile_start_timer(const char *name)
{
    for (i32 i = 0; i < g_profile_timers.count; i++) {
        // TODO(jesper): hash the name and use as identifier
        if (strcmp(name, g_profile_timers.name[i]) == 0) {
            g_profile_timers.open[i] = true;
            g_profile_timers.calls[i]++;
            return i;
        }
    }

    i32 index = (i32)g_profile_timers.count++;
    // NOTE(jesper): using a static array here for memory performance reasons,
    // so if we run out it's tough shit and you better increase
    // NUM_PROFILE_TIMERS
    assert(g_profile_timers.count < g_profile_timers.capacity);

    // NOTE(jesper): assume the passed in string won't be deallocated, I don't
    // see a use case for these functions where name isn't a pointer to a string
    // literal, so it'll be fine
    g_profile_timers.name[index]  = name;
    g_profile_timers.open[index]  = true;
    g_profile_timers.calls[index] = 1;

    DEBUG_LOG("new profile timer added: %d - %s", index, name);
    return index;
}

void profile_end_timer(i32 index, i64 cycles)
{
    g_profile_timers.open[index]    = false;
    g_profile_timers.cycles[index] += cycles;

    // TODO(jesper): this doesn't seem like the best solution for a hierarchical
    // profile timer system. An O(n) look-up at both start and end is rather...
    // terrible
    for (i32 i = 0; i < g_profile_timers.count; i++) {
        if (g_profile_timers.open[i] == true && i != index) {
            g_profile_timers.cycles[i] -= cycles;
        }
    }
}

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

void profile_start_frame()
{
}

void profile_end_frame()
{
    g_profile_timers_prev.count = g_profile_timers.count;
    for (int i = 0; i < g_profile_timers.count; i++) {
        g_profile_timers_prev.name[i]   = g_profile_timers.name[i];
        g_profile_timers_prev.calls[i]  = g_profile_timers.calls[i];

        if (g_profile_timers.cycles[i] != 0) {
            g_profile_timers_prev.cycles[i] = g_profile_timers.cycles[i];
        }

        g_profile_timers.cycles[i] = 0;
        g_profile_timers.calls[i]  = 0;
    }

    // TODO(jesper): better sort
    for (int i = 0; i < g_profile_timers_prev.count - 1; i++) {
        int j, jmax = i;
        for (j = i+1; j < g_profile_timers_prev.count; j++) {
            if (g_profile_timers_prev.cycles[j] > g_profile_timers_prev.cycles[jmax]) {
                jmax = j;
            }
        }

        if (jmax != i) {
            std::swap(g_profile_timers_prev.name[i],   g_profile_timers_prev.name[jmax]);
            std::swap(g_profile_timers_prev.cycles[i], g_profile_timers_prev.cycles[jmax]);
            std::swap(g_profile_timers_prev.calls[i],  g_profile_timers_prev.calls[jmax]);
            std::swap(g_profile_timers_prev.open[i],   g_profile_timers_prev.open[jmax]);
        }
    }
}


#else // PROFILE_TIMERS_ENABLE

#define profile_end_timer(...)
#define profile_end_frame()

#endif

