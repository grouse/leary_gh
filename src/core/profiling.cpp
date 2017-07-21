/**
 * file:    profiling.cpp
 * created: 2017-02-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#include "profiling.h"

#if PROFILE_TIMERS_ENABLE

StaticArray<ProfileTimer> g_profile_timers;
StaticArray<ProfileTimer> g_profile_timers_prev;

void profile_init(GameMemory *memory)
{
	void *buffer0 = alloc(&memory->persistent, sizeof(ProfileTimer) * NUM_PROFILE_TIMERS);
	void *buffer1 = alloc(&memory->persistent, sizeof(ProfileTimer) * NUM_PROFILE_TIMERS);

	g_profile_timers      = array_create_static<ProfileTimer>(buffer0, NUM_PROFILE_TIMERS);
	g_profile_timers_prev = array_create_static<ProfileTimer>(buffer1, NUM_PROFILE_TIMERS);
}


i32 profile_start_timer(const char *name)
{
	for (i32 i = 0; i < g_profile_timers.count; i++) {
		// TODO(jesper): hash the name and use as identifier
		if (strcmp(name, g_profile_timers[i].name) == 0) {
			g_profile_timers[i].open        = true;
			g_profile_timers[i].cycles_prev = 0;
			return i;
		}
	}

	i32 index = (i32)g_profile_timers.count++;
	// NOTE(jesper): using a static array here for memory performance reasons,
	// so if we run out it's tough shit and you better increase
	// NUM_PROFILE_TIMERS
	DEBUG_ASSERT(g_profile_timers.count < g_profile_timers.capacity);

	// NOTE(jesper): assume the passed in string won't be deallocated, I don't
	// see a use case for these functions where name isn't a pointer to a string
	// literal, so it'll be fine
	g_profile_timers[index].name = name;
	g_profile_timers[index].open = true;

	DEBUG_LOG("new profile timer added: %d - %s", index, name);
	return index;
}

void profile_end_timer(i32 index, i64 cycles)
{
	g_profile_timers[index].open         = false;
	g_profile_timers[index].cycles      += cycles;
	g_profile_timers[index].cycles_prev += cycles;

	// TODO(jesper): this doesn't seem like the best solution for a hierarchical
	// profile timer system. An O(n) look-up at both start and end is rather...
	// terrible
	for (i32 i = 0; i < g_profile_timers.count; i++) {
		if (g_profile_timers[i].open == true && i != index) {
			g_profile_timers[i].cycles      -= cycles;
			g_profile_timers[i].cycles_prev -= cycles;
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
	// TODO(jesper): better sort
	// TODO(jesper): I'm not sure actually sorting the global timers here is a
	// good idea. I'm thinking we want the global array in a static order so
	// that we can use taht for performance optimisation

	for (int i = 0; i < g_profile_timers_prev.count - 1; i++) {
		int j;
		int jMax = i;
		for (j = i+1; j < g_profile_timers_prev.count; j++) {
			if (g_profile_timers_prev[j].cycles > g_profile_timers_prev[i].cycles) {
				jMax = j;
			}
		}

		if (jMax != j) {
			ProfileTimer tmp            = g_profile_timers_prev[i];
			g_profile_timers_prev[i]    = g_profile_timers[jMax];
			g_profile_timers_prev[jMax] = tmp;
		}
	}
}

void profile_end_frame()
{
	g_profile_timers_prev  = g_profile_timers;
	g_profile_timers.count = 0;
}


#else // PROFILE_TIMERS_ENABLE

#define profile_end_timer(...)
#define profile_end_frame()

#endif

