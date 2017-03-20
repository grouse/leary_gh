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

#include "external/stb/stb_truetype.h"

#include "core/types.h"
#include "core/allocator.h"

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

struct GameMemory {
	void *game;
	LinearAllocator frame;
	LinearAllocator persistent;
};

#define SERIALIZE_SAVE_CONF(file, name, ptr) \
	serialize_save_conf(file, name ## _members, \
	                    sizeof(name ## _members) / sizeof(StructMemberInfo), \
	                    ptr)

#define SERIALIZE_LOAD_CONF(file, name, ptr) \
	serialize_load_conf(file, name ## _members, \
	                    sizeof(name ## _members) / sizeof(StructMemberInfo), \
	                    ptr)

#define GAME_FUNCS(M)\
	M(void, init, GameMemory*, PlatformState *);\
	M(void, load_platform_code, PlatformCode*);\
	M(void, quit, GameMemory*, PlatformState*);\
\
	M(void, input, GameMemory*, PlatformState*, InputEvent);\
	M(void, update_and_render, GameMemory*, f32)

#define GAME_TYPEDEF_FUNC(ret, name, ...) typedef ret game_##name##_t (__VA_ARGS__)
#define GAME_DCL_FPTR(ret, name, ...) game_##name##_t *name
#define GAME_DCL_STATIC_FPTR(ret, name, ...) static game_##name##_t *game_##name

#define MISC_FUNCS(M)\
	M(LinearAllocator, make_linear_allocator, void*, isize);\
	M(StackAllocator, make_stack_allocator, void*, isize);\
\
	M(void, profile_init, GameMemory*);\
	M(void, profile_start_frame, void);\
	M(void, profile_end_frame, void);\
	M(i32, profile_start_timer, const char*);\
	M(void, profile_end_timer, i32, u64);\
\
	M(void, serialize_load_conf, const char*, StructMemberInfo*, usize, void*);\
	M(void, serialize_save_conf, const char*, StructMemberInfo*, usize, void*)

#define MISC_TYPEDEF_FUNC(ret, name, ...) typedef ret name##_t (__VA_ARGS__)
#define MISC_DCL_FPTR(ret, name, ...) name##_t *name
#define MISC_DCL_STATIC_FPTR(ret, name, ...) static name##_t *name

#endif /* LEARY_H */

