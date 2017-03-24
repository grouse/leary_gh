/**
 * file:    leary.h
 * created: 2017-03-18
 * authors: Jesper Stefansson (jesper.stefansson@gmail.com)
 *
 * Copyright (c) 2017 - all rights reserved
 */

#ifndef LEARY_H
#define LEARY_H

#ifndef INTROSPECT
#define INTROSPECT
#endif

#include <math.h>

#include "platform/platform.h"
#include "platform/platform_input.h"

#include "core/types.h"
#include "core/allocator.h"

#define ARRAY_SIZE(a) sizeof((a)) / sizeof((a)[0])

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

struct GameMemory {
	void *game;
	LinearAllocator frame;
	LinearAllocator persistent;
};

#define GAME_INIT_FUNC(fname)               void fname(GameMemory *memory, PlatformState *platform)
#define GAME_LOAD_PLATFORM_CODE_FUNC(fname) void fname(PlatformCode *code)
#define GAME_QUIT_FUNC(fname)               void fname(GameMemory *memory, PlatformState *platform)
#define GAME_INPUT_FUNC(fname)              void fname(GameMemory *memory, PlatformState *platform, InputEvent event)
#define GAME_UPDATE_AND_RENDER_FUNC(fname)  void fname(GameMemory *memory, f32 dt)

#define MAKE_LINEAR_ALLOCATOR_FUNC(fname) LinearAllocator fname(void *start, isize size)
#define MAKE_STACK_ALLOCATOR_FUNC(fname)  StackAllocator fname(void *start, isize size)

#define PROFILE_INIT_FUNC(fname)          ProfileState fname(GameMemory *memory)
#define PROFILE_SET_STATE_FUNC(fname)     void fname(ProfileState *state)
#define PROFILE_START_FRAME_FUNC(fname)   void fname()
#define PROFILE_END_FRAME_FUNC(fname)     void fname()
#define PROFILE_START_TIMER_FUNC(fname)   i32 fname(const char *name)
#define PROFILE_END_TIMER_FUNC(fname)     void fname(i32 index, u64 cycles)

#define SERIALIZE_LOAD_CONF_FUNC(fname)   void fname(const char *path, StructMemberInfo *members, usize num_members, void *out)
#define SERIALIZE_SAVE_CONF_FUNC(fname)   void fname(const char *path, StructMemberInfo *members, usize num_members, void *ptr)

#endif /* LEARY_H */

