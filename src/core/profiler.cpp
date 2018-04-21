/**
 * file:    profiler.cpp
 * created: 2017-02-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "profiler.h"

#define PROFILER_MAX_STACK_DEPTH (256)

enum ProfileEventType {
    ProfileEvent_start,
    ProfileEvent_end
};

struct ProfileEvent {
    ProfileEventType type;
    const char *name;
    u64 timestamp;
};

struct ProfileTimer {
    const char *name;
    u64 duration;
    u64 calls;
    i32 parent;
};

Array<ProfileEvent> g_profile_events;
Array<ProfileEvent> g_profile_events_prev;

Array<ProfileTimer> g_profile_timers;

void profiler_start(const char *name)
{
    ProfileEvent event;
    event.type = ProfileEvent_start;
    event.name = name;
    event.timestamp = cpu_ticks();

    array_add(&g_profile_events, event);
}

void profiler_end(const char *name)
{
    ProfileEvent event;
    event.type = ProfileEvent_end;
    event.name = name;
    event.timestamp = cpu_ticks();

    array_add(&g_profile_events, event);
}

void init_profiler()
{
    init_array(&g_profile_events, g_heap);
    init_array(&g_profile_events_prev, g_heap);
    init_array(&g_profile_timers, g_heap);
}

void profiler_begin_frame()
{
    // TODO(jesper): THREAD_SAFETY
    std::swap(g_profile_events, g_profile_events_prev);
    g_profile_events.count = 0;
    g_profile_timers.count = 0;

    PROFILE_FUNCTION();

    i32 s = 0;
    i32 stack[PROFILER_MAX_STACK_DEPTH];

    for (i32 i = 0; i < PROFILER_MAX_STACK_DEPTH; i++) {
        stack[i] = -1;
    }

    // TODO(jesper): thread local g_profile_events
    for (i32 i = 0; i < g_profile_events_prev.count; i++) {
        ProfileEvent event = g_profile_events_prev[i];

        if (event.type == ProfileEvent_start) {
            stack[s++] = i;
            ASSERT(s < PROFILER_MAX_STACK_DEPTH);
        } else if (event.type == ProfileEvent_end) {
            i32 pid = stack[--s];
            ASSERT(pid != -1);

            ProfileEvent parent = g_profile_events_prev[pid];
            ASSERT(event.name == parent.name || strcmp(event.name, parent.name) == 0);

            ProfileTimer timer;
            timer.name = event.name;
            timer.duration = event.timestamp - parent.timestamp;
            timer.calls = 1;

            i32 j = 0;
            for (j = 0; j < g_profile_timers.count; j++) {
                ProfileTimer exist = g_profile_timers[j];

                if (exist.name == timer.name ||
                    strcmp(exist.name, timer.name) == 0)
                {
                    goto merge_existing;
                }
            }

            array_add(&g_profile_timers, timer);
            continue;

merge_existing:
            ProfileTimer &existing = g_profile_timers[j];
            existing.calls++;
            existing.duration += timer.duration;
        }
    }

    {
        PROFILE_SCOPE(profiler_timer_sort);
        array_ins_sort(
            &g_profile_timers,
            [](ProfileTimer *lhs, ProfileTimer *rhs) {
                return lhs->duration < rhs->duration;
            });
    }
}

void profiler_end_frame()
{
}
