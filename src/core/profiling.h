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

void profile_init(GameMemory *memory);
void profile_start_frame();
void profile_end_frame();
i32 profile_start_timer(const char *name);
void profile_end_timer(i32 index, u64 cycles);

#else

#define PROFILE_START(...)
#define PROFILE_END(...)

#define PROFILE_BLOCK(...)
#define PROFILE_FUNCTION(...)

void profile_init(GameMemory *) {}
void profile_start_frame() {}
void profile_end_frame() {}
i32 profile_start_timer(const char *) { return -1; }
void profile_end_timer(i32, u64) {}

#endif

#endif /* PROFILING_H */

