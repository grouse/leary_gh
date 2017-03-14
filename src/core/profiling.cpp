/**
 * file:    profiling.cpp
 * created: 2017-02-27
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef PROFILE_TIMERS_ENABLE
#define PROFILE_TIMERS_ENABLE 1
#endif

#if PROFILE_TIMERS_ENABLE

#define NUM_PROFILE_TIMERS (256)

#define PROFILE_START(name)\
	u64 start_##name = rdtsc();\
	i32 profile_timer_id_##name = profile_start_timer(#name)

#define PROFILE_END(name)\
	u64 end_##name = rdtsc();\
	u64 difference_##name = end_##name - start_##name;\
	profile_end_timer(profile_timer_id_##name, difference_##name)

#define PROFILE_BLOCK(name) ProfileBlock profile_block_##name(#name)
#define PROFILE_FUNCTION() ProfileBlock profile_block_##__FUNCTION__(__FUNCTION__)

i32 profile_start_timer(const char *name);
void profile_end_timer(i32 index, u64 cycles);

struct ProfileTimers {
	StaticArray<const char*, LinearAllocator> names;
	StaticArray<u64, LinearAllocator>         cycles;
	StaticArray<u64, LinearAllocator>         cycles_last;
	StaticArray<bool, LinearAllocator>        open;
};

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

ProfileTimers g_profile_timers;
ProfileTimers g_profile_timers_prev;

i32 profile_start_timer(const char *name)
{
	for (i32 i = 0; i < g_profile_timers.names.count; i++) {
		if (strcmp(name, g_profile_timers.names[i]) == 0) {
			g_profile_timers.open[i]        = true;
			g_profile_timers.cycles_last[i] = 0;
			return i;
		}
	}

	i32 index = (i32)g_profile_timers.names.count++;
	DEBUG_ASSERT(g_profile_timers.names.count < g_profile_timers.names.capacity);

	// NOTE(jesper): assume the passed in string won't be deallocated, I don't
	// see a use case for these functions where name isn't a pointer to a string
	// literal, so it'll be fine
	g_profile_timers.names[index] = name;
	g_profile_timers.open[index]  = true;

	DEBUG_LOG("new profile timer added: %d - %s", index, name);
	return index;
}

void profile_end_timer(i32 index, u64 cycles)
{
	g_profile_timers.cycles[index]      += cycles;
	g_profile_timers.cycles_last[index] += cycles;

	g_profile_timers.open[index] = false;

	for (i32 i = 0; i < g_profile_timers.names.count; i++) {
		if (g_profile_timers.open[i] == true && i != index) {
			g_profile_timers.cycles[i]      -= cycles;
			g_profile_timers.cycles_last[i] -= cycles;
		}
	}
}

void profile_init(GameMemory *memory)
{
	auto names       = make_static_array<const char*>(&memory->persistent, NUM_PROFILE_TIMERS);
	auto cycles      = make_static_array<u64>(&memory->persistent, NUM_PROFILE_TIMERS);
	auto cycles_last = make_static_array<u64>(&memory->persistent, NUM_PROFILE_TIMERS);
	auto open        = make_static_array<bool>(&memory->persistent, NUM_PROFILE_TIMERS);

	g_profile_timers.names       = names;
	g_profile_timers.cycles      = cycles;
	g_profile_timers.cycles_last = cycles_last;
	g_profile_timers.open        = open;

	names       = make_static_array<const char*>(&memory->persistent, NUM_PROFILE_TIMERS);
	cycles      = make_static_array<u64>(&memory->persistent, NUM_PROFILE_TIMERS);
	cycles_last = make_static_array<u64>(&memory->persistent, NUM_PROFILE_TIMERS);
	open        = make_static_array<bool>(&memory->persistent, NUM_PROFILE_TIMERS);

	g_profile_timers_prev.names       = names;
	g_profile_timers_prev.cycles      = cycles;
	g_profile_timers_prev.cycles_last = cycles_last;
	g_profile_timers_prev.open        = open;
}

void profile_start_frame()
{
	for (i32 i = 0; i < g_profile_timers_prev.names.count - 1; i++) {
		for (i32 j = i+1; j < g_profile_timers_prev.names.count; j++) {
			if (g_profile_timers_prev.cycles[j] > g_profile_timers_prev.cycles[i]) {
				const char *name_tmp = g_profile_timers_prev.names[j];
				u64 cycles_tmp = g_profile_timers_prev.cycles[j];
				u64 cycles_last_tmp = g_profile_timers_prev.cycles_last[j];

				g_profile_timers_prev.names[j] = g_profile_timers_prev.names[i];
				g_profile_timers_prev.cycles[j] = g_profile_timers_prev.cycles[i];
				g_profile_timers_prev.cycles_last[j] = g_profile_timers_prev.cycles_last[i];

				g_profile_timers_prev.names[i] = name_tmp;
				g_profile_timers_prev.cycles[i] = cycles_tmp;
				g_profile_timers_prev.cycles_last[i] = cycles_last_tmp;
			}
		}
	}
}

void profile_end_frame()
{
	ProfileTimers tmp      = g_profile_timers_prev;
	g_profile_timers_prev  = g_profile_timers;
	g_profile_timers       = tmp;

	for (i32 i = 0; i < g_profile_timers.names.count; i++) {
		g_profile_timers.cycles[i] = 0;
	}
}


#else // PROFILE_TIMERS_ENABLE

#define profile_start_frame()
#define profile_start_timer(...)

#define profile_end_timer(...)
#define profile_end_frame()

#define PROFILE_START(...)
#define PROFILE_END(...)

#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION(...)

#endif

