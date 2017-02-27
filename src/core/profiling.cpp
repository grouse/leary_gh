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

struct ProfileTimers {
	const char **names;
	u64  *cycles;
	u64  *cycles_last;
	bool *open;
	i32 index;
	i32 max_index;
};

#define NUM_PROFILE_TIMERS (256)
ProfileTimers g_profile_timers;
ProfileTimers g_profile_timers_prev;

i32 start_profile_timer (const char *name)
{
	for (i32 i = 0; i < g_profile_timers.index; i++) {
		if (strcmp(name, g_profile_timers.names[i]) == 0) {
			g_profile_timers.open[i] = true;
			g_profile_timers.cycles_last[i] = 0;
			return i;
		}
	}

	i32 index = g_profile_timers.index++;
	DEBUG_ASSERT(g_profile_timers.index < g_profile_timers.max_index);

	// NOTE(jesper): assume the passed in string won't be deallocated, I don't
	// see a use case for these functions where name isn't a pointer to a string
	// literal, so it'll be fine
	g_profile_timers.names[index] = name;
	g_profile_timers.open[index] = true;

	DEBUG_LOG("new profile timer added: %d - %s", index, name);

	return index;
}

void end_profile_timer(i32 index, u64 cycles)
{
	g_profile_timers.cycles[index]      += cycles;
	g_profile_timers.cycles_last[index] += cycles;

	g_profile_timers.open[index] = false;

	for (i32 i = 0; i < g_profile_timers.index; i++) {
		if (g_profile_timers.open[i] == true && i != index) {
			g_profile_timers.cycles[i] -= cycles;
			g_profile_timers.cycles_last[i] -= cycles;
		}
	}
}

struct ProfileBlock {
	i32 id;
	u64 start_cycles;

	ProfileBlock(const char *name) {
		this->id = start_profile_timer(name);
		this->start_cycles = rdtsc();
	}

	~ProfileBlock() {
		u64 end_cycles = rdtsc();
		end_profile_timer(this->id, end_cycles - start_cycles);
	}
};

#define PROFILE_START(name)\
	u64 start_##name = rdtsc();\
	i32 profile_timer_id_##name = start_profile_timer(#name)

#define PROFILE_END(name)\
	u64 end_##name = rdtsc();\
	u64 difference_##name = end_##name - start_##name;\
	end_profile_timer(profile_timer_id_##name, difference_##name)

#define PROFILE_BLOCK(name) ProfileBlock profile_block_##name(#name)
#define PROFILE_FUNCTION() ProfileBlock profile_block_##__FUNCTION__(__FUNCTION__)

#else // PROFILE_TIMERS_ENABLE

#define start_profile_timer(...)
#define end_profile_timer(...)

#define PROFILE_START(...)
#define PROFILE_END(...)

#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION(...)

#endif

